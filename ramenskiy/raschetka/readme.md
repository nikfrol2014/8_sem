gcc -o main main.c -lm -pthread

./main xy.txt xz.txt yz.txt

XY — вид сверху (координаты x и y)

XZ — вид спереди (координаты x и z)

YZ — вид сбоку (координаты y и z)

-------------------------
ТЕКУЩАЯ ВЕРСИЯ :
gcc -o try try.c -lm -lpthread
./try "polyhedral 2D.txt" 