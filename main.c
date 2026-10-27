#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N              10000     
#define TRIALS         100       
#define KEY_MAX        1000000   
#define PARTIAL_SWAPS  500      
#define STAB_KEY_RANGE 100    
#define SEED           12345    


typedef struct {
    int key1;
    int key2;
    int order;
} Data;


typedef struct {
    long long cmp;
    long long mov;
} Count;

typedef void (*SortFn)(Data *, int, Count *);


static int rand_range(int max)   
{
    int hi = rand() & 0x7FFF;
    int lo = rand() & 0x7FFF;
    return ((hi << 15) | lo) % (max + 1);
}

static void make_random_data(Data *a, int n, int key1_max)
{
    for (int i = 0; i < n; i++) {
        a[i].key1  = rand_range(key1_max);
        a[i].key2  = rand_range(KEY_MAX);   
        a[i].order = i;                   
    }
}


static void swap_data(Data *x, Data *y, Count *c)
{
    Data temp = *x;  c->mov++;
    *x = *y;         c->mov++;
    *y = temp;       c->mov++;
}

//1) 선택 정렬//
static void selection_sort(Data *a, int n, Count *c)
{
    for (int i = 0; i < n - 1; i++) {
        int min = i;
        for (int j = i + 1; j < n; j++) {
            c->cmp++;
            if (a[j].key1 < a[min].key1)
                min = j;
        }
        if (min != i)
            swap_data(&a[i], &a[min], c);
    }
}

//2) 버블 정렬//
static void bubble_core(Data *a, int n, Count *c, int early)
{
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - 1 - i; j++) {
            c->cmp++;
            if (a[j].key1 > a[j + 1].key1) {   
                swap_data(&a[j], &a[j + 1], c);
                swapped = 1;
            }
        }
        if (early && !swapped)  
            break;
    }
}

static void bubble_sort(Data *a, int n, Count *c)       { bubble_core(a, n, c, 0); }
static void bubble_sort_early(Data *a, int n, Count *c) { bubble_core(a, n, c, 1); }


 //(3) 삽입 정렬//

static void insertion_core(Data *a, int n, Count *c, int use_ge)
{
    for (int i = 1; i < n; i++) {
        Data cur = a[i];  c->mov++;
        int j = i - 1;

        while (j >= 0) {
            c->cmp++;
            int shift = use_ge ? (a[j].key1 >= cur.key1)
                               : (a[j].key1 >  cur.key1);
            if (!shift)
                break;
            a[j + 1] = a[j];  c->mov++;
            j--;
        }
        a[j + 1] = cur;  c->mov++;
    }
}

static void insertion_sort_A(Data *a, int n, Count *c) { insertion_core(a, n, c, 0); }  /* > */
static void insertion_sort_B(Data *a, int n, Count *c) { insertion_core(a, n, c, 1); }  /* >= */


static int is_sorted(const Data *a, int n)
{
    for (int i = 0; i < n - 1; i++)
        if (a[i].key1 > a[i + 1].key1)
            return 0;
    return 1;
}


static int count_stability_violations(const Data *a, int n)
{
    int bad = 0;
    for (int i = 0; i < n - 1; i++)
        if (a[i].key1 == a[i + 1].key1 && a[i].order > a[i + 1].order)
            bad++;
    return bad;
}


static int cmp_asc(const void *p, const void *q)
{
    const Data *x = p, *y = q;
    if (x->key1 != y->key1) return (x->key1 > y->key1) - (x->key1 < y->key1);
    return (x->order > y->order) - (x->order < y->order);
}
static int cmp_desc(const void *p, const void *q)
{
    return cmp_asc(q, p);
}


static int run_one(SortFn fn, const Data *src, Data *work, Count *out)
{
    memcpy(work, src, sizeof(Data) * N);
    out->cmp = 0;
    out->mov = 0;
    fn(work, N, out);
    return is_sorted(work, N);
}


#define ALGO_COUNT 4
static const char *algo_name[ALGO_COUNT] = {
    "Selection Sort",
    "Bubble Sort",
    "Bubble Sort (Early Termination)",
    "Insertion Sort"
};
static SortFn algo_fn[ALGO_COUNT] = {
    selection_sort, bubble_sort, bubble_sort_early, insertion_sort_A
};


static void run_state(const char *title, const Data *src, Data *work)
{
    printf("%s\n\n", title);
    for (int k = 0; k < ALGO_COUNT; k++) {
        Count c;
        int ok = run_one(algo_fn[k], src, work, &c);
        printf("%s\n", algo_name[k]);
        printf("Comparisons : %lld\n", c.cmp);
        printf("Movements   : %lld\n", c.mov);
        printf("Sorted Check: %s\n\n", ok ? "OK" : "FAIL");
    }
    printf("------------------------------------\n\n");
}


static void print_small(const char *label, const Data *a, int n)
{
    printf("  %-8s:", label);
    for (int i = 0; i < n; i++)
        printf(" (%d,%d)", a[i].key1, a[i].key2);
    printf("\n");
}

