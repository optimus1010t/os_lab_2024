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
int is_idle = 1;

int time_value = 0;

typedef struct {
    int tno;
    int ttime;
} tinfo;

pthread_mutex_t doc_mutex;
pthread_mutex_t patient_mutex;
pthread_mutex_t salesrep_mutex;
pthread_mutex_t reporter_mutex;

pthread_mutex_t count_mutex;
pthread_mutex_t time_mutex;
pthread_mutex_t is_idle_mutex;

pthread_mutex_t doc_attending;
pthread_mutex_t doc_done;


pthread_mutex_t ass_update_tok;

pthread_barrier_t rep_barrier;
pthread_barrier_t pat_barrier;
pthread_barrier_t sal_barrier;

pthread_cond_t doc_cond;
pthread_cond_t patient_cond;
pthread_cond_t salesrep_cond;
pthread_cond_t reporter_cond;


void* patient(void* arg) {
    pthread_mutex_lock(&patient_mutex);
    pthread_barrier_wait(&pat_barrier);
    pthread_mutex_lock(&doc_attending);
    printf("[%d] Doctor has next visitor\n", time_value);  
    pthread_mutex_lock(&time_mutex);
    time_value += ((tinfo *)arg)->ttime;
    printf("\t[%d - %d] Patient %d is in doctor's chamber\n", (time_value-((tinfo *)arg)->ttime), time_value, ((tinfo *)arg)->tno);
    pthread_mutex_unlock(&time_mutex);
    pthread_mutex_lock(&count_mutex);
    patients_done++;
    pthread_mutex_unlock(&count_mutex);
    pthread_barrier_init(&pat_barrier, NULL, 2);
    printf("asdasdasdsplpp\n");
    pthread_mutex_unlock(&doc_done);
    pthread_mutex_unlock(&patient_mutex);
}

void* salesrep(void* arg) {
    pthread_mutex_lock(&salesrep_mutex);
    pthread_barrier_wait(&sal_barrier);
    pthread_mutex_lock(&doc_attending);
    printf("[%d] Doctor has next visitor\n", time_value);  
    pthread_mutex_lock(&time_mutex);
    time_value += ((tinfo *)arg)->ttime;
    printf("\t[%d - %d] Salesrep %d is in doctor's chamber\n", (time_value-((tinfo *)arg)->ttime), time_value, ((tinfo *)arg)->tno);
    pthread_mutex_unlock(&time_mutex);
    pthread_mutex_lock(&count_mutex);
    salesreps_done++;
    pthread_mutex_unlock(&count_mutex);
    pthread_barrier_init(&sal_barrier, NULL, 2);
    pthread_mutex_unlock(&doc_done);
    pthread_mutex_unlock(&salesrep_mutex);
}

void* reporter(void* arg) {
    pthread_mutex_lock(&reporter_mutex);
    pthread_barrier_wait(&rep_barrier);
    pthread_mutex_lock(&doc_attending);
    printf("[%d] Doctor has next visitor\n", time_value);  
    pthread_mutex_lock(&time_mutex);
    time_value += ((tinfo *)arg)->ttime;
    printf("\t[%d - %d] Reporter %d is in doctor's chamber\n", (time_value-((tinfo *)arg)->ttime), time_value, ((tinfo *)arg)->tno);
    pthread_mutex_unlock(&time_mutex);
    pthread_mutex_lock(&count_mutex);
    reporters_done++;
    pthread_mutex_unlock(&count_mutex);
    pthread_barrier_init(&rep_barrier, NULL, 2);
    pthread_mutex_unlock(&doc_done);
    pthread_mutex_unlock(&reporter_mutex);    
}

