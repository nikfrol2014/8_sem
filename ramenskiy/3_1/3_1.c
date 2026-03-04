#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <unistd.h>

// Функция для проверки симметричности блока матрицы
int check_symmetry_block(double* block, int n, int k, int p, int block_size, int remainder, int local_rows, int start_row) {
    // Проверяем симметричность для элементов в блоке процесса
    for (int i = start_row; i < start_row + local_rows; i++) {
        for (int j = i; j < n; j++) { // Проверяем только верхний треугольник
            // Индекс в локальном блоке
            int local_i = i - start_row;
            double aij = block[local_i * n + j];
            
            // Определяем процесс, который хранит элемент aji
            int target_proc;
            
            // Вычисляем номер процесса для строки j
            if (j < remainder * (block_size + 1)) {
                target_proc = j / (block_size + 1);
            } else {
                target_proc = (j - remainder * (block_size + 1)) / block_size + remainder;
            }
            
            // Гарантируем, что target_proc в допустимых пределах
            if (target_proc >= p) target_proc = p - 1;
            
            // Если элемент aji находится в том же процессе
            if (target_proc == k) {
                int local_j = j - start_row;
                if (local_j >= 0 && local_j < local_rows) {
                    double aji = block[local_j * n + i];
                    if (fabs(aij - aji) > 1e-10) {
                        printf("Процесс %d: обнаружено несоответствие a[%d][%d]=%f != a[%d][%d]=%f\n", 
                               k, i, j, aij, j, i, aji);
                        return 0;
                    }
                }
            } else {
                // Отправляем запрос в процесс, который хранит aji
                double aji;
                MPI_Status status;
                
                // Отправляем запрос на получение элемента
                MPI_Send(&i, 1, MPI_INT, target_proc, 0, MPI_COMM_WORLD);
                MPI_Send(&j, 1, MPI_INT, target_proc, 1, MPI_COMM_WORLD);
                MPI_Recv(&aji, 1, MPI_DOUBLE, target_proc, 2, MPI_COMM_WORLD, &status);
                
                if (fabs(aij - aji) > 1e-10) {
                    printf("Процесс %d: обнаружено несоответствие a[%d][%d]=%f != a[%d][%d]=%f (из процесса %d)\n", 
                           k, i, j, aij, j, i, aji, target_proc);
                    return 0;
                }
            }
        }
    }
    
    return 1;
}

// Функция для обработки запросов от других процессов
void handle_requests(double* block, int n, int k, int local_rows, int start_row) {
    MPI_Status status;
    int flag;
    int received = 0;
    
    // Проверяем, есть ли входящие сообщения
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
    
    while (flag) {
        int req_i, req_j;
        
        // Получаем запрос
        MPI_Recv(&req_i, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&req_j, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
        
        // Вычисляем локальный индекс для запрошенного элемента
        // Запрос приходит для элемента a[req_j][req_i]
        int local_row = req_j - start_row;
        
        if (local_row >= 0 && local_row < local_rows) {
            // Отправляем запрошенный элемент
            double value = block[local_row * n + req_i];
            MPI_Send(&value, 1, MPI_DOUBLE, status.MPI_SOURCE, 2, MPI_COMM_WORLD);
            received++;
        }
        
        // Проверяем, есть ли еще сообщения
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
    }
    
    if (received > 0) {
        printf("Процесс %d обработал %d запросов\n", k, received);
    }
}

// Функция для чтения матрицы из файла
double* read_matrix_from_file(const char* filename, int* n) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Ошибка открытия файла %s\n", filename);
        return NULL;
    }
    
    // Читаем размерность матрицы
    if (fscanf(file, "%d", n) != 1) {
        printf("Ошибка чтения размерности матрицы\n");
        fclose(file);
        return NULL;
    }
    
    // Выделяем память под матрицу
    double* matrix = (double*)malloc((*n) * (*n) * sizeof(double));
    if (matrix == NULL) {
        printf("Ошибка выделения памяти\n");
        fclose(file);
        return NULL;
    }
    
    // Читаем данные матрицы
    for (int i = 0; i < (*n) * (*n); i++) {
        if (fscanf(file, "%lf", &matrix[i]) != 1) {
            printf("Ошибка чтения элемента матрицы %d\n", i);
            free(matrix);
            fclose(file);
            return NULL;
        }
    }
    
    fclose(file);
    return matrix;
}

