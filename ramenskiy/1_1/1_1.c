#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define SEM_NAME "/sum_semaphore"
#define SHM_NAME "/sum_shared_memory"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s файл1 [файл2 ... файлN]\n", argv[0]);
        fprintf(stderr, "Пример: %s data1.txt data2.txt data3.txt\n", argv[0]);
        return 1;
    }
    
    int num_files = argc - 1;
    pid_t pids[num_files];
    
    // Удаляем старые объекты, если они существуют
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);
    
    // 1. Создаем разделяемую память для общей суммы
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Ошибка создания разделяемой памяти");
        return 1;
    }
    
    // Устанавливаем размер
    if (ftruncate(shm_fd, sizeof(double)) == -1) {
        perror("Ошибка установки размера");
        shm_unlink(SHM_NAME);
        return 1;
    }
    
    // Отображаем в память
    double *shared_sum = mmap(NULL, sizeof(double), 
                              PROT_READ | PROT_WRITE, 
                              MAP_SHARED, shm_fd, 0);
    if (shared_sum == MAP_FAILED) {
        perror("Ошибка отображения памяти");
        shm_unlink(SHM_NAME);
        return 1;
    }
    
    // Инициализируем общую сумму
    *shared_sum = 0.0;
    
    // 2. Создаем именованный семафор
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("Ошибка создания семафора");
        munmap(shared_sum, sizeof(double));
        shm_unlink(SHM_NAME);
        return 1;
    }
    
    printf("Родительский процесс PID=%d\n", getpid());
    printf("Количество файлов для обработки: %d\n", num_files);
    printf("========================================\n");
    
    // 3. Создаем дочерние процессы
    for (int i = 0; i < num_files; i++) {
        pid_t pid = fork();
        
        if (pid == -1) {
            perror("Ошибка fork");
            // Завершаем уже созданные процессы
            for (int j = 0; j < i; j++) {
                kill(pids[j], SIGTERM);
            }
            exit(1);
        }
        
        if (pid == 0) {
            // ДОЧЕРНИЙ ПРОЦЕСС
            printf("Дочерний процесс %d (PID=%d) обрабатывает файл: %s\n", 
                   i, getpid(), argv[i + 1]);
            
            FILE *file = fopen(argv[i + 1], "r");
            if (file == NULL) {
                fprintf(stderr, "Процесс %d: Не удалось открыть файл %s\n", 
                        i, argv[i + 1]);
                exit(1);
            }
            
            double local_sum = 0.0;
            double num;
            int count = 0;
            
            // Читаем числа из файла
            while (fscanf(file, "%lf", &num) == 1) {
                local_sum += num;
                count++;
            }
            fclose(file);
            
            printf("Процесс %d: В файле %s найдено %d чисел, сумма = %.2f\n", 
                   i, argv[i + 1], count, local_sum);
            
            // Критическая секция - доступ к разделяемой памяти
            sem_wait(sem);
            
            *shared_sum += local_sum;
            printf("Процесс %d: Общая сумма после добавления = %.2f\n", 
                   i, *shared_sum);
            
            sem_post(sem);
            
            printf("Процесс %d завершил работу\n", i);
            exit(0);
        } else {
            // РОДИТЕЛЬСКИЙ ПРОЦЕСС
            pids[i] = pid;
        }
    }
    
    // 4. Ожидаем завершения всех дочерних процессов
    printf("========================================\n");
    printf("Ожидание завершения дочерних процессов...\n");
    
    int status;
    pid_t finished_pid;
    for (int i = 0; i < num_files; i++) {
        finished_pid = waitpid(pids[i], &status, 0);
        if (finished_pid == -1) {
            perror("Ошибка waitpid");
        } else {
            printf("Процесс %d (PID=%d) завершен\n", i, finished_pid);
        }
    }
    
    // 5. Записываем результат в файл res.txt
    FILE *output = fopen("res.txt", "w");
    if (output == NULL) {
        perror("Ошибка открытия файла res.txt");
    } else {
        fprintf(output, "%.2f\n", *shared_sum);
        fclose(output);
        printf("\nОбщая сумма = %.2f записана в файл res.txt\n", *shared_sum);
    }
    
    // 6. Очищаем ресурсы
    printf("Очистка ресурсов...\n");
    
    if (munmap(shared_sum, sizeof(double)) == -1) {
        perror("Ошибка munmap");
    }
    
    close(shm_fd);
    
    if (shm_unlink(SHM_NAME) == -1) {
        perror("Ошибка shm_unlink");
    }
    
    sem_close(sem);
    
    if (sem_unlink(SEM_NAME) == -1) {
        perror("Ошибка sem_unlink");
    }
    
    printf("Программа завершена\n");
    return 0;
}