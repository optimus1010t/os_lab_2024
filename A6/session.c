#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <event.h>

#define MAX_PATIENTS 25
#define MAX_SALESREP 3
#define MAX_VISITORS 1000

int token_patient = 0;
int token_salesrep = 0;
int token_reporter = 0;
int patients_done = 0;
int salesreps_done = 0;
int reporters_done = 0;
int is_idle = 0;

int time_value = 0;

typedef struct {
    int tno;
    int ttime;
} tinfo;

// pthread_mutex_t doc_mutex;
pthread_mutex_t patient_mutex;
pthread_mutex_t salesrep_mutex;
pthread_mutex_t reporter_mutex;

// pthread_mutex_t count_mutex;
// pthread_mutex_t time_mutex;
pthread_mutex_t is_idle_mutex;

pthread_barrier_t rep_barrier, rep_barrier_2;
pthread_barrier_t pat_barrier, pat_barrier_2;
pthread_barrier_t sal_barrier, sal_barrier_2;

pthread_barrier_t doc_ass_barrier_1, doc_ass_barrier_2;

pthread_cond_t doc_cond;
pthread_cond_t patient_cond;
pthread_cond_t salesrep_cond;
pthread_cond_t reporter_cond;

void printTime(int offsetMinutes) {
    int baseHour = 9;
    int baseMinute = 0;

    int totalMinutes = baseHour * 60 + baseMinute + offsetMinutes;
    int hour = totalMinutes / 60;
    int minute = totalMinutes % 60;

    char *ampm = "am";
    if (hour >= 12) {
        ampm = "pm";
    }
    if (hour > 12) {
        hour -= 12;
    }
    printf("%02d:%02d%s", hour, minute, ampm);
}

void* patient(void* arg) {
    pthread_mutex_lock(&patient_mutex);
    pthread_barrier_wait(&pat_barrier);
    printf("["); printTime(time_value); printf("] Doctor has next visitor\n", time_value);  
    time_value += ((tinfo *)arg)->ttime;
    printf("["); printTime((time_value-((tinfo *)arg)->ttime)); printf(" - "); printTime(time_value);
    printf("] Patient %d is in doctor's chamber\n", ((tinfo *)arg)->tno);
    patients_done++;
    // pthread_barrier_init(&pat_barrier, NULL, 2);
    // printf("asdasdasdsplpp\n");
    pthread_barrier_wait(&pat_barrier_2);
    pthread_mutex_unlock(&patient_mutex);
}

void* salesrep(void* arg) {
    pthread_mutex_lock(&salesrep_mutex);
    pthread_barrier_wait(&sal_barrier);
    printf("["); printTime(time_value); printf("] Doctor has next visitor\n", time_value);  
    time_value += ((tinfo *)arg)->ttime;
    printf("["); printTime((time_value-((tinfo *)arg)->ttime)); printf(" - "); printTime(time_value);
    printf("] Sales representative %d is in doctor's chamber\n", ((tinfo *)arg)->tno);
    salesreps_done++;
    // pthread_barrier_init(&sal_barrier, NULL, 2);
    pthread_barrier_wait(&sal_barrier_2);
    pthread_mutex_unlock(&salesrep_mutex);
}

void* reporter(void* arg) {
    pthread_mutex_lock(&reporter_mutex);
    pthread_barrier_wait(&rep_barrier);
    printf("["); printTime(time_value); printf("] Doctor has next visitor\n", time_value);  
    time_value += ((tinfo *)arg)->ttime;
    printf("["); printTime((time_value-((tinfo *)arg)->ttime)); printf(" - "); printTime(time_value);
    printf("] Reporter %d is in doctor's chamber\n", ((tinfo *)arg)->tno);
    reporters_done++;
    // pthread_barrier_init(&rep_barrier, NULL, 2);
    pthread_barrier_wait(&rep_barrier_2);
    pthread_mutex_unlock(&reporter_mutex);    
}

