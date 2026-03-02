#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define EPS 0.001
#define DEFAULT_XY_FILE "xy.txt"
#define DEFAULT_XZ_FILE "xz.txt"
#define DEFAULT_YZ_FILE "yz.txt"

/* Структуры точек для каждой проекции */
typedef struct {
    double x, y;
} PointXY;

typedef struct {
    double x, z;
} PointXZ;

typedef struct {
    double y, z;
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
    
    // Выбираем медиану из трёх для лучшей производительности
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

/* Потоковая функция */
void* sort_thread(void *arg) {
    SortThreadArg *ta = (SortThreadArg*)arg;
    printf("Thread sorting %zu elements...\n", ta->nmemb);
    quicksort(ta->base, ta->nmemb, ta->size, ta->compar);
    printf("Thread finished sorting\n");
    return NULL;
}

/* ----------------------------------------- */
/* Чтение массивов из файлов */
PointXY* read_xy(const char *filename, size_t *n) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror(filename);
        return NULL;
    }
    
    PointXY *arr = NULL;
    size_t capacity = 0;
    size_t count = 0;
    double x, y;
    
    printf("Reading XY from %s...\n", filename);
    while (fscanf(f, "%lf %lf", &x, &y) == 2) {
        if (count == capacity) {
            capacity = capacity ? capacity * 2 : 16;
            arr = realloc(arr, capacity * sizeof(PointXY));
            if (!arr) { 
                fclose(f); 
                return NULL; 
            }
        }
        arr[count].x = x;
        arr[count].y = y;
        printf("  Read XY: %.3f %.3f\n", x, y);
        count++;
    }
    fclose(f);
    *n = count;
    printf("Total XY points: %zu\n", count);
    return arr;
}

PointXZ* read_xz(const char *filename, size_t *n) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror(filename);
        return NULL;
    }
    
    PointXZ *arr = NULL;
    size_t capacity = 0;
    size_t count = 0;
    double x, z;
    
    printf("Reading XZ from %s...\n", filename);
    while (fscanf(f, "%lf %lf", &x, &z) == 2) {
        if (count == capacity) {
            capacity = capacity ? capacity * 2 : 16;
            arr = realloc(arr, capacity * sizeof(PointXZ));
            if (!arr) { 
                fclose(f); 
                return NULL; 
            }
        }
        arr[count].x = x;
        arr[count].z = z;
        printf("  Read XZ: %.3f %.3f\n", x, z);
        count++;
    }
    fclose(f);
    *n = count;
    printf("Total XZ points: %zu\n", count);
    return arr;
}

PointYZ* read_yz(const char *filename, size_t *n) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror(filename);
        return NULL;
    }
    
    PointYZ *arr = NULL;
    size_t capacity = 0;
    size_t count = 0;
    double y, z;
    
    printf("Reading YZ from %s...\n", filename);
    while (fscanf(f, "%lf %lf", &y, &z) == 2) {
        if (count == capacity) {
            capacity = capacity ? capacity * 2 : 16;
            arr = realloc(arr, capacity * sizeof(PointYZ));
            if (!arr) { 
                fclose(f); 
                return NULL; 
            }
        }
        arr[count].y = y;
        arr[count].z = z;
        printf("  Read YZ: %.3f %.3f\n", y, z);
        count++;
    }
    fclose(f);
    *n = count;
    printf("Total YZ points: %zu\n", count);
    return arr;
}

/* ----------------------------------------- */
/* Проверка двух чисел на равенство с eps */
static inline int approx_eq(double a, double b) {
    return fabs(a - b) <= EPS;
}

/* Линейный поиск совпадений (более надёжный, чем бинарный с учётом погрешности) */
PointXYZ* find_all_xyz_simple(const PointXY *xy, size_t nxy,
                              const PointXZ *xz, size_t nxz,
                              const PointYZ *yz, size_t nyz,
                              size_t *out_cnt) {
    PointXYZ *result = NULL;
    size_t cap = 0;
    size_t cnt = 0;
    
    printf("\nSearching for matching 3D points...\n");
    
    for (size_t i = 0; i < nxy; i++) {
        double x = xy[i].x;
        double y = xy[i].y;
        
        printf("Checking XY point (%.3f, %.3f)\n", x, y);
        
        // Ищем совпадения в XZ
        for (size_t j = 0; j < nxz; j++) {
            if (approx_eq(xz[j].x, x)) {
                double z = xz[j].z;
                printf("  Found XZ match: (%.3f, %.3f) with z=%.3f\n", xz[j].x, xz[j].z, z);
                
                // Ищем совпадения в YZ
                for (size_t k = 0; k < nyz; k++) {
                    if (approx_eq(yz[k].y, y) && approx_eq(yz[k].z, z)) {
                        printf("    Found YZ match: (%.3f, %.3f)\n", yz[k].y, yz[k].z);
                        printf("    -> Found 3D point: (%.3f, %.3f, %.3f)\n", x, y, z);
                        
                        // Добавляем точку
                        if (cnt == cap) {
                            cap = cap ? cap * 2 : 16;
                            result = realloc(result, cap * sizeof(PointXYZ));
                            if (!result) {
                                fprintf(stderr, "Out of memory\n");
                                *out_cnt = cnt;
                                return result;
                            }
                        }
                        result[cnt].x = x;
                        result[cnt].y = y;
                        result[cnt].z = z;
                        cnt++;
                    }
                }
            }
        }
    }
    
    *out_cnt = cnt;
    return result;
}

