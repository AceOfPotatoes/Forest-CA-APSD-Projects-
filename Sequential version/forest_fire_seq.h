int** allocatePlane(int rowSize, int colSize);
void deallocatePlane(int** plane, int rowSize);
void printPlane(int** plane, int rowSize, int colSize);
int hasNeighbor(int STATE, int** plane, int row, int col, int rowSize, int colSize);