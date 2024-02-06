#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

int main()
{

	int shmid, status;
	int i, n, t;
	shmid = shmget(IPC_PRIVATE, 2*sizeof(int), 0777|IPC_CREAT);
	printf("n (the number of consumers) = ");
	scanf("%d",&n);
	printf("t (the number of items to be produced) = ");
	scanf("%d",&t);

	printf("Producer is alive\n");
	int con_pids[n+1];
	int item_counts[n+1];
	// set all element to zero in items
	for (i=0; i<n+1; i++) {
		item_counts[i] = 0;
	}
	int items[n+1][t];
	for (i=0; i<n+1; i++) {
		for (int j=0; j<t; j++) {
			items[i][j] = 0;
		}
	}
	int *M;
	M = (int *) shmat(shmid, 0, 0);
	M[0] = 0;
	for (i=1; i<n+1; i++) {
		int pid;
		if ((pid = fork()) == 0) {
			printf("\t\t\t\t\t\t\t\tConsumer %d is alive\n",i);
			int *m = (int *) shmat(shmid, 0, 0);
			int item_child[t],count_child = 0;
			for (int j=0; j<t; j++) {
				item_child[j] = 0;
			}
			while (1) {
				while (m[0] != -1 && m[0] != i);
				if (m[0] == -1){
					break;
				}
				if (m[0] == i) {
					#ifdef VERBOSE
					printf("\t\t\t\t\t\t\t\tConsumer %d reads %d\n\n",i,m[1]);
					#endif
					item_child[count_child] = m[1];
					count_child++;
					m[0] = 0;
				}
			}
			int sum = 0;
			for (int j=0; j<count_child; j++) {
				sum += item_child[j];
			}
			printf("\t\t\t\t\t\t\t\tConsumer %d has read %d items: Checksum = %d\n",i,count_child,sum);
			shmdt(m);
			exit(0);
		}
		else {
			con_pids[i] = pid;
		}
	}
	for (i=1; i<=t; i++){
		while (M[0] != 0);
		int item = rand() % 900 + 100;
		int consumer = rand() % n + 1;
		#ifdef VERBOSE
			printf("Producer produces %d for Consumer %d\n",item, consumer);
		#endif
		M[0] = consumer;
		#ifdef SLEEP
			usleep(10);
		#endif
		M[1] = item;
		items[consumer][item_counts[consumer]] = item;
		item_counts[consumer]++;
	}
	while (M[0] != 0);
	M[0] = -1;
	for (i=1; i<n+1; i++) {
		waitpid(con_pids[i], &status, 0);
	}
	printf("Producer has produced %d items\n",t);
	for (i=1; i<n+1; i++) {
		int sum = 0;
		for (int j=0; j<item_counts[i]; j++) {
			sum += items[i][j];
		}
		printf("%d items for Consumer %d: Checksum = %d\n",item_counts[i],i,sum);
	}
	shmdt(M);
	shmctl(shmid, IPC_RMID, 0);
}