void* doctor(void* arg) {
    while (1) {
        // printf("doctorrrrrr\n");
        if (patients_done == MAX_PATIENTS && salesreps_done == MAX_SALESREP) {
            break;
        }
        pthread_barrier_wait(&doc_ass_barrier_1);
        // printf("dfsdfsdf\n");
        if (token_reporter > reporters_done) {
            // printf("sasqreyreydasd\n");
            pthread_mutex_lock(&is_idle_mutex);
            // printf("sasdassdfsdfdfd\n");
            is_idle = 0;
            pthread_mutex_unlock(&is_idle_mutex);
            pthread_barrier_wait(&rep_barrier);
            // printf("fgppooooo090909\n");
            pthread_barrier_wait(&rep_barrier_2);
            pthread_barrier_wait(&doc_ass_barrier_2);
        }
        else if ((token_patient > MAX_PATIENTS ? MAX_PATIENTS : token_patient) > patients_done) {
            // printf("sasdasd\n");
            pthread_mutex_lock(&is_idle_mutex);
            is_idle = 0;
            pthread_mutex_unlock(&is_idle_mutex);
            // printf("ASdasdasdfg\n");
            pthread_barrier_wait(&pat_barrier);
            // printf("ASdasdasdfdfghjklg\n");
            // printf("fgppooooo\n");
            pthread_barrier_wait(&pat_barrier_2);
            pthread_barrier_wait(&doc_ass_barrier_2);
            // printf("sdasdakjljkljl\n");
        }
        else if ((token_salesrep > MAX_SALESREP ? MAX_SALESREP : token_salesrep) > salesreps_done) {
            pthread_mutex_lock(&is_idle_mutex);
            is_idle = 0;
            pthread_mutex_unlock(&is_idle_mutex);
            pthread_barrier_wait(&sal_barrier);
            pthread_barrier_wait(&sal_barrier_2);
            pthread_barrier_wait(&doc_ass_barrier_2);
        } else {
            pthread_mutex_lock(&is_idle_mutex);
            is_idle = 1;
            pthread_mutex_unlock(&is_idle_mutex);
            pthread_barrier_wait(&doc_ass_barrier_2);
        }       
    }
    // printf("Doctor leaves (session over)\n");
}

void create_mutex() {
    // pthread_mutex_init(&doc_mutex, NULL); pthread_mutex_trylock(&doc_mutex);
    pthread_mutex_init(&patient_mutex, NULL); pthread_mutex_trylock(&patient_mutex); pthread_mutex_unlock(&patient_mutex);
    pthread_mutex_init(&salesrep_mutex, NULL); pthread_mutex_trylock(&salesrep_mutex); pthread_mutex_unlock(&salesrep_mutex);
    pthread_mutex_init(&reporter_mutex, NULL); pthread_mutex_trylock(&reporter_mutex); pthread_mutex_unlock(&reporter_mutex);
    pthread_mutex_init(&is_idle_mutex, NULL); pthread_mutex_trylock(&is_idle_mutex); pthread_mutex_unlock(&is_idle_mutex);
    
    pthread_barrier_init(&rep_barrier, NULL, 2); pthread_barrier_init(&rep_barrier_2, NULL, 2);
    pthread_barrier_init(&pat_barrier, NULL, 2); pthread_barrier_init(&pat_barrier_2, NULL, 2);
    pthread_barrier_init(&sal_barrier, NULL, 2); pthread_barrier_init(&sal_barrier_2, NULL, 2);
    pthread_barrier_init(&doc_ass_barrier_1, NULL, 2); pthread_barrier_init(&doc_ass_barrier_2, NULL, 2);
    
    pthread_cond_init(&doc_cond, NULL);
    pthread_cond_init(&patient_cond, NULL);
    pthread_cond_init(&salesrep_cond, NULL);
    pthread_cond_init(&reporter_cond, NULL);
}

void destroy_mutex() {
    pthread_mutex_destroy(&patient_mutex);
    pthread_mutex_destroy(&salesrep_mutex);
    pthread_mutex_destroy(&reporter_mutex);
    pthread_mutex_destroy(&is_idle_mutex);
    pthread_barrier_destroy(&rep_barrier); pthread_barrier_destroy(&rep_barrier_2);
    pthread_barrier_destroy(&pat_barrier); pthread_barrier_destroy(&pat_barrier_2);
    pthread_barrier_destroy(&sal_barrier); pthread_barrier_destroy(&sal_barrier_2);
    pthread_barrier_destroy(&doc_ass_barrier_1); pthread_barrier_destroy(&doc_ass_barrier_2);
}

