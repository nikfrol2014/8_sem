#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

void symmetrize(double *matrix, int n, int start, int end) {
    
    for (int i = start; i < end; i++) {
        for (int j = 0; j < i; j++) {
            matrix[i * n + j] = (matrix[i * n + j] + matrix[j * n + i]) / 2;
            matrix[j * n + i] = matrix[i * n + j];
        }
    }

}

int main(int argc, char *argv[]) {
    
    int n, rank, size;
    double *matrix = NULL;
    double *local_matrix = NULL;
    char *input_filename = NULL;
    // char *output_filename = NULL;
    
    /*Массивы для сбора данных*/
    int *displs = NULL; /*Количество элементов от каждого процесса*/
    int *sendcounts = NULL; /*Смещения для размещения данных в результирующей матрице*/

    /*Инициализация MPI. Получаем ранг текущего процесса и общее количество процессов*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /*Проверка аргументов командной строки только для корневого процесса*/
    if (rank == 0) {
        if (argc != 3) {
            fprintf(stderr, "Использование: %s <входной_файл>\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        input_filename = argv[1];
        // output_filename = argv[2];
    }

    /*Чтение матрицы только для корневого процесса*/
    if (rank == 0) {
        FILE *file = fopen(input_filename, "r");
        if (!file) {
            fprintf(stderr, "Ошибка открытия файла %s\n", input_filename);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if (fscanf(file, "%d", &n) != 1) {
            fprintf(stderr, "Ошибка чтения размера матрицы\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        matrix = (double *)malloc(n * n * sizeof(double));
        if (!matrix) {
            fprintf(stderr, "Ошибка выделения памяти для матрицы\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (int i = 0; i < n * n; i++) {
            if (fscanf(file, "%lf", &matrix[i]) != 1) {
                fprintf(stderr, "Ошибка чтения элемента матрицы\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
        fclose(file);
    }

    /*Передача размера матрицы всем процессам*/
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /*Локальная копия матрицы для каждого процесса*/
    local_matrix = (double *)malloc(n * n * sizeof(double));
    if (!local_matrix) {
        fprintf(stderr, "Ошибка выделения памяти в процессе %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /*Копирования данных из matrix в local_matrix*/
    if (rank == 0) {
        memcpy(local_matrix, matrix, n * n * sizeof(double));
    }
    
    /*Передача матрицы всем процессам*/
    MPI_Bcast(local_matrix, n * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    /*Распределение строк матрицы между процессами*/
    int rows_per_proc = n / size; /*Базовое количество строк на процесс*/
    int extra_rows = n % size; /*Лишние строки матрицы для распределения между процессами*/
    int start_row = rank * rows_per_proc + (rank < extra_rows ? rank : extra_rows); /*Начало диапозона строк*/
    int end_row = start_row + rows_per_proc + (rank < extra_rows ? 1 : 0); /*Конец диапозона строк*/
    int send_count = (end_row - start_row) * n; /* Количество элементов матрицы, которые текущий процесс должен отправить корневому процессу*/

    symmetrize(local_matrix, n, start_row, end_row);

    if (rank == 0) {

        displs = (int*)malloc(size * sizeof(int));
        sendcounts = (int*)malloc(size * sizeof(int));
        
        int offset = 0;  /*Текущее смещение в буфере приёма*/

        for (int i = 0; i < size; i++) {

            int s = i * rows_per_proc + (i < extra_rows ? i : extra_rows); /*Начальная строка процесса i*/
            int e = s + rows_per_proc + (i < extra_rows ? 1 : 0); /*Конечная строка процесса i*/
            sendcounts[i] = (e - s) * n; /*Количество элементов от процесса i (число строк * число столбцов)*/
            displs[i] = offset; /*Смещение для данных процесса i*/
            offset += sendcounts[i]; /*Общее смещение на размер данных от процесса i*/

        }

    }
    
    MPI_Gatherv(
        local_matrix + start_row * n, /*Отправной адрес данных для отправки*/
        (end_row - start_row) * n, /*Количество отправляемых элементов*/
        MPI_DOUBLE, /*Тип отправляемых данных*/
        rank == 0 ? matrix : NULL, /*Буфер приема*/
        sendcounts, /*Массив количеств элементов от каждого процесса*/
        displs, /*Массив смещений для размещения данных*/
        MPI_DOUBLE, /*Тип принимаемых данных*/
        0, /*Ранг корневого процесса*/
        MPI_COMM_WORLD); /*Коммуникатор*/
    
    /*Запись симметричной матрицы только для корневого процесса*/
    if (rank == 0) {
        printf("Симметризованная матрица:\n");
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                printf("%8.2f ", matrix[i * n + j]);
            }
            printf("\n");
        }
    
        free(matrix);
        free(displs);
        free(sendcounts);
    }

    free(local_matrix);
    MPI_Finalize(); /*Завершение работы MPI*/

    return 0;

}