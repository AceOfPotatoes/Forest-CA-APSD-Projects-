#ifndef MPI_FOREST_FIRE_H
#define MPI_FOREST_FIRE_H
inline int allegroInit();
inline void allegroDestroy();
inline void sendBorders();
inline void recvBorders();
inline void swapPlanes();
inline void transFunc(int row, int col, int rowSize);
inline void applyTransFuncAroundHalo();
inline void applyTransFuncInside();
inline int matCoordToIndex(int row, int column);
inline void printPlane(int* plane, int rowSize, int colSize);
int hasNeighbor(int STATE, int* plane, int row, int col, int rowSize, int colSize);
#endif