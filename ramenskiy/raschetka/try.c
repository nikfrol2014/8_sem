#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define EPS 0.001
// #define EPS 0.000000
#define LINE_BUF 256

/* Структуры точек для каждой проекции */
typedef struct {
    double x, y;  // для XY (topView)
} PointXY;

typedef struct {
    double x, z;  // для XZ (frontView)
} PointXZ;

typedef struct {
    double y, z;  // для YZ (profileView)
} PointYZ;

typedef struct {
    double x, y, z;
} PointXYZ;

/* Аргументы для потока сортировки */
typedef struct {
    void *base;
    size_t nmemb;
    size_t size;
    int (*compar)(const void*, const void*);
    const char *name;
} SortThreadArg;

/* ----------------------------------------- */
/* Функции сравнения для сортировки */
int cmp_xy(const void *a, const void *b) {
    const PointXY *pa = (const PointXY*)a;
    const PointXY *pb = (const PointXY*)b;
    if (fabs(pa->x - pb->x) <= EPS) {
        if (fabs(pa->y - pb->y) <= EPS) return 0;
        return (pa->y < pb->y) ? -1 : 1;
    }
    return (pa->x < pb->x) ? -1 : 1;
}

int cmp_xz(const void *a, const void *b) {
    const PointXZ *pa = (const PointXZ*)a;
    const PointXZ *pb = (const PointXZ*)b;
    if (fabs(pa->x - pb->x) <= EPS) {
        if (fabs(pa->z - pb->z) <= EPS) return 0;
        return (pa->z < pb->z) ? -1 : 1;
    }
    return (pa->x < pb->x) ? -1 : 1;
}

int cmp_yz(const void *a, const void *b) {
    const PointYZ *pa = (const PointYZ*)a;
    const PointYZ *pb = (const PointYZ*)b;
    if (fabs(pa->y - pb->y) <= EPS) {
        if (fabs(pa->z - pb->z) <= EPS) return 0;
        return (pa->z < pb->z) ? -1 : 1;
    }
    return (pa->y < pb->y) ? -1 : 1;
}

/* ----------------------------------------- */
/* Реализация быстрой сортировки */
static void swap(void *a, void *b, size_t size) {
    char *pa = (char*)a;
    char *pb = (char*)b;
    char tmp;
    for (size_t i = 0; i < size; i++) {
        tmp = pa[i];
        pa[i] = pb[i];
        pb[i] = tmp;
    }
}

static void quicksort_impl(void *base, size_t lo, size_t hi, size_t size,
                           int (*compar)(const void*, const void*)) {
    if (lo >= hi) return;
    
    char *arr = (char*)base;
    
    // Медиана из трёх для лучшей производительности
    size_t mid = lo + (hi - lo) / 2;
    if (compar(arr + lo*size, arr + mid*size) > 0)
        swap(arr + lo*size, arr + mid*size, size);
    if (compar(arr + lo*size, arr + hi*size) > 0)
        swap(arr + lo*size, arr + hi*size, size);
    if (compar(arr + mid*size, arr + hi*size) > 0)
        swap(arr + mid*size, arr + hi*size, size);
    
    // Ставим медиану в конец как опорный элемент
    swap(arr + mid*size, arr + hi*size, size);
    
    size_t pivot = hi;
    size_t i = lo;
    
    for (size_t j = lo; j < hi; j++) {
        if (compar(arr + j*size, arr + pivot*size) <= 0) {
            swap(arr + i*size, arr + j*size, size);
            i++;
        }
    }
    swap(arr + i*size, arr + pivot*size, size);
    
    if (i > lo) quicksort_impl(base, lo, i-1, size, compar);
    if (i < hi) quicksort_impl(base, i+1, hi, size, compar);
}

void quicksort(void *base, size_t nmemb, size_t size,
               int (*compar)(const void*, const void*)) {
    if (nmemb <= 1) return;
    quicksort_impl(base, 0, nmemb-1, size, compar);
}

/* Потоковая функция для сортировки */
void* sort_thread(void *arg) {
    SortThreadArg *ta = (SortThreadArg*)arg;
    printf("[%s] Начало сортировки %zu элементов...\n", ta->name, ta->nmemb);
    quicksort(ta->base, ta->nmemb, ta->size, ta->compar);
    printf("[%s] Сортировка завершена\n", ta->name);
    return NULL;
}

