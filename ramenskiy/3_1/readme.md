mpicc -o 3_1 3_1_new.c

mpirun -np 4 ./3_1 ../data/symmat.txt

или

mpirun -np 4 ./3_1 ../data/nsymmat.txt

# Установить Open MPI (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install openmpi-bin openmpi-common libopenmpi-dev

# После установки, убедитесь, что используется правильный mpirun
which mpirun
# Должно показывать /usr/bin/mpirun или /usr/local/bin/mpirun