#include <stdlib.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <stdbool.h>
#include "forest_fire_seq.h"

#define STEPS 500
#define DEFAULT_SIZE 25
#define POINT_SIZE 4
#define TITLE "Forest Allegro (Sequential) - Alessandro Monetti mat. 220021"

int rowSize, colSize, ** oldPlane, ** newPlane;
float displayRest = (1/30.0);

enum states {EMPTY, TREE, BURNT_TREE, BURNING_LOW, BURNING_MID, BURNING_HIGH, WATER_LOW, WATER_MID, WATER_HIGH};

int main(int argc, char** argv){
    srand((unsigned)time(NULL));
    //argv[0] number of rows and columns
    switch(argc){
        case 1:
            colSize = rowSize = DEFAULT_SIZE;
            break;
        case 2: {
            rowSize = atoi(*(argv+1));
            colSize = rowSize;
            break;
        }
        case 3: {
            rowSize = atoi(*(argv+1));
            colSize = atoi(*(argv+2));
            break;
        }
        default: {
            printf("Error: too many arguments given.\nMax was 2, where given: %d\n", argc-1);
            exit(1);
            break;
        }   
    }  

    //allegro setup
    ALLEGRO_DISPLAY *display;

    if(!al_init()){
        printf("Error: failed to initalize allegro!\n");
        return -1;
    }
    
    display = al_create_display(rowSize*(POINT_SIZE+1), colSize*(POINT_SIZE+1));
    al_init_primitives_addon();

    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_EVENT event;

    al_register_event_source(queue, al_get_display_event_source(display));
    // set title
	al_set_window_title(display, TITLE);

    oldPlane = allocatePlane(rowSize, colSize);
    newPlane = allocatePlane(rowSize, colSize); 
    int stepsDone = 0;
    while(6*10+9 == 420-351){       //Funny way to loop indefinitely
        al_peek_next_event(queue, &event);
        if(event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;

        for(int row = 0; row < rowSize; ++row){
            for(int col = 0; col < colSize; ++col){
                switch (oldPlane[row][col])
                {
                    case EMPTY:
                        if(!(rand() % 500))
                            newPlane[row][col] = TREE;
                        break;

                    case TREE:
                        if(hasNeighbor(BURNING_HIGH, oldPlane, row, col, rowSize, colSize) || !(rand() % 50000))
                            newPlane[row][col] = BURNING_HIGH;
                        else
                            newPlane[row][col] = TREE;
                        break;

                    case BURNING_HIGH:
                        if(hasNeighbor(WATER_HIGH, oldPlane, row, col, rowSize, colSize) || !(rand() % 500))
                            newPlane[row][col] = WATER_HIGH;
                        else
                            newPlane[row][col] = BURNING_MID;
                        break;
            
                    case BURNING_MID:
                        if(hasNeighbor(WATER_HIGH, oldPlane, row, col, rowSize, colSize) || hasNeighbor(WATER_MID, oldPlane, row, col, rowSize, colSize))
                            newPlane[row][col] = WATER_MID;
                        else
                            newPlane[row][col] = BURNING_LOW;
                        break;

                    case BURNING_LOW:
                        if(hasNeighbor(WATER_HIGH, oldPlane, row, col, rowSize, colSize) || hasNeighbor(WATER_MID, oldPlane, row, col, rowSize, colSize)|| hasNeighbor(WATER_LOW, oldPlane, row, col, rowSize, colSize))
                            newPlane[row][col] = WATER_LOW;
                        else
                            newPlane[row][col] = EMPTY;
                        break;
                    
                    case WATER_LOW:
                        newPlane[row][col] = BURNT_TREE;
                        break;

                    case WATER_MID:
                        newPlane[row][col] = WATER_LOW;
                        break;

                    case WATER_HIGH:
                        newPlane[row][col] = WATER_MID;
                        break;

                    case BURNT_TREE:
                        if(!(rand() % 100))
                            newPlane[row][col] = TREE;
                        else
                            newPlane[row][col] = BURNT_TREE;
                        break;

                    default: break;
                }
            }
        }
        free(oldPlane);
        oldPlane = newPlane;
        printPlane(newPlane, rowSize, colSize); 
        newPlane = allocatePlane(rowSize, colSize);  
    }

    al_destroy_event_queue(queue);
    al_destroy_display(display);

    deallocatePlane(oldPlane, rowSize);
    deallocatePlane(newPlane, rowSize);
    return 0;
}

int** allocatePlane(int rowSize, int colSize){
    int** plane = calloc(rowSize, sizeof(int*));
    for(int i = 0; i < rowSize; ++i)
        *(plane+i) = calloc(colSize, sizeof(int));

    return plane;
}

void deallocatePlane(int** plane, int rowSize){
    for(int i = 0; i < rowSize; i++)
        free(*(plane+i));
    free(plane);
}

void printPlane(int** plane, int rowSize, int colSize){
    al_clear_to_color(al_map_rgb(0, 0, 0));
    for(int y = 0; y < rowSize; ++y){
        for(int x = 0; x < colSize; ++x){
            switch (plane[y][x])
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

int hasNeighbor(int STATE, int** plane, int row, int col, int rowSize, int colSize){
    int upperRow    = row-1, 
        lowerRow    = row+1,   
        leftCol     = col-1,
        rightCol    = col+1, 
        upperInside = upperRow >= 0,
        lowerInside = lowerRow < rowSize,
        leftInside  = leftCol >= 0,
        rightInside = rightCol < colSize;

    if(rightInside){
        if(plane[row][rightCol] == STATE)
            return true;
        if(upperInside)
            if(plane[upperRow][rightCol] == STATE)
                return true;
        if(lowerInside)
            if(plane[lowerRow][rightCol] == STATE)
                return true;
    }

    if(leftCol){
        if(plane[row][leftCol] == STATE)
            return true;
        if(upperInside)
            if(plane[upperRow][leftCol] == STATE)
                return true;
        if(lowerInside)
            if(plane[lowerRow][leftCol] == STATE)
                return true;
    }

    if(upperInside)
        if(plane[upperRow][col] == STATE)
            return true;
    
    if(lowerInside)
        if(plane[lowerRow][col] == STATE)
            return true;

    return false;
}