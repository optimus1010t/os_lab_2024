#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define N 4

typedef struct {
    int tno;
    char tname[5];
} tinfo;

#define S 5

int A[S], C[S];

pthread_mutex_t csmutex;
pthread_mutex_t donemutex;
pthread_cond_t donecond;
int mdone = 0, wdone = 0;

void *tmain(void *targ) {
    int no, i;
    char name[5];
    pthread_t tid;
    int count = 0, s, t;

    no = ((tinfo *)targ)->tno;
    strcpy(name, ((tinfo *)targ)->tname);
    tid = pthread_self();

    printf("\t\t\t\t\t(%d,%s) [%lu] running\n", no, name, tid);

    while (1) {
        pthread_mutex_lock(&donemutex);
        if (mdone) {
            ++wdone;
            if (wdone < N) {
                if (wdone == 1)
                    printf("\n");
                printf("\t\t\t\t\t(%d,%s) going to wait\n", no, name);
                pthread_cond_wait(&donecond, &donemutex);
            } else {
                printf("\n");
                printf("\t\t\t\t\t(%d,%s) going to broadcast\n", no, name);
                printf("\n");
                pthread_cond_broadcast(&donecond);
            }
            pthread_mutex_unlock(&donemutex);
            printf("\t\t\t\t\t(%d,%s) exits with count = %d\n", no, name, count);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&donemutex);

        i = rand() % S;

        pthread_mutex_lock(&csmutex);
        s = A[i];
        t = (s % 2 == 0) ? (s / 2) : (3 * s + 1);
        A[i] = t;
        ++C[i];
        pthread_mutex_unlock(&csmutex);

        ++count;
        printf("\t\t\t\t\t(%d,%s) changes A[%2d] : %5d -> %5d\n", no, name, i, s, t);

        usleep(10);
    }
}

void init_arrays() {
    int i;
    for (i = 0; i < S; ++i) {
        A[i] = 1 + rand() % 999;
        C[i] = 0;
    }
}

void print_arrays() {
    int i;
    for (i = 0; i < S; ++i) {
        printf("%4d (%4d)", A[i], C[i]);
        if (i % 7 == 6)
            printf("\n");
    }
    printf("\n");
}

void create_workers(pthread_t *tid, tinfo *param) {
    pthread_attr_t attr;
    int i, j;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (i = 0; i < N; ++i) {
        param[i].tno = i + 1;
        for (j = 0; j < 4; ++j)
            param[i].tname[j] = 'A' + rand() % 26;
        param[i].tname[4] = '\0';
        if (pthread_create(tid + i, &attr, tmain, (void *)(param + i))) {
            fprintf(stderr, "Master thread: Unable to create thread\n");
            pthread_attr_destroy(&attr);
            exit(1);
        }
        printf("(%d,%s) [%lu] created\n", param[i].tno, param[i].tname, tid[i]);
    }
    sleep(1);
    pthread_attr_destroy(&attr);
}

void create_mutex() {
    pthread_mutex_init(&csmutex, NULL);
    pthread_mutex_init(&donemutex, NULL);
    pthread_mutex_trylock(&csmutex);
    pthread_mutex_unlock(&csmutex);
    pthread_mutex_trylock(&donemutex);
    pthread_mutex_unlock(&donemutex);
    pthread_cond_init(&donecond, NULL);
}

void do_work(pthread_t *tid, tinfo *param) {
    int i;
    sleep(10);
    pthread_mutex_lock(&donemutex);
    mdone = 1;
    pthread_mutex_unlock(&donemutex);
    for (i = 0; i < N; ++i) {
        if (pthread_join(tid[i], NULL)) {
            fprintf(stderr, "Unable to wait for thread [%lu]\n", tid[i]);
        } else {
            printf("(%d,%s) has joined\n", param[i].tno, param[i].tname);
        }
    }
    printf("\n");
}

void  wind_up() {
    printf("\nWinding up\n\n");
    pthread_mutex_destroy(&csmutex);
    pthread_mutex_destroy(&donemutex);
    pthread_cond_destroy(&donecond);
}

int main() {
    pthread_t tid[N];
    tinfo param[N];

    srand((unsigned int)time(NULL));
    create_mutex();
    init_arrays();
    print_arrays();
    create_workers(tid, param);
    do_work(tid, param);
    print_arrays();
    wind_up();
    exit(0);
}
