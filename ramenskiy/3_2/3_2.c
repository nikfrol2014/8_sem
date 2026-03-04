#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// Функция для симметризации матрицы (упрощенная версия)
void symmetrize_matrix(double* local_block, int n, int rank, int size, int local_rows, int start_row) {
    // Проходим по всем элементам в локальном блоке
    for (int i = start_row; i < start_row + local_rows; i++) {
        int local_i = i - start_row;
        
        for (int j = 0; j < n; j++) {
            if (i == j) continue; // Пропускаем диагональ
            
            double aij = local_block[local_i * n + j];
            
            // Определяем процесс для строки j
            int target_proc = j % size;
            
            if (target_proc == rank) {
                // Элемент в том же процессе
                int local_j = j - start_row;
                if (local_j >= 0 && local_j < local_rows) {
                    double aji = local_block[local_j * n + i];
                    double sym = (aij + aji) / 2.0;
                    local_block[local_i * n + j] = sym;
                    local_block[local_j * n + i] = sym;
                }
            }
        }
    }
}

// Функция для обмена данными между процессами
void exchange_data(double* local_block, int n, int rank, int size, int local_rows, int start_row) {
    MPI_Status status;
    
    // Обмениваемся данными со всеми процессами
    for (int target = 0; target < size; target++) {
        if (target == rank) continue;
        
        // Отправляем свою первую строку целевому процессу
        if (local_rows > 0) {
            MPI_Send(local_block, n, MPI_DOUBLE, target, 0, MPI_COMM_WORLD);
        }
        
        // Получаем строку от целевого процесса
        double* recv_buffer = (double*)malloc(n * sizeof(double));
        MPI_Recv(recv_buffer, n, MPI_DOUBLE, target, 0, MPI_COMM_WORLD, &status);
        
        // Обновляем свои элементы, используя полученные данные
        for (int j = 0; j < n; j++) {
            if (j >= start_row && j < start_row + local_rows) {
                int local_j = j - start_row;
                double aij = local_block[local_j * n + target];
                double aji = recv_buffer[target];
                double sym = (aij + aji) / 2.0;
                local_block[local_j * n + target] = sym;
            }
        }
        
        free(recv_buffer);
    }
}

// Чтение матрицы из файла
double* read_matrix(const char* filename, int* n) {
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;
    
    fscanf(f, "%d", n);
    double* m = (double*)malloc((*n) * (*n) * sizeof(double));
    
    for (int i = 0; i < (*n) * (*n); i++) {
        fscanf(f, "%lf", &m[i]);
    }
    
    fclose(f);
    return m;
}

int main(int argc, char* argv[]) {
    int rank, size;
    int n;
    double* full_matrix = NULL;
    double* local_block = NULL;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (argc != 2) {
        if (rank == 0) 
            printf("Использование: %s <файл>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }
    
    // Процесс 0 читает матрицу
    if (rank == 0) {
        full_matrix = read_matrix(argv[1], &n);
        if (!full_matrix) {
            printf("Ошибка чтения файла\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        printf("Исходная матрица %dx%d:\n", n, n);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                printf("%7.2f ", full_matrix[i*n + j]);
            }
            printf("\n");
        }
        printf("\n");
    }
    
    // Рассылаем размер
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Простое распределение: каждый процесс получает одну строку
    // (если процессов больше чем строк, лишние процессы ничего не делают)
    int local_rows = 0;
    int start_row = -1;
    
    if (rank < n) {
        local_rows = 1;
        start_row = rank;
    }
    
    printf("Процесс %d: строк=%d, начало=%d\n", rank, local_rows, start_row);
    
    // Выделяем память
    if (local_rows > 0) {
        local_block = (double*)malloc(local_rows * n * sizeof(double));
    }
    
    // Рассылаем данные построчно
    if (rank == 0) {
        // Процесс 0 оставляет себе строку 0
        if (0 < n) {
            for (int j = 0; j < n; j++) {
                local_block[j] = full_matrix[0 * n + j];
            }
        }
        
        // Отправляем строки другим процессам
        for (int i = 1; i < n && i < size; i++) {
            MPI_Send(&full_matrix[i * n], n, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        }
    } else {
        if (local_rows > 0) {
            MPI_Recv(local_block, n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Симметризация (только для процессов с данными)
    if (local_rows > 0) {
        symmetrize_matrix(local_block, n, rank, size, local_rows, start_row);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Обмен данными между процессами
    if (local_rows > 0) {
        exchange_data(local_block, n, rank, size, local_rows, start_row);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Сбор результатов
    if (rank == 0) {
        printf("Симметризованная матрица:\n");
        
        // Выводим строку 0
        for (int j = 0; j < n; j++) {
            printf("%7.2f ", local_block[j]);
        }
        printf("\n");
        
        // Получаем и выводим строки от других процессов
        for (int i = 1; i < n && i < size; i++) {
            double* row = (double*)malloc(n * sizeof(double));
            MPI_Recv(row, n, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            for (int j = 0; j < n; j++) {
                printf("%7.2f ", row[j]);
            }
            printf("\n");
            
            free(row);
        }
        
        // Если есть строки, которые не были обработаны (при size < n)
        for (int i = size; i < n; i++) {
            printf("Строка %d не была обработана (недостаточно процессов)\n", i);
        }
        
    } else {
        if (local_rows > 0) {
            MPI_Send(local_block, n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
    }
    
    // Очистка
    if (local_rows > 0) free(local_block);
    if (rank == 0) free(full_matrix);
    
    MPI_Finalize();
    return 0;
}