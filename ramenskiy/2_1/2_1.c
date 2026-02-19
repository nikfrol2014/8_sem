#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

// Функция для чтения матрицы из файла
double** read_matrix_from_file(const char* filename, int* n, int* p) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка открытия файла");
        return NULL;
    }
    
    // Читаем размерность и количество потоков
    if (fscanf(file, "%d %d", n, p) != 2) {
        fprintf(stderr, "Ошибка чтения размерности матрицы и количества потоков\n");
        fclose(file);
        return NULL;
    }
    
    // Выделяем память под матрицу
    double** matrix = (double**)malloc(*n * sizeof(double*));
    for (int i = 0; i < *n; i++) {
        matrix[i] = (double*)malloc(*n * sizeof(double));
    }
    
    // Читаем элементы матрицы
    for (int i = 0; i < *n; i++) {
        for (int j = 0; j < *n; j++) {
            if (fscanf(file, "%lf", &matrix[i][j]) != 1) {
                fprintf(stderr, "Ошибка чтения элемента [%d][%d]\n", i, j);
                // Освобождаем память
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
double** generate_test_matrix(int n, int is_symmetric) {
    double** matrix = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        matrix[i] = (double*)malloc(n * sizeof(double));
        for (int j = 0; j < n; j++) {
            if (is_symmetric) {
                // Симметричная матрица: a[i][j] = i + j
                matrix[i][j] = i + j;
            } else {
                // Несимметричная матрица: a[i][j] = i * n + j
                matrix[i][j] = i * n + j;
            }
        }
    }
    return matrix;
}

// Функция для вывода матрицы
void print_matrix(double** matrix, int n) {
    printf("Матрица %dx%d:\n", n, n);
    for (int i = 0; i < n; i++) {
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

// Функция проверки симметричности с использованием OpenMP
int check_symmetric_omp(double** matrix, int n, int num_threads) {
    int is_symmetric = 1;  // Предполагаем, что матрица симметрична
    
    // Устанавливаем количество потоков
    omp_set_num_threads(num_threads);
    
    // Параллельная проверка
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int total_threads = omp_get_num_threads();
        
        // Равномерно распределяем строки между потоками
        for (int i = thread_id; i < n; i += total_threads) {
            for (int j = i + 1; j < n; j++) {
                // Сравниваем с плавающей точкой
                if (fabs(matrix[i][j] - matrix[j][i]) > 1e-9) {
                    #pragma omp critical
                    {
                        if (is_symmetric) {
                            printf("Поток %d: Найдена несимметричность: "
                                   "a[%d][%d] = %.6f != a[%d][%d] = %.6f\n",
                                   thread_id, i, j, matrix[i][j], j, i, matrix[j][i]);
                            is_symmetric = 0;
                        }
                    }
                    // Можно досрочно выйти, но в OpenMP нет прямого break для параллельных циклов
                }
            }
        }
    }
    
    return is_symmetric;
}

// Альтернативная версия с использованием reduction и shared переменной
int check_symmetric_omp_reduction(double** matrix, int n, int num_threads) {
    int is_symmetric = 1;
    omp_set_num_threads(num_threads);
    
    // Используем reduction для безопасного обновления флага
    #pragma omp parallel for reduction(&&:is_symmetric)
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (fabs(matrix[i][j] - matrix[j][i]) > 1e-9) {
                is_symmetric = 0;
            }
        }
    }
    
    return is_symmetric;
}

int main(int argc, char* argv[]) {
    double** matrix = NULL;
    int n, p;
    double start_time, end_time;
    
    // Проверяем аргументы командной строки
    if (argc != 2) {
        printf("Использование: %s <файл_с_матрицей>\n", argv[0]);
        printf("Формат файла: <размерность> <количество_потоков> <элементы_матрицы>\n");
        printf("\nПример файла для матрицы 4x4 и 2 потоков (matrix.txt):\n");
        printf("4 2\n");
        printf("1.0 2.0 3.0 4.0\n");
        printf("2.0 5.0 6.0 7.0\n");
        printf("3.0 6.0 8.0 9.0\n");
        printf("4.0 7.0 9.0 10.0\n");
        
        // Используем тестовую матрицу
        printf("\nИспользуем тестовую матрицу 4x4\n");
        n = 4;
        p = 4;  // 4 потока
        matrix = generate_test_matrix(n, 1);  // Симметричная матрица
        print_matrix(matrix, n);
    } else {
        // Читаем матрицу из файла
        matrix = read_matrix_from_file(argv[1], &n, &p);
        if (!matrix) {
            fprintf(stderr, "Не удалось прочитать матрицу из файла\n");
            return 1;
        }
        print_matrix(matrix, n);
    }
    
    printf("\n========================================\n");
    printf("Проверка симметричности матрицы %dx%d\n", n, n);
    printf("Количество потоков: %d\n", p);
    printf("========================================\n");
    
    // Измеряем время выполнения
    start_time = omp_get_wtime();
    
    // Вызываем функцию проверки
    int result = check_symmetric_omp(matrix, n, p);
    
    end_time = omp_get_wtime();
    
    // Выводим результат
    printf("\nРезультат: ");
    if (result) {
        printf("Матрица СИММЕТРИЧНА\n");
    } else {
        printf("Матрица НЕСИММЕТРИЧНА\n");
    }
    printf("Время выполнения: %.6f секунд\n", end_time - start_time);
    
    // Освобождаем память
    if (matrix) {
        free_matrix(matrix, n);
    }
    
    return result ? 0 : 1;
}