int main(int argc, char* argv[]) {
    int rank, size;
    int n = 0;
    double* full_matrix = NULL;
    double* local_block = NULL;
    int result = 1;
    int local_result = 1;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (argc != 2) {
        if (rank == 0) {
            printf("Использование: %s <имя_файла_с_матрицей>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }
    
    // Процесс 0 читает матрицу из файла
    if (rank == 0) {
        full_matrix = read_matrix_from_file(argv[1], &n);
        if (full_matrix == NULL) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        printf("Размер матрицы: %d x %d\n", n, n);
        printf("Матрица:\n");
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                printf("%.1f ", full_matrix[i * n + j]);
            }
            printf("\n");
        }
    }
    
    // Рассылаем размерность матрицы всем процессам
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (n == 0) {
        if (rank == 0) {
            printf("Ошибка: нулевой размер матрицы\n");
        }
        MPI_Finalize();
        return 1;
    }
    
    // Вычисляем размер блока для каждого процесса
    int block_size = n / size;
    int remainder = n % size;
    
    // Определяем размер локального блока и начальную строку
    int local_rows;
    int start_row;
    
    if (rank < remainder) {
        local_rows = block_size + 1;
        start_row = rank * (block_size + 1);
    } else {
        local_rows = block_size;
        start_row = rank * block_size + remainder;
    }
    
    // Корректируем для последних процессов
    if (start_row >= n) {
        local_rows = 0;
        start_row = n;
    }
    
    if (start_row + local_rows > n) {
        local_rows = n - start_row;
    }
    
    printf("Процесс %d: local_rows=%d, start_row=%d\n", rank, local_rows, start_row);
    
    // Выделяем память под локальный блок
    if (local_rows > 0) {
        local_block = (double*)malloc(local_rows * n * sizeof(double));
        if (local_block == NULL) {
            printf("Процесс %d: ошибка выделения памяти\n", rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    
    // Создаем массивы для Scatterv
    int* sendcounts = (int*)malloc(size * sizeof(int));
    int* displs = (int*)malloc(size * sizeof(int));
    
    if (rank == 0) {
        int offset = 0;
        for (int i = 0; i < size; i++) {
            int rows;
            if (i < remainder) {
                rows = block_size + 1;
            } else {
                rows = block_size;
            }
            
            // Корректируем для последних процессов
            if (offset + rows * n > n * n) {
                rows = (n * n - offset) / n;
            }
            
            sendcounts[i] = (rows > 0) ? rows * n : 0;
            displs[i] = offset;
            offset += sendcounts[i];
            
            printf("Процесс %d: rows=%d, offset=%d, count=%d\n", i, rows, displs[i], sendcounts[i]);
        }
    }
    
    // Распределяем данные
    MPI_Scatterv(full_matrix, sendcounts, displs, MPI_DOUBLE,
                 local_block, local_rows * n, MPI_DOUBLE,
                 0, MPI_COMM_WORLD);
    
    // Барьер для синхронизации
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Каждый процесс проверяет свою часть
    if (local_rows > 0) {
        local_result = check_symmetry_block(local_block, n, rank, size, block_size, remainder, local_rows, start_row);
    }
    
    // Обрабатываем запросы от других процессов
    // Важно: все процессы должны обрабатывать запросы, даже если у них нет данных
    for (int iter = 0; iter < size * 2; iter++) {
        handle_requests(local_block, n, rank, local_rows, start_row);
        
        // Небольшая задержка, чтобы дать время другим процессам отправить запросы
        usleep(1000);
    }
    
    // Собираем результаты
    MPI_Allreduce(&local_result, &result, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    
    // Выводим результат
    if (rank == 0) {
        if (result) {
            printf("Матрица симметрична.\n");
        } else {
            printf("Матрица НЕ симметрична.\n");
        }
    }
    
    // Освобождаем память
    if (local_rows > 0) {
        free(local_block);
    }
    free(sendcounts);
    free(displs);
    if (rank == 0) {
        free(full_matrix);
    }
    
    MPI_Finalize();
    return 0;
}