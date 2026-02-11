#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

// Структура для хранения матрицы и параметров
typedef struct {
    double** matrix;
    int n;
    int num_threads;
} MatrixData;

// Функция для чтения матрицы из файла
double** read_matrix_from_file(const char* filename, int* n, int* p) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка открытия файла");
        return NULL;
    }
    
    if (fscanf(file, "%d %d", n, p) != 2) {
        fprintf(stderr, "Ошибка чтения размерности матрицы и количества потоков\n");
        fclose(file);
        return NULL;
    }
    
    double** matrix = (double**)malloc(*n * sizeof(double*));
    for (int i = 0; i < *n; i++) {
        matrix[i] = (double*)malloc(*n * sizeof(double));
    }
    
    for (int i = 0; i < *n; i++) {
        for (int j = 0; j < *n; j++) {
            if (fscanf(file, "%lf", &matrix[i][j]) != 1) {
                fprintf(stderr, "Ошибка чтения элемента [%d][%d]\n", i, j);
                for (int k = 0; k <= i; k++) {
                    free(matrix[k]);
                }
                free(matrix);
                fclose(file);
                return NULL;
            }
        }
    }
    
    fclose(file);
    return matrix;
}

// Функция для генерации тестовой матрицы
double** generate_test_matrix(int n, int type) {
    double** matrix = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        matrix[i] = (double*)malloc(n * sizeof(double));
        for (int j = 0; j < n; j++) {
            switch(type) {
                case 0: // Случайная матрица
                    matrix[i][j] = (rand() % 100) / 10.0;
                    break;
                case 1: // Диагонально-доминирующая
                    matrix[i][j] = (i == j) ? n + i : 1.0 / (i + j + 1);
                    break;
                case 2: // Матрица Гильберта
                    matrix[i][j] = 1.0 / (i + j + 1);
                    break;
                default: // Линейная
                    matrix[i][j] = i * n + j;
            }
        }
    }
    return matrix;
}

// Функция для вывода матрицы
void print_matrix(double** matrix, int n, const char* title) {
    printf("\n%s:\n", title);
    printf("    ");
    for (int j = 0; j < n; j++) {
        printf("    col[%d] ", j);
    }
    printf("\n");
    
    for (int i = 0; i < n; i++) {
        printf("row[%d] ", i);
        for (int j = 0; j < n; j++) {
            printf("%8.2f ", matrix[i][j]);
        }
        printf("\n");
    }
}

// Функция для освобождения памяти матрицы
void free_matrix(double** matrix, int n) {
    for (int i = 0; i < n; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// Функция для создания копии матрицы
double** copy_matrix(double** source, int n) {
    double** copy = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        copy[i] = (double*)malloc(n * sizeof(double));
        for (int j = 0; j < n; j++) {
            copy[i][j] = source[i][j];
        }
    }
    return copy;
}

// Функция для транспонирования матрицы
double** transpose_matrix(double** matrix, int n) {
    double** transposed = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        transposed[i] = (double*)malloc(n * sizeof(double));
        for (int j = 0; j < n; j++) {
            transposed[i][j] = matrix[j][i];
        }
    }
    return transposed;
}

// Функция для симметризации матрицы с помощью OpenMP (a + a^T)/2
void symmetrize_matrix_omp(double** matrix, int n, int num_threads) {
    omp_set_num_threads(num_threads);
    
    printf("Симметризация матрицы с использованием %d потоков\n", num_threads);
    printf("Формула: (a + a^T)/2\n");
    
    double start_time = omp_get_wtime();
    
    // Параллельная симметризация
    // Оптимизация: обрабатываем только верхний треугольник
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < n; i++) {
        int thread_id = omp_get_thread_num();
        
        for (int j = i; j < n; j++) {
            // Вычисляем среднее арифметическое
            double avg = (matrix[i][j] + matrix[j][i]) / 2.0;
            
            // Присваиваем обоим элементам
            matrix[i][j] = avg;
            matrix[j][i] = avg;
            
            #pragma omp critical
            {
                // Для отладки: показываем только первые несколько элементов
                if (i < 3 && j < 3 && i != j) {
                    printf("Поток %d: обновлен элемент [%d][%d] = %.2f\n", 
                           thread_id, i, j, avg);
                }
            }
        }
    }
    
    double end_time = omp_get_wtime();
    printf("Время симметризации: %.6f секунд\n", end_time - start_time);
}

