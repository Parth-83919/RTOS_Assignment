#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sched.h>
#include<signal.h>
#include<unistd.h>
#include<sys/shm.h>
#include<sys/wait.h>
#include<pthread.h>
typedef struct shm {
    pthread_mutex_t m;
    pthread_mutexattr_t ma;
    pthread_cond_t c;
    pthread_condattr_t ca;
    int wakeup;
} shm_t;

shm_t *ptr;

void task_func(int cnt) {
    int i;

    pthread_mutex_lock(&ptr->m);
    while(ptr->wakeup == 0)
        pthread_cond_wait(&ptr->c, &ptr->m);
    pthread_mutex_unlock(&ptr->m);

    for(i=1; i<=cnt; i++) {
        printf("task (%d) : %d\n", getpid(), i);
    }

    shmdt(ptr);
    _exit(0);
}

int main(void){

	int ret1, ret2, ret3,s,shmid;
	shmid = shmget(0x1234, sizeof(shm_t), IPC_CREAT | 0600);
	if(shmid < 0){
		perror("shmget() failed");
	}

	ptr = shmat(shmid, NULL, 0);
	if(ptr == (void*)-1){
		perror("shmctl() failed");
		shmctl(shmid, IPC_RMID, NULL);
		_exit(2);
	}

	ptr->wakeup =0;
	pthread_mutexattr_init(&ptr->ma);
	pthread_mutexattr_setpshared(&ptr->ma, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&ptr->m, &ptr->ma);

	pthread_condattr_init(&ptr->ca);
	pthread_condattr_setpshared(&ptr->ca, PTHREAD_PROCESS_SHARED);
	pthread_cond_init(&ptr->c, &ptr->ca);

	shmctl(shmid, IPC_RMID, NULL);
	ret1 = fork();
	if(ret1 == 0){
		int pid1 = getpid();
		struct sched_param s1;
		s1.sched_priority = 10;
		int sch1 = sched_setscheduler(pid1, SCHED_FIFO, &s1);
		if(sch1 < 0){
			perror("sch1 failed");
			_exit(1);
		}
		printf("child1: %d\n",pid1);
	//	for(int i=0;i<5;i++){
	//		printf("child 1 : %d\n",i);
	//	}
		task_func(5);
	}

	ret2 = fork();
	if(ret2 == 0){
		int pid2 = getpid();
		struct sched_param s2;
		s2.sched_priority = 11;
       	int sch2 = sched_setscheduler(pid2, SCHED_FIFO, &s2);
		if(sch2 < 0){
			perror("sch2 failed");
			_exit(1);
		}
		printf("child2: %d\n",pid2);
		task_func(5);
	//	for(int i=0;i<5;i++){
	//		printf("child 2 : %d\n",i);
	//	}
	}

	ret3 = fork();
	if(ret3 == 0){
		int pid3 = getpid();
        struct sched_param s3;
		s3.sched_priority = 13;
		int sch3 = sched_setscheduler(pid3, SCHED_FIFO, &s3);
		if(sch3 < 0){
			perror("sch3 failed");
			_exit(1);
		}
		printf("child3: %d\n",pid3);
		task_func(5);
	//	for(int i=0;i<5;i++){
	//		printf("child 3 : %d\n",i);
		//}
	}

	printf("Enter ");
	getchar();

	ptr->wakeup = 1;
	pthread_cond_broadcast(&ptr->c);

	waitpid(ret1,&s,0);
	waitpid(ret2,&s,0);
	waitpid(ret3,&s,0);

	printf("bye\n");
	pthread_cond_destroy(&ptr->c);
	pthread_mutex_destroy(&ptr->m);
	shmdt(ptr);
	return 0;
}

