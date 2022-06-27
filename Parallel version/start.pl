#!/usr/bin/perl
system("mpicc mpi_forest_fire.c -lallegro -lallegro_primitives && mpirun -oversubscribe -np 8 ./a.out");
qx(rm a.out);