static void stability_small_example(const char *name, SortFn fn)
{
    Data a[4] = {
        {527, 100, 0}, {527, 200, 1}, {125, 300, 2}, {892, 400, 3}
    };
    Count dummy = {0, 0};

    printf(" [%s]\n", name);
    print_small("before", a, 4);
    fn(a, 4, &dummy);
    print_small("after", a, 4);
}

static void stability_big_test(const char *name, SortFn fn, const Data *dup_src, Data *work)
{
    Count dummy = {0, 0};
    memcpy(work, dup_src, sizeof(Data) * N);
    fn(work, N, &dummy);

    int sorted_ok = is_sorted(work, N);
    int bad = count_stability_violations(work, N);

    printf("%-21s: %s  (sorted=%s, order violations=%d)\n",
           name, bad == 0 ? "Stable" : "Not Stable",
           sorted_ok ? "OK" : "FAIL", bad);
}


int main(void)
{
    static Data base[N], work[N];   /* 스택 말고 static으로 (10000개 * 12바이트) */
    srand(SEED);

    printf("Data Count : %d\n\n", N);

    /* ===== 1. 무작위 데이터 100회 ===== */
    long long total_cmp[ALGO_COUNT] = {0};
    long long total_mov[ALGO_COUNT] = {0};
    int fail_count[ALGO_COUNT] = {0};

    for (int t = 0; t < TRIALS; t++) {
        make_random_data(base, N, KEY_MAX);   /* 이번 회차 데이터 1벌 -> 4개 알고리즘이 같이 사용 */
        for (int k = 0; k < ALGO_COUNT; k++) {
            Count c;
            if (!run_one(algo_fn[k], base, work, &c))
                fail_count[k]++;
            total_cmp[k] += c.cmp;
            total_mov[k] += c.mov;
        }
    }

    printf("Random Data\nNumber of Trials : %d\n\n", TRIALS);
    for (int k = 0; k < ALGO_COUNT; k++) {
        printf("%s\n", algo_name[k]);
        printf("Total Comparisons   : %lld\n", total_cmp[k]);
        printf("Average Comparisons : %lld\n", total_cmp[k] / TRIALS);
        printf("Average Movements   : %lld\n", total_mov[k] / TRIALS);
        printf("Sorted Check        : %d/%d OK\n\n", TRIALS - fail_count[k], TRIALS);
    }
    printf("------------------------------------\n\n");

 
    make_random_data(base, N, KEY_MAX);   /* 원본 1개 */

    static Data sorted_data[N], reverse_data[N], partial_data[N];
    memcpy(sorted_data, base, sizeof(Data) * N);
    qsort(sorted_data, N, sizeof(Data), cmp_asc);


    memcpy(reverse_data, base, sizeof(Data) * N);
    qsort(reverse_data, N, sizeof(Data), cmp_desc);

 
    memcpy(partial_data, sorted_data, sizeof(Data) * N);
    for (int s = 0; s < PARTIAL_SWAPS; s++) {
        int i = rand_range(N - 1);
        int j = rand_range(N - 1);
        Data tmp = partial_data[i];
        partial_data[i] = partial_data[j];
        partial_data[j] = tmp;
    }

    run_state("Sorted Data", sorted_data, work);
    run_state("Reverse-Sorted Data", reverse_data, work);
    run_state("Partially Sorted Data", partial_data, work);

    
    static Data dup_data[N];
    make_random_data(dup_data, N, STAB_KEY_RANGE - 1);   /* key1: 0~99 */

    printf("Stability Verification\n\n");
    printf("(1) Small example (527,100) (527,200) (125,300) (892,400)\n");
    stability_small_example("Selection Sort",     selection_sort);
    stability_small_example("Bubble Sort",        bubble_sort);
    stability_small_example("Insertion Sort (>)",  insertion_sort_A);
    stability_small_example("Insertion Sort (>=)", insertion_sort_B);

    printf("\n(2) Large test: %d data, key1 in 0~%d (many duplicates)\n", N, STAB_KEY_RANGE - 1);
    stability_big_test("Selection Sort",      selection_sort,   dup_data, work);
    stability_big_test("Bubble Sort",         bubble_sort,      dup_data, work);
    stability_big_test("Insertion Sort (>)",  insertion_sort_A, dup_data, work);
    stability_big_test("Insertion Sort (>=)", insertion_sort_B, dup_data, work);

   
    printf("\n(3) Insertion Sort > vs >= on the same duplicate data\n");
    {
        Count ca, cb;
        run_one(insertion_sort_A, dup_data, work, &ca);
        run_one(insertion_sort_B, dup_data, work, &cb);
        printf("Insertion (>)  : Comparisons %lld, Movements %lld\n", ca.cmp, ca.mov);
        printf("Insertion (>=) : Comparisons %lld, Movements %lld\n", cb.cmp, cb.mov);
    }

    return 0;
}
