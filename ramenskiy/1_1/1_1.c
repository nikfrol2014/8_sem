#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>


#define SEM_NAME "/sum_semaphore"
#define SHM_NAME "/sum_shared_memory"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1 [file2 ...]\n", argv[0]);
        return 1;
    }
    
    int num_files = argc - 1;
    pid_t pids[num_files];
    
    // Создаем разделяемую память
    int shm_fd = shm_open(SHM_NAME,O_CREAT|O_RDWR,0666);
    ftruncate(shm_fd, sizeof(double));
    double *shared_sum = mmap(NULL, sizeof(double), 
                              PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    *shared_sum = 0.0;
    
    // Создаем именованный семафор
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    
    // Создаем процессы
    for (int i = 0; i < num_files; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Дочерний процесс
            FILE *file = fopen(argv[i + 1], "r");
            double local_sum = 0.0;
            double num;
            
            while (fscanf(file, "%lf", &num) == 1) {
                local_sum += num;
            }
            fclose(file);
            
            sem_wait(sem);
            *shared_sum += local_sum;
            sem_post(sem);
            
            exit(0);
        } else if (pid > 0) {
            pids[i] = pid;
        } else {
            perror("fork failed");
            exit(1);
        }
    }
    
    // Ожидаем завершения всех дочерних процессов
    for (int i = 0; i < num_files; i++) {
        waitpid(pids[i], NULL, 0);
    }
    
    // Записываем результат
    FILE *output = fopen("res.txt", "w");
    fprintf(output, "%.2f\n", *shared_sum);
    fclose(output);
    
    // Очистка
    munmap(shared_sum, sizeof(double));
    close(shm_fd);
    shm_unlink(SHM_NAME);
    sem_close(sem);
    sem_unlink(SEM_NAME);
    
    return 0;
}