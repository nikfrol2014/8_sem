#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// Структура для передачи данных в поток
typedef struct {
    char* filename;      // Имя файла для обработки
    int index;           // Индекс потока
    double* shared_sum;  // Указатель на разделяемую сумму
    sem_t* semaphore;    // Указатель на семафор
} thread_data_t;

// Функция для обработки одного файла в потоке
void* process_file(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    FILE* file = NULL;
    double sum = 0.0;
    double num;
    
    // Открываем файл для чтения
    file = fopen(data->filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Поток %d: Не удалось открыть файл %s\n", 
                data->index, data->filename);
        pthread_exit(NULL);
    }
    
    // Читаем и суммируем числа из файла
    while (fscanf(file, "%lf", &num) == 1) {
        sum += num;
    }
    
    fclose(file);
    
    printf("Поток %d: Сумма в файле %s = %.2f\n", 
           data->index, data->filename, sum);
    
    // Захватываем семафор для доступа к разделяемой памяти
    sem_wait(data->semaphore);
    
    // Добавляем локальную сумму к общей
    *(data->shared_sum) += sum;
    
    printf("Поток %d: Общая сумма после добавления = %.2f\n", 
           data->index, *(data->shared_sum));
    
    // Освобождаем семафор
    sem_post(data->semaphore);
    
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s файл1 [файл2 ... файлN]\n", argv[0]);
        return 1;
    }
    
    int num_files = argc - 1;
    pthread_t* threads = malloc(num_files * sizeof(pthread_t));
    thread_data_t* thread_data = malloc(num_files * sizeof(thread_data_t));
    
    if (!threads || !thread_data) {
        perror("Ошибка выделения памяти");
        return 1;
    }
    
    // Создаем разделяемую память для общей суммы
    double* shared_sum = mmap(NULL, sizeof(double), 
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_sum == MAP_FAILED) {
        perror("Ошибка создания разделяемой памяти");
        free(threads);
        free(thread_data);
        return 1;
    }
    
    // Инициализируем общую сумму
    *shared_sum = 0.0;
    
    // Создаем и инициализируем семафор
    sem_t semaphore;
    if (sem_init(&semaphore, 0, 1) != 0) {
        perror("Ошибка инициализации семафора");
        munmap(shared_sum, sizeof(double));
        free(threads);
        free(thread_data);
        return 1;
    }
    
    // Создаем потоки для обработки файлов
    for (int i = 0; i < num_files; i++) {
        thread_data[i].filename = argv[i + 1];
        thread_data[i].index = i;
        thread_data[i].shared_sum = shared_sum;
        thread_data[i].semaphore = &semaphore;
        
        if (pthread_create(&threads[i], NULL, process_file, &thread_data[i]) != 0) {
            perror("Ошибка создания потока");
            // Очищаем ресурсы
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            sem_destroy(&semaphore);
            munmap(shared_sum, sizeof(double));
            free(threads);
            free(thread_data);
            return 1;
        }
    }
    
    // Ожидаем завершения всех потоков
    for (int i = 0; i < num_files; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Записываем результат в файл res.txt
    FILE* output = fopen("res.txt", "w");
    if (output == NULL) {
        perror("Ошибка открытия файла res.txt");
        sem_destroy(&semaphore);
        munmap(shared_sum, sizeof(double));
        free(threads);
        free(thread_data);
        return 1;
    }
    
    fprintf(output, "%.2f\n", *shared_sum);
    fclose(output);
    
    printf("Общая сумма записана в файл res.txt: %.2f\n", *shared_sum);
    
    // Очищаем ресурсы
    sem_destroy(&semaphore);
    munmap(shared_sum, sizeof(double));
    free(threads);
    free(thread_data);
    
    return 0;
}