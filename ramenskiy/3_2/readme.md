mpicc -o 3_2 3_2.c -lm

mpirun -np 4 ./3_2 nonsym_matrix.txt