int main() {
    eventQ E;
    E = initEQ("ARRIVAL.txt");
    create_mutex();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    int param_doc;
    param_doc = 1;
    pthread_t tid_doc;

    tinfo arg_pat[MAX_PATIENTS]; pthread_t tid_patient[MAX_PATIENTS];
    for (int i = 0; i < MAX_PATIENTS; ++i) arg_pat[i].tno = i + 1;
    tinfo arg_sal[MAX_SALESREP]; pthread_t tid_salesrep[MAX_SALESREP];
    for (int i = 0; i < MAX_SALESREP; ++i) arg_sal[i].tno = i + 1; 
    tinfo arg_rep[MAX_VISITORS]; pthread_t tid_rep[MAX_VISITORS];
    for (int i = 0; i < MAX_VISITORS; ++i) arg_rep[i].tno = i + 1;    
    int count = 0;
    
    while ((!emptyQ(E)) && (patients_done < MAX_PATIENTS || salesreps_done < MAX_SALESREP)) {
        event e = nextevent(E);
        if (count != 0) {
            if (e.time > time_value && (patients_done == token_patient || patients_done == MAX_PATIENTS) && (salesreps_done == token_salesrep || salesreps_done == MAX_SALESREP) && reporters_done == token_reporter ){
                time_value = e.time;
                continue;
            }
            if (e.time > time_value) {
                pthread_barrier_wait(&doc_ass_barrier_1);
                // printf("doc_ass_1_done\n");
                pthread_barrier_wait(&doc_ass_barrier_2);
                continue;
                // printf("doc_ass_2_done\n");
            }
        } else if (e.time >= 0){
            pthread_create(&tid_doc, &attr, doctor, (void *)&param_doc);
            count++;
            continue;
        }
        if (e.type == 'P') {
            printf("\t["); printTime(e.time); 
            printf("] Patient %d arrives\n", token_patient + 1);
        }
        else if (e.type == 'S') {
            printf("\t["); printTime(e.time); 
            printf("] Salesrep %d arrives\n", token_salesrep+1);
        }
        else if (e.type == 'R') {
            printf("\t["); printTime(e.time); 
            printf("] Reporter %d arrives\n", token_reporter+1);
        }
        else printf("Invalid event type\n");
        if (e.type == 'P') {
            if (token_patient < MAX_PATIENTS) {
                arg_pat[token_patient].ttime = e.duration;
                pthread_create(&tid_patient[token_patient], &attr, patient, (void *)&arg_pat[token_patient]);
            }
            else {
                printf("\t["); printTime(e.time);
                printf("] Patient %d leaves (quota full)\n", token_patient+1);
            }
            token_patient++;
        }
        else if (e.type == 'S') {
            if (token_salesrep < MAX_SALESREP) {
                arg_sal[token_salesrep].ttime = e.duration;
                pthread_create(&tid_salesrep[token_salesrep], &attr, salesrep, (void *)&arg_sal[token_salesrep]);
            }
            else {
                printf("\t["); printTime(e.time);
                printf("] Salesrep %d leaves (quota full)\n", token_salesrep+1);
            }
            token_salesrep++;
        }
        else if (e.type == 'R') {
            if (patients_done != MAX_PATIENTS && salesreps_done != MAX_SALESREP) {
                arg_rep[token_reporter].ttime = e.duration;
                pthread_create(&tid_rep[token_reporter], &attr, reporter, (void *)&arg_rep[token_reporter]);
            }
            else {
                printf("\t["); printTime(e.time);
                printf("] Reporter %d leaves (session over)\n", token_reporter+1);
            }
            token_reporter++;
        }
        else printf("Invalid event type\n");
        E = delevent(E);       
    }
    if (emptyQ(E)){
        printf("["); printTime(time_value); printf("] Doctor leaves (session over)\n");
    }
    int c = 0;
    while (!emptyQ(E)) {
        event e = nextevent(E);
        if (e.time > time_value && c == 0) {
            printf("["); printTime(time_value); printf("] Doctor leaves (session over)\n");
            c++;
        }
        if (e.type == 'P') {
            if (time_value >= e.time) {
                printf("\t["); printTime(e.time); printf("] Patient %d arrives\n", token_patient+1);
                printf("\t["); printTime(e.time); printf("] Patient %d leaves (quota full)\n", token_patient+1);
            }
            else {
                printf("\t["); printTime(e.time); printf("] Patient %d arrives\n", token_patient+1);
                printf("\t["); printTime(e.time); printf("] Patient %d leaves (session over)\n", token_patient+1);
            }
            token_patient++;
        }
        else if (e.type == 'S') {
            if (time_value >= e.time) {
                printf("\t["); printTime(e.time); printf("] Salesrep %d arrives\n", token_salesrep+1);
                printf("\t["); printTime(e.time); printf("] Salesrep %d leaves (quota full)\n", token_salesrep+1);
            }
            else {
                printf("\t["); printTime(e.time); printf("] Salesrep %d arrives\n", token_salesrep+1);
                printf("\t["); printTime(e.time); printf("] Salesrep %d leaves (session over)\n", token_salesrep+1);
            }
            token_salesrep++;
        }
        else if (e.type == 'R') {
            if (time_value >= e.time) {
                printf("\t["); printTime(e.time); printf("] Reporter %d arrives\n", token_reporter+1);
                printf("\t["); printTime(e.time); printf("] Reporter %d leaves (session over)\n", token_reporter+1);
            }
            else {
                printf("\t["); printTime(e.time); printf("] Reporter %d arrives\n", token_reporter+1);
                printf("\t["); printTime(e.time); printf("] Reporter %d leaves (session over)\n", token_reporter+1);
            }
            token_reporter++;
        }
        else {
            printf("Invalid event type\n");
        }
        E = delevent(E);
    }
    destroy_mutex();    
}
