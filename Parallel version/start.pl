#!/usr/bin/perl
system("mpicc mpi_forest_fire.c -lallegro -lallegro_primitives -lallegro_image && mpirun -oversubscribe -np 4 ./a.out");
qx(rm a.out);