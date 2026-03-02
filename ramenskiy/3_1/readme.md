mpicc -o 3_1 3_1.c -lm

mpirun -np 4 ./3_1 matrix.txt