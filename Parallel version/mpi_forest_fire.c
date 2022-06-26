#include <stdlib.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <stdbool.h>
#include "mpi.h"
#include "mpi_forest_fire.h"

#define STEPS 500
#define POINT_SIZE 4
#define TITLE "MPI Forest - Alessandro Monetti mat. 220021"

enum states {EMPTY, TREE, BURNT_TREE, BURNING_LOW, BURNING_MID, BURNING_HIGH, WATER_LOW, WATER_MID, WATER_HIGH};

int rank,
    size,
    source,
    dest,
    nRows = 150,
    nCols,
    up,
    down,
    *oldPlane,
    *newPlane,
    *mainPlane;

float displayRest = 1.0/144.0;

ALLEGRO_DISPLAY *display;
ALLEGRO_EVENT event;
ALLEGRO_EVENT_QUEUE *queue;

MPI_Datatype MPI_MYROW;
MPI_Datatype MPI_MYLOCALMATRIX;

MPI_Comm forest;

int main(int argc, char** argv){
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    while(nRows--%size != 0) {} ++nRows;
    nCols = nRows;
    
    srand((unsigned)time(NULL) + rank);
    //Allocating the two local planes
    oldPlane = (int*) calloc((nRows/size+2)*nCols,sizeof(int));
    newPlane = (int*) calloc((nRows/size+2)*nCols,sizeof(int));

    int dimensions[1]   = {size};
    int periods[1]      = {0};
    
    //Creating the 1D (vertical) virtual topology 
    MPI_Cart_create(MPI_COMM_WORLD, 1, dimensions, periods, 0, &forest);
    MPI_Cart_shift(forest, 0, 1, &up, &down);

    //Creating some new contiguous types to avoid calculating the count value for every send/receive/gather
    MPI_Type_contiguous(nCols, MPI_INT, &MPI_MYROW);
    MPI_Type_contiguous((nRows/size)*nCols, MPI_INT, &MPI_MYLOCALMATRIX);
    MPI_Type_commit(&MPI_MYROW);
    MPI_Type_commit(&MPI_MYLOCALMATRIX);

    if(rank == 0){
        mainPlane = (int*)calloc(nRows*nCols, sizeof(int));
        if(allegroInit() == -1)
            MPI_Abort(forest, -1);
    }

    int numSteps = 0;
    while(numSteps++ < STEPS || 1==1){
        sendBorders();
        applyTransFuncInside();
        recvBorders();
        applyTransFuncAroundHalo();
        swapPlanes();  
        MPI_Gather(&oldPlane[matCoordToIndex(1,0)], 1, MPI_MYLOCALMATRIX, mainPlane, 1, MPI_MYLOCALMATRIX, 0, forest);

        if(rank == 0){
            printPlane(mainPlane, nRows, nCols);
            al_peek_next_event(queue, &event);
            if(event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
                break;
        }
    }

    if(rank == 0){
        allegroDestroy();
        free(mainPlane);
        mainPlane = 0;
    }

    free(newPlane);
    free(oldPlane);
    oldPlane = newPlane = 0;
    MPI_Finalize();
}

int allegroInit(){
    if(!al_init()){
        printf("Error: failed to initalize allegro!\n");
        return -1;
    }

    display = al_create_display(nRows*(POINT_SIZE+1), nCols*(POINT_SIZE+1));
    queue = al_create_event_queue();

    al_init_primitives_addon();

    al_register_event_source(queue, al_get_display_event_source(display));
    // set allegro window title
	al_set_window_title(display, TITLE);
}

void allegroDestroy(){
    if (queue) {
        al_destroy_event_queue(queue);
        queue = 0;
    }

    if(display){
        al_destroy_display(display);
        display = 0;
    }   
}

void applyTransFuncInside(){
    int startIdx = 2, endIdx = (nRows/size)-1;
    int isZero = (rank == 0);
    int isLast = (rank == size-1);
    if(isZero)
        --startIdx;         //startIdx = 1;
    if(isLast)
        ++endIdx;           //endIdx = (nRows/size);

    for(int i = startIdx; i <= endIdx; ++i)
        for(int j = 0; j < nCols; ++j)
            transFunc(i, j, (nRows/size)+1);
}

void applyTransFuncAroundHalo(){
    int isZero = (rank == 0);
    int isLast = (rank == size-1);
    int startIdx = 1, endIdx = (nRows/size);
    if(isZero)
        startIdx = (nRows/size);
    if (isLast)
        endIdx = 1;
    for(int i = startIdx; i <= endIdx; i+=(nRows/size)-1){
        for(int j = 0; j < nCols; ++j){
            transFunc(i, j, (nRows/size)+1);
        }
    }       
}

void transFunc(int row, int col, int rowSize){
    switch (oldPlane[matCoordToIndex(row,col)])
    {
        case EMPTY:
            if(!(rand() % 500))
                newPlane[matCoordToIndex(row,col)] = TREE;
                break;

        case TREE:
            if(hasNeighbor(BURNING_HIGH, oldPlane, row, col, rowSize, nCols) || !(rand() % 50000))
                newPlane[matCoordToIndex(row,col)] = BURNING_HIGH;
            else
                newPlane[matCoordToIndex(row,col)] = TREE;
            break;

        case BURNING_HIGH:
            if(hasNeighbor(WATER_HIGH, oldPlane, row, col, rowSize, nCols) || !(rand() % 500))
                newPlane[matCoordToIndex(row,col)] = WATER_HIGH;
            else
                newPlane[matCoordToIndex(row,col)] = BURNING_MID;
            break;
            
        case BURNING_MID:
            if(hasNeighbor(WATER_HIGH, oldPlane, row, col, rowSize, nCols) || hasNeighbor(WATER_MID, oldPlane, row, col, rowSize, nCols))
                newPlane[matCoordToIndex(row,col)] = WATER_MID;
            else
                newPlane[matCoordToIndex(row,col)] = BURNING_LOW;
            break;

        case BURNING_LOW:
            if(hasNeighbor(WATER_HIGH, oldPlane, row, col, rowSize, nCols) || hasNeighbor(WATER_MID, oldPlane, row, col, rowSize, nCols)|| hasNeighbor(WATER_LOW, oldPlane, row, col, rowSize, nCols))
                newPlane[matCoordToIndex(row,col)] = WATER_LOW;
            else
                newPlane[matCoordToIndex(row,col)] = EMPTY;
            break;
                    
        case WATER_LOW:
            newPlane[matCoordToIndex(row,col)] = BURNT_TREE;
            break;

        case WATER_MID:
            newPlane[matCoordToIndex(row,col)] = WATER_LOW;
            break;

        case WATER_HIGH:
            newPlane[matCoordToIndex(row,col)] = WATER_MID;
            break;

        case BURNT_TREE:
            if(!(rand() % 100))
                newPlane[matCoordToIndex(row,col)] = TREE;
            else
                newPlane[matCoordToIndex(row,col)] = BURNT_TREE;
            break;

        default: break;
    }
}

void sendBorders(){
    MPI_Request req;

    //rank 0 only sends down because he works onto matrix's top-most area
    if(rank == 0)
        MPI_Isend(oldPlane+matCoordToIndex((nRows/size),0), 1, MPI_MYROW, down, 666, forest, &req);
    //last rank only sends up because he works onto matrix's bottom-most area
    else if(rank == size-1)
        MPI_Isend(oldPlane+matCoordToIndex(1,0), 1, MPI_MYROW, up, 420, forest, &req);
    //other ranks send their borders either up and down
    else{
        MPI_Isend(oldPlane+matCoordToIndex(1,0), 1, MPI_MYROW, up, 420, forest, &req);
        MPI_Isend(oldPlane+matCoordToIndex((nRows/size),0), 1, MPI_MYROW, down, 666, forest, &req);
    }
}

void recvBorders(){
    MPI_Status status;

    //rank 0 only receives from below because he works onto matrix's top-most area
    if(rank == 0)
        MPI_Recv(oldPlane+matCoordToIndex((nRows/size)+1,0), 1, MPI_MYROW, down, 420, forest, &status);
    //last rank only receives from above because he works onto matrix's bottom-most area
    else if(rank == size-1)
        MPI_Recv(oldPlane+matCoordToIndex(0,0), 1, MPI_MYROW, up, 666, forest, &status);
    //other ranks receive their borders from either above and below
    else{
        MPI_Recv(oldPlane+matCoordToIndex(0,0), 1, MPI_MYROW, up, 666, forest, &status);
        MPI_Recv(oldPlane+matCoordToIndex((nRows/size)+1,0), 1, MPI_MYROW, down, 420, forest, &status);
    }
}

int matCoordToIndex(int row, int column){ return (row*nCols+column); }

void printPlane(int* plane, int rowSize, int colSize){
    al_clear_to_color(al_map_rgb(0, 0, 0));
    for(int y = 0; y < rowSize; ++y){
        for(int x = 0; x < colSize; ++x){
            switch (plane[matCoordToIndex(y,x)])
            {
                case EMPTY:
                    al_draw_filled_rectangle(x * (POINT_SIZE+1), y * (POINT_SIZE+1), x * (POINT_SIZE+1) + POINT_SIZE, y * (POINT_SIZE+1) + POINT_SIZE, al_map_rgb(18,18,18));  
                    break;
                
                case TREE: 
                    al_draw_filled_rectangle(x * (POINT_SIZE+1), y * (POINT_SIZE+1), x * (POINT_SIZE+1) + POINT_SIZE, y * (POINT_SIZE+1) + POINT_SIZE, al_map_rgb(34,139,34));   
                    break;
                
                case BURNING_HIGH: 
                    al_draw_filled_rectangle(x * (POINT_SIZE+1), y * (POINT_SIZE+1), x * (POINT_SIZE+1) + POINT_SIZE, y * (POINT_SIZE+1) + POINT_SIZE, al_map_rgb(255,0,0));
                    break;
                
                case BURNING_MID: 
                    al_draw_filled_rectangle(x * (POINT_SIZE+1), y * (POINT_SIZE+1), x * (POINT_SIZE+1) + POINT_SIZE, y * (POINT_SIZE+1) + POINT_SIZE, al_map_rgb(139,0,0));  
                    break;
                
                case BURNING_LOW: 
                    al_draw_filled_rectangle(x * (POINT_SIZE+1), y * (POINT_SIZE+1), x * (POINT_SIZE+1) + POINT_SIZE, y * (POINT_SIZE+1) + POINT_SIZE, al_map_rgb(69,0,0));  
                    break;

                case WATER_LOW:
                    al_draw_filled_rectangle(x * (POINT_SIZE+1), y * (POINT_SIZE+1), x * (POINT_SIZE+1) + POINT_SIZE, y * (POINT_SIZE+1) + POINT_SIZE, al_map_rgb(0,76,153)); 
                    break;

                case WATER_MID:
                    al_draw_filled_rectangle(x * (POINT_SIZE+1), y * (POINT_SIZE+1), x * (POINT_SIZE+1) + POINT_SIZE, y * (POINT_SIZE+1) + POINT_SIZE, al_map_rgb(0,102,204));
                    break;

                case WATER_HIGH:
                    al_draw_filled_rectangle(x * (POINT_SIZE+1), y * (POINT_SIZE+1), x * (POINT_SIZE+1) + POINT_SIZE, y * (POINT_SIZE+1) + POINT_SIZE, al_map_rgb(0,128,255));
                    break;
                
                case BURNT_TREE:
                    al_draw_filled_rectangle(x * (POINT_SIZE+1), y * (POINT_SIZE+1), x * (POINT_SIZE+1) + POINT_SIZE, y * (POINT_SIZE+1) + POINT_SIZE, al_map_rgb(0,69,0));  
                
                default: break;
            }
        }
    }
    al_flip_display();
    //al_rest(1);               //1Hz mode
    //al_rest(1.0 / 10.0);      //10Hz mode
    //al_rest(1.0 / 30.0);      //30Hz mode
    //al_rest(1.0 / 60.0);      //60Hz mode
    //al_rest(1.0 / 144.0);     //144Hz mode
    al_rest(displayRest);
}

void swapPlanes(){
    free(oldPlane);
    oldPlane = newPlane;
    newPlane = (int*) calloc((nRows/size+2)*nCols,sizeof(int));
}

int hasNeighbor(int STATE, int* plane, int row, int col, int rowSize, int colSize){
    int upperRow    = row-1, 
        lowerRow    = row+1,   
        leftCol     = col-1,
        rightCol    = col+1, 
        upperInside = upperRow >= 0,
        lowerInside = lowerRow <= rowSize,
        leftInside  = leftCol >= 0,
        rightInside = rightCol < colSize;

    if(rightInside){
        if(plane[matCoordToIndex(row, rightCol)] == STATE)
            return true;
        if(upperInside)
            if(plane[matCoordToIndex(upperRow, rightCol)] == STATE)
                return true;
        if(lowerInside)
            if(plane[matCoordToIndex(lowerRow, rightCol)] == STATE)
                return true;
    }

    if(leftCol){
        if(plane[matCoordToIndex(row, leftCol)] == STATE)
            return true;
        if(upperInside)
            if(plane[matCoordToIndex(upperRow, leftCol)] == STATE)
                return true;
        if(lowerInside)
            if(plane[matCoordToIndex(lowerRow, leftCol)] == STATE)
                return true;
    }

    if(upperInside)
        if(plane[matCoordToIndex(upperRow, col)] == STATE)
            return true;
    
    if(lowerInside)
        if(plane[matCoordToIndex(lowerRow, col)] == STATE)
            return true;

    return false;
}