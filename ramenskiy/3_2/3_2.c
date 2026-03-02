#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// Функция для чтения матрицы из файла
double* read_matrix(const char* filename, int* n) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Ошибка открытия файла %s\n", filename);
        return NULL;
    }
    
    if (fscanf(file, "%d", n) != 1) {
        fprintf(stderr, "Ошибка чтения размерности матрицы\n");
        fclose(file);
        return NULL;
    }
    
    printf("Чтение матрицы размером %d x %d\n", *n, *n);
    
    double* matrix = (double*)malloc((*n) * (*n) * sizeof(double));
    if (!matrix) {
        fclose(file);
        return NULL;
    }
    
    for (int i = 0; i < (*n) * (*n); i++) {
        if (fscanf(file, "%lf", &matrix[i]) != 1) {
            fprintf(stderr, "Ошибка чтения элемента %d\n", i);
            free(matrix);
            fclose(file);
            return NULL;
        }
    }
    
    fclose(file);
    return matrix;
}

// Функция для вывода матрицы
void print_matrix(double* matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%8.2f ", matrix[i * n + j]);
        }
        printf("\n");
    }
}

// Функция для проверки симметричности матрицы
int check_matrix_symmetry(double* matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (fabs(matrix[i * n + j] - matrix[j * n + i]) > 1e-10) {
                printf("Матрица не симметрична: a[%d][%d]=%f, a[%d][%d]=%f\n", 
                       i, j, matrix[i * n + j], j, i, matrix[j * n + i]);
                return 0;
            }
        }
    }
    return 1;
}

// MPI-подпрограмма для преобразования матрицы
void symmetrize_matrix(double* local_block, int n, int rank, int size, int* counts, int* displs) {
    // Создаем полную матрицу на каждом процессе для простоты
    // В реальных приложениях нужно более эффективное распределение
    double* full_matrix = (double*)malloc(n * n * sizeof(double));
    double* full_transpose = (double*)malloc(n * n * sizeof(double));
    
    // Собираем полную матрицу на всех процессах
    MPI_Allgatherv(local_block, counts[rank], MPI_DOUBLE,
                   full_matrix, counts, displs, MPI_DOUBLE,
                   MPI_COMM_WORLD);
    
    printf("Процесс %d получил полную матрицу\n", rank);
    
    // Транспонируем матрицу
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            full_transpose[j * n + i] = full_matrix[i * n + j];
        }
    }
    
    // Вычисляем (a + a^T)/2 для всей матрицы
    for (int i = 0; i < n * n; i++) {
        full_matrix[i] = (full_matrix[i] + full_transpose[i]) / 2.0;
    }
    
    // Копируем только локальную часть обратно
    for (int i = 0; i < counts[rank]; i++) {
        local_block[i] = full_matrix[displs[rank] + i];
    }
    
    free(full_matrix);
    free(full_transpose);
}

int main(int argc, char* argv[]) {
    int rank, size;
    double* matrix = NULL;
    double* local_block = NULL;
    int n = 0;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Подавление предупреждения о MIT-MAGIC-COOKIE (опционально)
    setenv("QT_QPA_PLATFORM", "offscreen", 0);
    
    if (argc != 2) {
        if (rank == 0) {
            fprintf(stderr, "Использование: %s <имя_файла_с_матрицей>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }
    
    // Процесс 0 читает матрицу из файла
    if (rank == 0) {
        matrix = read_matrix(argv[1], &n);
        if (!matrix) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        printf("Исходная матрица:\n");
        print_matrix(matrix, n);
        
        // Проверяем исходную матрицу на симметричность
        printf("\nПроверка исходной матрицы:\n");
        if (check_matrix_symmetry(matrix, n)) {
            printf("Исходная матрица симметрична\n");
        } else {
            printf("Исходная матрица НЕ симметрична\n");
        }
    }
    
    // Рассылаем размерность матрицы всем процессам
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Вычисляем распределение данных
    int* counts = (int*)malloc(size * sizeof(int));
    int* displs = (int*)malloc(size * sizeof(int));
    
    int elements_per_proc_base = (n * n) / size;
    int remainder = (n * n) % size;
    
    displs[0] = 0;
    for (int i = 0; i < size; i++) {
        counts[i] = elements_per_proc_base + (i < remainder ? 1 : 0);
        if (i > 0) {
            displs[i] = displs[i-1] + counts[i-1];
        }
    }
    
    // Выделяем память под локальный блок
    local_block = (double*)malloc(counts[rank] * sizeof(double));
    
    // Распределяем данные между процессами
    MPI_Scatterv(matrix, counts, displs, MPI_DOUBLE,
                 local_block, counts[rank], MPI_DOUBLE,
                 0, MPI_COMM_WORLD);
    
    // Преобразуем матрицу
    symmetrize_matrix(local_block, n, rank, size, counts, displs);
    
    // Собираем результат обратно на процесс 0
    MPI_Gatherv(local_block, counts[rank], MPI_DOUBLE,
                matrix, counts, displs, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    
    // Процесс 0 выводит результат и проверяет симметричность
    if (rank == 0) {
        printf("\nСимметризованная матрица (a + a^T)/2:\n");
        print_matrix(matrix, n);
        
        printf("\nПроверка полученной матрицы:\n");
        if (check_matrix_symmetry(matrix, n)) {
            printf("Полученная матрица симметрична\n");
        } else {
            printf("Полученная матрица НЕ симметрична - ошибка!\n");
        }
    }
    
    // Освобождаем память
    free(local_block);
    free(counts);
    free(displs);
    if (rank == 0) {
        free(matrix);
    }
    
    MPI_Finalize();
    return 0;
}