void* doctor(void* arg) {
    while (1) {
        pthread_mutex_lock(&doc_mutex);
        printf("doctorrrrrr\n");
        pthread_mutex_lock(&count_mutex);
        if (patients_done == MAX_PATIENTS && salesreps_done == MAX_SALESREP) {
            pthread_mutex_unlock(&count_mutex);
            break;
        }
        if (token_reporter > reporters_done) {
            pthread_mutex_unlock(&count_mutex);
            pthread_mutex_lock(&is_idle_mutex);
            is_idle = 0;
            pthread_mutex_unlock(&is_idle_mutex);
            pthread_barrier_wait(&rep_barrier);
            pthread_mutex_unlock(&doc_attending);
            printf("fgppooooo090909\n");
            pthread_mutex_lock(&doc_done);
            // pthread_mutex_unlock(&ass_update_tok);
        }
        else if (token_patient > patients_done) {
            // printf("sasdasd\n");
            pthread_mutex_unlock(&count_mutex);
            // printf("sasdassdfsdfdfd\n");
            pthread_mutex_lock(&is_idle_mutex);
            // printf("sasqreyreydasd\n");
            is_idle = 0;
            pthread_mutex_unlock(&is_idle_mutex);
            // printf("ASdasdasdfg\n");
            pthread_barrier_wait(&pat_barrier);
            // printf("ASdasdasdfdfghjklg\n");
            pthread_mutex_unlock(&doc_attending);
            printf("fgppooooo\n");
            pthread_mutex_lock(&doc_done);
            printf("sdasdakjljkljl\n");
            // pthread_mutex_unlock(&ass_update_tok);
        }
        else if (token_salesrep > salesreps_done) {
            pthread_mutex_unlock(&count_mutex);
            pthread_mutex_lock(&is_idle_mutex);
            is_idle = 0;
            pthread_mutex_unlock(&is_idle_mutex);
            pthread_barrier_wait(&sal_barrier);
            pthread_mutex_unlock(&doc_attending);
            pthread_mutex_lock(&doc_done);
            // pthread_mutex_unlock(&ass_update_tok);
        } else {
            pthread_mutex_unlock(&count_mutex);
            pthread_mutex_lock(&is_idle_mutex);
            is_idle = 1;
            pthread_mutex_unlock(&is_idle_mutex);
        }
        
        // printf("[%d] Doctor has next visitor\n", time_value);        
    }
    printf("Doctor leaves (session over)\n");
}

