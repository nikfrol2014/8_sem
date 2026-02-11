#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

// Структура для передачи данных в поток
typedef struct {
    double **matrix;     // Указатель на матрицу
    int n;               // Размер матрицы
    int thread_id;       // ID потока (от 0 до p-1)
    int total_threads;   // Общее количество потоков
    int result;          // Результат проверки (1 - симметрична, 0 - нет)
} ThreadData;

// Функция потока для проверки симметричности части матрицы
void* check_symmetry(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int n = data->n;
    int thread_id = data->thread_id;
    int total_threads = data->total_threads;
    double **matrix = data->matrix;
    
    // Изначально предполагаем, что часть матрицы симметрична
    data->result = 1;
    
    // Распределяем работу между потоками
    // Каждый поток проверяет определенные строки
    for (int i = thread_id; i < n; i += total_threads) {
        // Проверяем только элементы выше главной диагонали
        // (нижние будут проверены другими потоками)
        for (int j = i + 1; j < n; j++) {
            // Сравниваем с заданной точностью (для вещественных чисел)
            if (fabs(matrix[i][j] - matrix[j][i]) > 1e-9) {
                data->result = 0;
                printf("Поток %d: Найдена несимметричность: "
                       "a[%d][%d] = %.6f != a[%d][%d] = %.6f\n",
                       thread_id, i, j, matrix[i][j], j, i, matrix[j][i]);
                pthread_exit(NULL);
            }
        }
    }
    
    printf("Поток %d: Проверенная часть матрицы симметрична\n", thread_id);
    pthread_exit(NULL);
}

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

// Основная функция
int main(int argc, char* argv[]) {
    double** matrix = NULL;
    int n, p;
    
    // Проверяем аргументы командной строки
    if (argc != 2) {
        printf("Использование: %s <файл_с_матрицей>\n", argv[0]);
        printf("Формат файла: <размерность> <количество_потоков> <элементы_матрицы>\n");
        printf("Пример файла для матрицы 3x3 и 2 потоков:\n");
        printf("3 2\n");
        printf("1.0 2.0 3.0\n");
        printf("2.0 5.0 6.0\n");
        printf("3.0 6.0 9.0\n");
        
        // Используем тестовую матрицу
        printf("\nИспользуем тестовую матрицу 4x4\n");
        n = 4;
        p = 2;
        matrix = generate_test_matrix(n, 1); // Симметричная матрица
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
    
    printf("\nПроверка симметричности матрицы %dx%d с использованием %d потоков\n", 
           n, n, p);
    
    // Создаем потоки
    pthread_t* threads = (pthread_t*)malloc(p * sizeof(pthread_t));
    ThreadData* thread_data = (ThreadData*)malloc(p * sizeof(ThreadData));
    
    if (!threads || !thread_data) {
        perror("Ошибка выделения памяти");
        return 1;
    }
    
    // Инициализируем данные для потоков
    for (int i = 0; i < p; i++) {
        thread_data[i].matrix = matrix;
        thread_data[i].n = n;
        thread_data[i].thread_id = i;
        thread_data[i].total_threads = p;
        thread_data[i].result = 1; // Изначально предполагаем симметричность
    }
    
    // Создаем потоки
    for (int i = 0; i < p; i++) {
        if (pthread_create(&threads[i], NULL, check_symmetry, &thread_data[i]) != 0) {
            perror("Ошибка создания потока");
            free(threads);
            free(thread_data);
            return 1;
        }
    }
    
    // Ожидаем завершения всех потоков
    int is_symmetric = 1;
    for (int i = 0; i < p; i++) {
        pthread_join(threads[i], NULL);
        if (thread_data[i].result == 0) {
            is_symmetric = 0;
        }
    }
    
    // Выводим результат
    printf("\nРезультат проверки: ");
    if (is_symmetric) {
        printf("Матрица СИММЕТРИЧНА\n");
    } else {
        printf("Матрица НЕСИММЕТРИЧНА\n");
    }
    
    // Освобождаем память
    for (int i = 0; i < n; i++) {
        free(matrix[i]);
    }
    free(matrix);
    free(threads);
    free(thread_data);
    
    return is_symmetric ? 1 : 0;
}