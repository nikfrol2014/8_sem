#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int check_symmetry(double *matrix, int n, int start, int end) {
    
    int is_sym = 1;
    
    for (int i = start; i < end; i++) {
        for (int j = 0; j < i; j++) {
            if (matrix[i * n + j] != matrix[j * n + i]) {
                is_sym = 0;
            }
        }
    }

    return is_sym;

}

int main(int argc, char *argv[]) {
    
    int n, rank, size, local_result, global_result;
    double *matrix = NULL;
    char *filename = NULL;

    /*Инициализация MPI. Получаем ранг текущего процесса и общее количество процессов*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /*Проверка аргументов командной строки только для корневого процесса*/
    if (rank == 0) {
        if (argc != 2) {
            printf("Использование: %s <имя_файла>\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        filename = argv[1];
    }

    /*Чтение матрицы только для корневого процесса*/
    if (rank == 0) {
        
        FILE *file = fopen(filename, "r");
        if (!file) {
            printf("Ошибка открытия файла %s\n", filename);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fscanf(file, "%d", &n);

        matrix = (double *)malloc(n * n * sizeof(double));
        if (!matrix) {
            printf("Ошибка выделения памяти для матрицы\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (int i = 0; i < n * n; i++) {
            fscanf(file, "%lf", &matrix[i]);
        }

        fclose(file);

    }

    /*Передача размера матрицы всем процессам*/
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /*Выделение памяти для матрицы всеми процессами*/
    if (rank != 0) {
        
        matrix = (double *)malloc(n * n * sizeof(double));
        if (!matrix) {
            printf("Ошибка выделения памяти в процессе %d\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

    }

    /*Передача матрицы всем процессам*/
    MPI_Bcast(matrix, n * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    /*Распределение строк матрицы между процессами*/
    int rows_per_proc = n / size; /*Базовое количество строк на процесс*/
    int extra_rows = n % size; /*Лишние строки матрицы для распределения между процессами*/
    int start_row = rank * rows_per_proc + (rank < extra_rows ? rank : extra_rows); /*Начало диапозона строк*/
    int end_row = start_row + rows_per_proc + (rank < extra_rows); /*Конец диапозона строк*/

    local_result = check_symmetry(matrix, n, start_row, end_row);
    
    /*Собирает результаты со всех процессов с операцией MPI_MIN
     Если хотя бы один процесс обнаружил несимметричность (вернул 0), то и глобальный результат будет 0*/
    MPI_Reduce(
        &local_result, /*Входные данные от каждого процесса */
        &global_result, /*Буфер для итогового результата*/
        1, /*Количество элементов*/
        MPI_INT, /*Тип данных*/
        MPI_MIN, /*Операция редукции*/
        0, /*Ранг корневого процесса*/
        MPI_COMM_WORLD); /*Коммуникатор*/
    
    /*Вывод результат проверки только корневым процессом*/
    if (rank == 0) {
        if (global_result) {
            printf("Матрица симметрична\n");
        } else {
            printf("Матрица не симметрична\n");
        }
    }

    free(matrix);
    MPI_Finalize(); /*Завершение работы MPI*/

    return 0;

}