void create_mutex() {
    pthread_mutex_init(&doc_mutex, NULL); pthread_mutex_trylock(&doc_mutex);
    pthread_mutex_init(&patient_mutex, NULL); pthread_mutex_trylock(&patient_mutex); pthread_mutex_unlock(&patient_mutex);
    pthread_mutex_init(&salesrep_mutex, NULL); pthread_mutex_trylock(&salesrep_mutex); pthread_mutex_unlock(&salesrep_mutex);
    pthread_mutex_init(&reporter_mutex, NULL); pthread_mutex_trylock(&reporter_mutex); pthread_mutex_unlock(&reporter_mutex);
    pthread_mutex_init(&count_mutex, NULL); pthread_mutex_trylock(&count_mutex); pthread_mutex_unlock(&count_mutex);
    pthread_mutex_init(&time_mutex, NULL); pthread_mutex_trylock(&time_mutex); pthread_mutex_unlock(&time_mutex);
    pthread_mutex_init(&is_idle_mutex, NULL); pthread_mutex_trylock(&is_idle_mutex); pthread_mutex_unlock(&is_idle_mutex);
    pthread_mutex_init(&doc_attending, NULL); pthread_mutex_trylock(&doc_attending); // pthread_mutex_unlock(&doc_attending);
    pthread_mutex_init(&doc_done, NULL); pthread_mutex_trylock(&doc_done); // pthread_mutex_unlock(&doc_done);
    pthread_mutex_init(&ass_update_tok, NULL); pthread_mutex_trylock(&ass_update_tok); // pthread_mutex_unlock(&ass_update_tok);
    
    pthread_barrier_init(&rep_barrier, NULL, 2);
    pthread_barrier_init(&pat_barrier, NULL, 2);
    pthread_barrier_init(&sal_barrier, NULL, 2);
    
    pthread_cond_init(&doc_cond, NULL);
    pthread_cond_init(&patient_cond, NULL);
    pthread_cond_init(&salesrep_cond, NULL);
    pthread_cond_init(&reporter_cond, NULL);
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
            pthread_mutex_lock(&is_idle_mutex);
            if (e.time > time_value) {
                if (is_idle == 0) {
                    pthread_mutex_unlock(&is_idle_mutex);
                    continue;
                } else if (e.time > time_value && is_idle == 1) {
                    pthread_mutex_unlock(&is_idle_mutex);
                    pthread_mutex_lock(&count_mutex);
                    if (patients_done < token_patient || salesreps_done < token_salesrep || reporters_done < token_reporter) {
                        pthread_mutex_unlock(&count_mutex);
                        pthread_mutex_unlock(&doc_mutex);
                        continue;
                    }
                    pthread_mutex_unlock(&count_mutex);
                    pthread_mutex_lock(&time_mutex);
                    time_value = e.time;
                    pthread_mutex_unlock(&time_mutex);
                }
            } else {
                pthread_mutex_unlock(&is_idle_mutex);            
            }
        } else if (e.time >= 0){
            pthread_create(&tid_doc, &attr, doctor, (void *)&param_doc);
            count++;
            continue;
        }
        // pthread_mutex_unlock(&ass_update_tok);
        if (e.type == 'P') printf("\t[%d] Patient %d arrives\n", e.time, token_patient + 1);
        else if (e.type == 'S') printf("\t[%d] Salesrep %d arrives\n", e.time, token_salesrep+1);
        else if (e.type == 'R') printf("\t[%d] Reporter %d arrives\n", e.time, token_reporter+1);
        else printf("Invalid event type\n");
        if (e.type == 'P') {
            if (token_patient < MAX_PATIENTS) {
                arg_pat[token_patient].ttime = e.duration;
                pthread_create(&tid_patient[token_patient], &attr, patient, (void *)&arg_pat[token_patient]);
            }
            else printf("\tPatient %d leaves (quota full)\n", token_patient+1);
            token_patient++;
        }
        else if (e.type == 'S') {
            if (token_salesrep < MAX_SALESREP) {
                arg_sal[token_salesrep].ttime = e.duration;
                pthread_create(&tid_salesrep[token_salesrep], &attr, salesrep, (void *)&arg_sal[token_salesrep]);
            }
            else printf("\tSalesrep %d leaves (quota full)\n", token_salesrep+1);
            token_salesrep++;
        }
        else if (e.type == 'R') {
            if (patients_done != MAX_PATIENTS && salesreps_done != MAX_SALESREP) {
                arg_rep[token_reporter].ttime = e.duration;
                pthread_create(&tid_rep[token_reporter], &attr, reporter, (void *)&arg_rep);
            }
            else printf("\tReporter %d leaves (session over)\n", token_reporter+1);
            token_reporter++;
        }
        else printf("Invalid event type\n");
        E = delevent(E);       
    }

    while (!emptyQ(E)) {
        event e = nextevent(E);
        if (e.type == 'P') {
            printf("\tPatient %d arrives\n", token_patient+1);
            printf("\tPatient %d leaves (session over)\n", token_patient+1);
            token_patient++;
        }
        else if (e.type == 'S') {
            printf("\tSalesrep %d arrives\n", token_salesrep+1);
            printf("\tSalesrep %d leaves (session over)\n", token_salesrep+1);
            token_salesrep++;
        }
        else if (e.type == 'R') {
            printf("\tReporter %d arrives\n", token_reporter+1);
            printf("\tReporter %d leaves (session over)\n", token_reporter+1);
            token_reporter++;
        }
        else {
            printf("Invalid event type\n");
        }
    }
    
}
