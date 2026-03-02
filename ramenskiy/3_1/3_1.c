#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// Чтение матрицы из файла (только процесс 0)
double* read_matrix(const char* filename, int* n) {
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;
    fscanf(f, "%d", n);
    double* a = (double*)malloc((*n) * (*n) * sizeof(double));
    for (int i = 0; i < (*n) * (*n); ++i)
        fscanf(f, "%lf", &a[i]);
    fclose(f);
    return a;
}

// Вывод матрицы (процесс 0)
void print_matrix(double* a, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j)
            printf("%8.2f ", a[i * n + j]);
        printf("\n");
    }
}

// Функция проверки симметричности (вызывается всеми процессами)
int check_symmetry(double* local_rows, int n, int rank, int size,
                   int* counts, int* displs) {
    // Собираем всю матрицу на процессе 0
    double* full_matrix = NULL;
    if (rank == 0) {
        full_matrix = (double*)malloc(n * n * sizeof(double));
    }
    MPI_Gatherv(local_rows, counts[rank] * n, MPI_DOUBLE,
                full_matrix, counts, displs, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    int symmetric = 1;
    if (rank == 0) {
        for (int i = 0; i < n && symmetric; ++i) {
            for (int j = i + 1; j < n && symmetric; ++j) {
                if (fabs(full_matrix[i * n + j] - full_matrix[j * n + i]) > 1e-10) {
                    symmetric = 0;
                }
            }
        }
        free(full_matrix);
    }

    // Рассылаем результат всем процессам
    MPI_Bcast(&symmetric, 1, MPI_INT, 0, MPI_COMM_WORLD);
    return symmetric;
}

int main(int argc, char* argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 2) {
        if (rank == 0) fprintf(stderr, "Использование: %s <файл_с_матрицей>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    double* full_matrix = NULL;
    int n = 0;

    // Процесс 0 читает матрицу из файла
    if (rank == 0) {
        full_matrix = read_matrix(argv[1], &n);
        if (!full_matrix) MPI_Abort(MPI_COMM_WORLD, 1);
        printf("Исходная матрица %d x %d:\n", n, n);
        print_matrix(full_matrix, n);
    }

    // Рассылаем размерность матрицы всем
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Распределение строк по процессам (равномерно)
    int* counts = (int*)malloc(size * sizeof(int));
    int* displs = (int*)malloc(size * sizeof(int));
    int rows_per_proc_base = n / size;
    int remainder = n % size;
    displs[0] = 0;
    for (int i = 0; i < size; ++i) {
        counts[i] = rows_per_proc_base + (i < remainder ? 1 : 0);
        if (i > 0) displs[i] = displs[i - 1] + counts[i - 1];
    }

    // Каждый процесс выделяет память под свои строки
    double* local_rows = (double*)malloc(counts[rank] * n * sizeof(double));

    // Рассылаем строки матрицы по процессам
    MPI_Scatterv(full_matrix, counts, displs, MPI_DOUBLE,
                 local_rows, counts[rank] * n, MPI_DOUBLE,
                 0, MPI_COMM_WORLD);

    // Вызываем функцию проверки
    int is_symmetric = check_symmetry(local_rows, n, rank, size, counts, displs);

    // Процесс 0 выводит результат
    if (rank == 0) {
        printf("\nРезультат: %s\n", is_symmetric ? "симметрична" : "НЕ симметрична");
    }

    // Освобождение памяти
    free(local_rows);
    free(counts);
    free(displs);
    if (rank == 0) free(full_matrix);

    MPI_Finalize();
    return 0;
}