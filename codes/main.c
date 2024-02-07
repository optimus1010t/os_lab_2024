#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define wait_sem(s) semop(s, &pop, 1)
#define signal_sem(s) semop(s, &vop, 1)

int main()
{
    int n;
	int *a, *b;
	int i,j, count = 50, status;

    
	int semid1, semid2 ;
	struct sembuf pop, vop ;
	semid1 = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	semid2 = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	semctl(semid1, 0, SETVAL, 0);
	semctl(semid2, 0, SETVAL, 1);

	pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1;

	if (fork() == 0) {
		FILE *fp;
		int data;

		while (count) {

			wait_sem(semid1);
			fp = fopen("datafile","r");
			fscanf(fp, "%d", &data);
			printf("\t\t\t\t Consumer reads %d\n",data);
			fclose(fp);
			signal_sem(semid2);
			count--;
		}
	}
	else {
		FILE *fp;
		int data = 0;

		while (count) {

			sleep(1);

			wait_sem(semid2);
			fp = fopen("datafile","w");
			fprintf(fp, "%d\n", data);
			printf("Producer writes %d\n", data);
			data++;
			fclose(fp);
			signal_sem(semid1);
			count--;
		}
		wait(&status);
		semctl(semid1, 0, IPC_RMID, 0);
		semctl(semid2, 0, IPC_RMID, 0);
	}
}
