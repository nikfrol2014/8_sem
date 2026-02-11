#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>

// Константы
#define SHM_NAME "/shared_stack"
#define SEM_NAME "/stack_semaphore"
#define MAX_STACK_SIZE 100
#define MAX_STRING_LENGTH 256

// Структура элемента стека
typedef struct {
    char data[MAX_STRING_LENGTH];
} StackItem;

// Структура разделяемого стека
typedef struct {
    int top;                      // Индекс верхнего элемента
    StackItem items[MAX_STACK_SIZE]; // Массив элементов
} SharedStack;

// Функция для отображения меню
void print_menu() {
    printf("\n=== Меню работы со стеком ===\n");
    printf("1. Добавить строку в стек\n");
    printf("2. Удалить строку из стека\n");
    printf("3. Просмотреть содержимое стека\n");
    printf("4. Очистить стек\n");
    printf("5. Завершить работу\n");
    printf("Выберите действие: ");
}

// Основная программа
int main(int argc, char *argv[]) {
    int shm_fd;
    SharedStack *stack;
    sem_t *sem;
    
    printf("Программа работы со стеком в разделяемой памяти\n");
    printf("PID процесса: %d\n", getpid());
    
    // 1. Создаем или открываем разделяемую память
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Ошибка при создании/открытии разделяемой памяти");
        exit(1);
    }
    
    // Устанавливаем размер разделяемой памяти
    if (ftruncate(shm_fd, sizeof(SharedStack)) == -1) {
        perror("Ошибка при установке размера разделяемой памяти");
        exit(1);
    }
    
    // Отображаем разделяемую память в адресное пространство процесса
    stack = mmap(NULL, sizeof(SharedStack), 
                 PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (stack == MAP_FAILED) {
        perror("Ошибка при отображении разделяемой памяти");
        exit(1);
    }
    
    // 2. Создаем или открываем именованный семафор
    // Семафор используется для синхронизации доступа к стеку
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("Ошибка при создании/открытии семафора");
        exit(1);
    }
    
    // Инициализируем стек, если он пустой (первый процесс)
    sem_wait(sem); // Захватываем семафор для безопасной инициализации
    if (stack->top == 0) {
        // Проверяем, действительно ли стек пуст (может содержать мусор)
        stack->top = -1; // -1 означает пустой стек
        printf("Инициализирован новый стек (первый процесс)\n");
    } else {
        printf("Подключен к существующему стеку. Текущий размер: %d\n", 
               stack->top + 1);
    }
    sem_post(sem); // Освобождаем семафор
    
    // 3. Основной цикл работы со стеком
    int choice;
    char buffer[MAX_STRING_LENGTH];
    
    while (1) {
        print_menu();
        scanf("%d", &choice);
        getchar(); // Очищаем буфер после scanf
        
        switch (choice) {
            case 1: // Добавление элемента
                printf("Введите строку для добавления (макс %d символов): ", 
                       MAX_STRING_LENGTH - 1);
                fgets(buffer, MAX_STRING_LENGTH, stdin);
                
                // Убираем символ новой строки
                buffer[strcspn(buffer, "\n")] = '\0';
                
                sem_wait(sem); // Захватываем семафор
                
                if (stack->top >= MAX_STACK_SIZE - 1) {
                    printf("Стек переполнен! Невозможно добавить элемент.\n");
                } else {
                    stack->top++;
                    strncpy(stack->items[stack->top].data, buffer, MAX_STRING_LENGTH);
                    printf("Строка добавлена. Новый размер стека: %d\n", 
                           stack->top + 1);
                }
                
                sem_post(sem); // Освобождаем семафор
                break;
                
            case 2: // Удаление элемента
                sem_wait(sem); // Захватываем семафор
                
                if (stack->top < 0) {
                    printf("Стек пуст! Невозможно удалить элемент.\n");
                } else {
                    printf("Удалена строка: %s\n", 
                           stack->items[stack->top].data);
                    stack->top--;
                    printf("Новый размер стека: %d\n", stack->top + 1);
                }
                
                sem_post(sem); // Освобождаем семафор
                break;
                
            case 3: // Просмотр стека
                sem_wait(sem); // Захватываем семафор
                
                if (stack->top < 0) {
                    printf("Стек пуст.\n");
                } else {
                    printf("\n=== Содержимое стека (сверху вниз) ===\n");
                    for (int i = stack->top; i >= 0; i--) {
                        printf("[%d]: %s\n", i, stack->items[i].data);
                    }
                    printf("Всего элементов: %d\n", stack->top + 1);
                }
                
                sem_post(sem); // Освобождаем семафор
                break;
                
            case 4: // Очистка стека
                sem_wait(sem); // Захватываем семафор
                stack->top = -1;
                printf("Стек очищен.\n");
                sem_post(sem); // Освобождаем семафор
                break;
                
            case 5: // Завершение работы
                printf("Завершение работы...\n");
                
                // Проверяем, остались ли другие процессы
                int remaining_processes = 0;
                // Здесь могла бы быть логика проверки активных процессов,
                // но в упрощенной реализации просто отключаемся
                
                // Если это последний процесс, очищаем ресурсы
                if (remaining_processes == 0) {
                    printf("Последний процесс. Очистка ресурсов...\n");
                    munmap(stack, sizeof(SharedStack));
                    close(shm_fd);
                    shm_unlink(SHM_NAME);
                    sem_close(sem);
                    sem_unlink(SEM_NAME);
                } else {
                    printf("Другие процессы еще работают со стеком.\n");
                    munmap(stack, sizeof(SharedStack));
                    close(shm_fd);
                    sem_close(sem);
                }
                
                return 0;
                
            default:
                printf("Неверный выбор. Попробуйте снова.\n");
        }
    }
    
    return 0;
}