/* ----------------------------------------- */
int main(int argc, char *argv[]) {
    const char *xy_file = DEFAULT_XY_FILE;
    const char *xz_file = DEFAULT_XZ_FILE;
    const char *yz_file = DEFAULT_YZ_FILE;

    if (argc >= 4) {
        xy_file = argv[1];
        xz_file = argv[2];
        yz_file = argv[3];
    } else {
        printf("Usage: %s <xy_file> <xz_file> <yz_file>\n", argv[0]);
        printf("Using default file names: %s, %s, %s\n", xy_file, xz_file, yz_file);
    }

    // Чтение массивов
    size_t nxy, nxz, nyz;
    PointXY *xy = read_xy(xy_file, &nxy);
    PointXZ *xz = read_xz(xz_file, &nxz);
    PointYZ *yz = read_yz(yz_file, &nyz);

    if (!xy || !xz || !yz) {
        fprintf(stderr, "Error reading input files\n");
        free(xy); free(xz); free(yz);
        return 1;
    }

    printf("\nRead %zu XY points, %zu XZ points, %zu YZ points\n", nxy, nxz, nyz);

    // Подготовка аргументов для потоков сортировки
    SortThreadArg arg_xy = {xy, nxy, sizeof(PointXY), cmp_xy};
    SortThreadArg arg_xz = {xz, nxz, sizeof(PointXZ), cmp_xz};
    SortThreadArg arg_yz = {yz, nyz, sizeof(PointYZ), cmp_yz};

    pthread_t thread_xy, thread_xz, thread_yz;

    printf("\nStarting sorting threads...\n");
    
    // Создание потоков
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

    // Ожидание завершения сортировки
    pthread_join(thread_xy, NULL);
    pthread_join(thread_xz, NULL);
    pthread_join(thread_yz, NULL);

    printf("\nSorting completed\n");

    // Вывод отсортированных массивов для проверки
    printf("\nSorted XY points:\n");
    for (size_t i = 0; i < nxy; i++) {
        printf("  XY[%zu]: %.3f %.3f\n", i, xy[i].x, xy[i].y);
    }
    
    printf("\nSorted XZ points:\n");
    for (size_t i = 0; i < nxz; i++) {
        printf("  XZ[%zu]: %.3f %.3f\n", i, xz[i].x, xz[i].z);
    }
    
    printf("\nSorted YZ points:\n");
    for (size_t i = 0; i < nyz; i++) {
        printf("  YZ[%zu]: %.3f %.3f\n", i, yz[i].y, yz[i].z);
    }

    // Поиск всех троек (используем простой линейный поиск для надёжности)
    size_t nxyz;
    PointXYZ *xyz = find_all_xyz_simple(xy, nxy, xz, nxz, yz, nyz, &nxyz);

    // Вывод результатов
    printf("\n========================================================\n");
    printf("RESULTS:\n");
    printf("Found %zu unique 3D points (eps = %.3f):\n", nxyz, EPS);
    if (nxyz > 0) {
        for (size_t i = 0; i < nxyz; i++) {
            printf("  Point %zu: (%.6f, %.6f, %.6f)\n", i+1, xyz[i].x, xyz[i].y, xyz[i].z);
        }
    } else {
        printf("  No 3D points found\n");
        printf("\nPossible reasons:\n");
        printf("  1. The points in different files don't match within eps=%.3f\n", EPS);
        printf("  2. Check if the coordinates in files are correct\n");
        printf("  3. Try increasing EPS value or check the input data\n");
    }
    printf("========================================================\n");

    // Освобождение памяти
    free(xy);
    free(xz);
    free(yz);
    free(xyz);

    return 0;
}