// Функция для проверки симметричности матрицы
int is_symmetric(double** matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (fabs(matrix[i][j] - matrix[j][i]) > 1e-9) {
                return 0;
            }
        }
    }
    return 1;
}

// Функция для вычисления нормы разницы с симметричной частью
double compute_symmetry_error(double** matrix, int n) {
    double max_error = 0.0;
    
    #pragma omp parallel for reduction(max:max_error)
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double error = fabs(matrix[i][j] - matrix[j][i]);
            if (error > max_error) {
                max_error = error;
            }
        }
    }
    
    return max_error;
}

// Функция для сравнения производительности при разном количестве потоков
void benchmark_symmetrization(double** matrix, int n, int max_threads) {
    printf("\n========================================\n");
    printf("БЕНЧМАРК: Симметризация матрицы %dx%d\n", n, n);
    printf("========================================\n");
    
    for (int num_threads = 1; num_threads <= max_threads; num_threads++) {
        // Создаем копию матрицы для каждого теста
        double** test_matrix = copy_matrix(matrix, n);
        
        omp_set_num_threads(num_threads);
        double start = omp_get_wtime();
        
        symmetrize_matrix_omp(test_matrix, n, num_threads);
        
        double end = omp_get_wtime();
        double error = compute_symmetry_error(test_matrix, n);
        
        printf("Потоков: %d, Время: %.6f с, Ошибка симметрии: %.6e\n", 
               num_threads, end - start, error);
        
        free_matrix(test_matrix, n);
    }
}

int main(int argc, char* argv[]) {
    double** matrix = NULL;
    int n, p;
    
    // Инициализируем генератор случайных чисел
    srand(42);
    
    printf("========================================\n");
    printf("ПРОГРАММА СИММЕТРИЗАЦИИ МАТРИЦЫ С OPENMP\n");
    printf("========================================\n");
    
    // Проверяем аргументы командной строки
    if (argc != 2) {
        printf("Использование: %s <файл_с_матрицей>\n", argv[0]);
        printf("Или запуск без аргументов для демонстрации\n");
        printf("\nФормат файла: <размерность> <количество_потоков> <элементы_матрицы>\n");
        printf("\nПример файла (matrix.txt):\n");
        printf("4 4\n");
        printf("1.0 2.0 3.0 4.0\n");
        printf("5.0 6.0 7.0 8.0\n");
        printf("9.0 1.0 2.0 3.0\n");
        printf("4.0 5.0 6.0 7.0\n");
        
        printf("\nДемонстрация с тестовой матрицей:\n");
        n = 5;
        p = 4;
        matrix = generate_test_matrix(n, 0); // Случайная матрица
    } else {
        // Читаем матрицу из файла
        matrix = read_matrix_from_file(argv[1], &n, &p);
        if (!matrix) {
            fprintf(stderr, "Не удалось прочитать матрицу из файла\n");
            return 1;
        }
    }
    
    // Выводим исходную матрицу
    print_matrix(matrix, n, "Исходная матрица");
    
    // Проверяем, является ли исходная матрица симметричной
    if (is_symmetric(matrix, n)) {
        printf("\nИсходная матрица уже симметрична.\n");
    } else {
        printf("\nИсходная матрица НЕ симметрична.\n");
        printf("Максимальная ошибка симметрии: %.6e\n", 
               compute_symmetry_error(matrix, n));
    }
    
    printf("\n----------------------------------------\n");
    
    // Симметризуем матрицу
    symmetrize_matrix_omp(matrix, n, p);
    
    // Выводим результат
    print_matrix(matrix, n, "Симметризованная матрица (a + a^T)/2");
    
    // Проверяем результат
    if (is_symmetric(matrix, n)) {
        printf("\nРезультат: Матрица успешно симметризована!\n");
        printf("   Максимальная ошибка симметрии: %.6e\n", 
               compute_symmetry_error(matrix, n));
    } else {
        printf("\nОшибка: Матрица не полностью симметрична!\n");
    }
    
    // Бенчмарк для демонстрации масштабирования
    if (n <= 1000) { // Для больших матриц пропускаем бенчмарк
        benchmark_symmetrization(matrix, n, p);
    }
    
    // Освобождаем память
    if (matrix) {
        free_matrix(matrix, n);
    }
    
    return 0;
}