/* ----------------------------------------- */
/* Функция чтения данных из файла */
int read_projections(const char *filename, 
                     PointXY **xy_out, size_t *nxy_out,
                     PointXZ **xz_out, size_t *nxz_out,
                     PointYZ **yz_out, size_t *nyz_out) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return 0;
    }
    
    int xy_cap = 1024;
    int xz_cap = 1024;
    int yz_cap = 1024;
    
    PointXY *xy = (PointXY *)malloc((size_t)xy_cap * sizeof(PointXY));
    PointXZ *xz = (PointXZ *)malloc((size_t)xz_cap * sizeof(PointXZ));
    PointYZ *yz = (PointYZ *)malloc((size_t)yz_cap * sizeof(PointYZ));
    
    if (!xy || !xz || !yz) {
        fclose(fp);
        free(xy); free(xz); free(yz);
        return 0;
    }
    
    int nxy = 0, nxz = 0, nyz = 0;
    
    char line[LINE_BUF];
    int block = 0;          // 1-frontView, 2-profileView, 3-topView
    int waiting_second = 0;
    double saved = 0.0;
    
    printf("Чтение файла %s...\n", filename);
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';
        
        if (line[0] == '\0') continue;
        
        if (strstr(line, "frontView")) {
            block = 1;
            waiting_second = 0;
            printf("Найдена секция frontView (X,Z)\n");
            continue;
        }
        if (strstr(line, "profileView")) {
            block = 2;
            waiting_second = 0;
            printf("Найдена секция profileView (Y,Z)\n");
            continue;
        }
        if (strstr(line, "topView")) {
            block = 3;
            waiting_second = 0;
            printf("Найдена секция topView (X,Y)\n");
            continue;
        }
        
        if (strstr(line, "3D points")) {
            break;
        }
        
        if (strstr(line, "lcsX") || strstr(line, "lcsY") ||
            strstr(line, "3D edges") || strstr(line, "Algorithm") ||
            strstr(line, "Read plot") || strstr(line, "Get wires")) {
            continue;
        }
        
        char *comma = strchr(line, ',');
        if (comma) {
            *comma = '.';
        }
        
        double value;
        if (sscanf(line, "%lf", &value) != 1) {
            continue;
        }
        
        if (!waiting_second) {
            saved = value;
            waiting_second = 1;
            continue;
        }
        
        if (block == 1) {
            if (nxz >= xz_cap) {
                xz_cap *= 2;
                PointXZ *resized = (PointXZ *)realloc(xz, (size_t)xz_cap * sizeof(PointXZ));
                if (!resized) {
                    fclose(fp);
                    free(xy); free(xz); free(yz);
                    return 0;
                }
                xz = resized;
            }
            xz[nxz].x = saved;
            xz[nxz].z = value;
            nxz++;
        } 
        else if (block == 2) {
            if (nyz >= yz_cap) {
                yz_cap *= 2;
                PointYZ *resized = (PointYZ *)realloc(yz, (size_t)yz_cap * sizeof(PointYZ));
                if (!resized) {
                    fclose(fp);
                    free(xy); free(xz); free(yz);
                    return 0;
                }
                yz = resized;
            }
            yz[nyz].y = saved;
            yz[nyz].z = value;
            nyz++;
        } 
        else if (block == 3) {
            if (nxy >= xy_cap) {
                xy_cap *= 2;
                PointXY *resized = (PointXY *)realloc(xy, (size_t)xy_cap * sizeof(PointXY));
                if (!resized) {
                    fclose(fp);
                    free(xy); free(xz); free(yz);
                    return 0;
                }
                xy = resized;
            }
            xy[nxy].x = saved;
            xy[nxy].y = value;
            nxy++;
        }
        
        waiting_second = 0;
    }
    
    fclose(fp);
    
    printf("\nИтоги чтения:\n");
    printf("  topView (X,Y): %d точек\n", nxy);
    printf("  frontView (X,Z): %d точек\n", nxz);
    printf("  profileView (Y,Z): %d точек\n", nyz);
    
    *xy_out = xy;
    *nxy_out = nxy;
    *xz_out = xz;
    *nxz_out = nxz;
    *yz_out = yz;
    *nyz_out = nyz;
    
    return 1;
}

/* ----------------------------------------- */
/* Проверка двух чисел на равенство с EPS */
static inline int approx_eq(double a, double b) {
    return fabs(a - b) <= EPS;
}

