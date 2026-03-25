mpicc -o 3_2 3_2_new.c

mpirun -np 7 ./3_2 ../data/nsymmat.txt res.txt