/* ----------------------------------------- */
int main(int argc, char *argv[]) {
    const char *input_file = "polyhedral 2D.txt";
    
    if (argc >= 2) {
        input_file = argv[1];
    } else {
        printf("Использование: %s <input_file>\n", argv[0]);
        printf("Используется файл по умолчанию: %s\n", input_file);
    }
    
    // Чтение данных
    PointXY *xy = NULL;
    PointXZ *xz = NULL;
    PointYZ *yz = NULL;
    size_t nxy, nxz, nyz;
    
    if (!read_projections(input_file, &xy, &nxy, &xz, &nxz, &yz, &nyz)) {
        fprintf(stderr, "Ошибка чтения файла\n");
        return 1;
    }
    
    if (nxy == 0 || nxz == 0 || nyz == 0) {
        fprintf(stderr, "Ошибка: недостаточно данных в файле\n");
        fprintf(stderr, "topView: %zu, frontView: %zu, profileView: %zu\n", nxy, nxz, nyz);
        free(xy); free(xz); free(yz);
        return 1;
    }
    
    printf("\nЗагружено: %zu точек XY, %zu точек XZ, %zu точек YZ\n", nxy, nxz, nyz);
    
    // Подготовка аргументов для потоков
    SortThreadArg arg_xy = {xy, nxy, sizeof(PointXY), cmp_xy, "XY"};
    SortThreadArg arg_xz = {xz, nxz, sizeof(PointXZ), cmp_xz, "XZ"};
    SortThreadArg arg_yz = {yz, nyz, sizeof(PointYZ), cmp_yz, "YZ"};
    
    pthread_t thread_xy, thread_xz, thread_yz;
    
    printf("\n=== Запуск параллельной сортировки ===\n");
    
    // Создание потоков для сортировки каждого массива
    if (pthread_create(&thread_xy, NULL, sort_thread, &arg_xy) != 0) {
        perror("pthread_create xy");
        free(xy); free(xz); free(yz);
        return 1;
    }
    if (pthread_create(&thread_xz, NULL, sort_thread, &arg_xz) != 0) {
        perror("pthread_create xz");
        pthread_cancel(thread_xy);
        free(xy); free(xz); free(yz);
        return 1;
    }
    if (pthread_create(&thread_yz, NULL, sort_thread, &arg_yz) != 0) {
        perror("pthread_create yz");
        pthread_cancel(thread_xy);
        pthread_cancel(thread_xz);
        free(xy); free(xz); free(yz);
        return 1;
    }
    
    // Ожидание завершения всех потоков
    pthread_join(thread_xy, NULL);
    pthread_join(thread_xz, NULL);
    pthread_join(thread_yz, NULL);
    
    printf("\n=== Сортировка завершена ===\n");
    
    // Вывод первых 5 отсортированных точек для проверки
    printf("\nПервые 5 отсортированных точек:\n");
    printf("XY (x,y): ");
    for (size_t i = 0; i < (nxy < 5 ? nxy : 5); i++) {
        printf("(%.3f,%.3f) ", xy[i].x, xy[i].y);
    }
    printf("\nXZ (x,z): ");
    for (size_t i = 0; i < (nxz < 5 ? nxz : 5); i++) {
        printf("(%.3f,%.3f) ", xz[i].x, xz[i].z);
    }
    printf("\nYZ (y,z): ");
    for (size_t i = 0; i < (nyz < 5 ? nyz : 5); i++) {
        printf("(%.3f,%.3f) ", yz[i].y, yz[i].z);
    }
    printf("\n");
    
    // Поиск уникальных 3D точек
    printf("\n========================================================\n");
    printf("ПОИСК 3D ТОЧЕК (x, y, z) с точностью EPS = %.3f\n", EPS);
    printf("========================================================\n");
    
    PointXYZ *unique_points = NULL;
    int unique_count = 0;
    int found_count = 0;
    
    for (size_t i = 0; i < nxy; i++) {
        double x = xy[i].x;
        double y = xy[i].y;
        
        for (size_t j = 0; j < nxz; j++) {
            if (approx_eq(xz[j].x, x)) {
                double z = xz[j].z;
                
                for (size_t k = 0; k < nyz; k++) {
                    if (approx_eq(yz[k].y, y) && approx_eq(yz[k].z, z)) {
                        found_count++;
                        
                        // Проверка на уникальность
                        int exists = 0;
                        for (int t = 0; t < unique_count; t++) {
                            if (approx_eq(unique_points[t].x, x) &&
                                approx_eq(unique_points[t].y, y) &&
                                approx_eq(unique_points[t].z, z)) {
                                exists = 1;
                                break;
                            }
                        }
                        
                        if (!exists) {
                            unique_count++;
                            unique_points = realloc(unique_points, unique_count * sizeof(PointXYZ));
                            unique_points[unique_count-1] = (PointXYZ){x, y, z};
                            printf("Точка %d: (%.6f, %.6f, %.6f)\n", unique_count, x, y, z);
                        }
                        break;
                    }
                }
            }
        }
    }
    
    printf("\n========================================================\n");
    printf("РЕЗУЛЬТАТЫ:\n");
    printf("  Всего совпадений: %d\n", found_count);
    printf("  Уникальных 3D точек: %d\n", unique_count);
    printf("  Точность сравнения: EPS = %.3f\n", EPS);
    printf("========================================================\n");
    
    // Освобождение памяти
    free(xy);
    free(xz);
    free(yz);
    free(unique_points);
    
    return 0;
}