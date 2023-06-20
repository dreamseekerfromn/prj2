#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "log.h"
#include "pcb.h"

//global var
int shmid;
int turn_id;
char *fname = "test.out";
int proc_num;

//prototype for signal handler
void sig_handler(int signo);
void kill_shm();

int 
main(int argc, char *argv[]){
	fflush(stdout);
	//init var
	int c, iflag, lflag, err_flag;
	//char *fname = "test.out";	//for -l switch, default is logfile.txt
	char str[256];			//for strtol
	char *strbuff = "3";		//handling default value for -n
	
	char *endptr;			//for strtol
	long value;			//for strtol

	int pid = getpid();		//for pid
	int sig_num;			//for signal

	int k;				//loop
	int i = atoi(argv[1]);		//will store process #
	int max_writing;		//max writes
	int *num;

	int option_index = 0;
	opterr = 0;
	iflag = 0;
	lflag = 0;
	err_flag = 0;

	proc_num = i;

	//time info
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	//seed random
	srand((unsigned) time(&rawtime));

	//int shmid;
	int *shmptr;
	pcb *p;

	signal(SIGINT, sig_handler);	//handling signal

        //using atoi b/c master always pass the right type. free to use atoi w/o additional checking
        max_writing = atoi(argv[3]);
        fname = (char *)malloc(strlen(argv[5])+1);
        strcpy(fname,argv[5]);
        

	//create a shared memory
	if ((shmid = shmget((key_t)12348888, sizeof(int), 0600)) == -1)
		{
			perror("slave : fail to get a shared memory\n");
			snprintf(str,sizeof(str),"slave %d : fail to get a shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"slave %d : got a shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			clearlog();
			memset(str,0,sizeof(str));
		}

	if ((turn_id = shmget((key_t)88881234, sizeof(int)*20, 0600)) == -1)
		{
			perror("slave : fail to get a shared memory for the peterson algorithm\n");
			snprintf(str,sizeof(str),"slave %d : fail to get a shared memory for the peterson algorithm\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"slave%d : get a second shared memory\n", proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();

		}

	//attatching shared memory
	//first shm for data
	if ((shmptr = shmat (shmid, NULL, 0)) == -1)
		{
			perror("slave : fail to attatch the shared memory\n");
			snprintf(str,sizeof(str),"slave %d : fail to attatch the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"slave %d : success to attatch the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();
		}
	//second for the algorithm
	if ((p = (pcb *)shmat(turn_id,NULL,0)) == -1)
		{
			perror("slave : fail to attatch the second shared memory\n");
			snprintf(str,sizeof(str),"slave %d : fail to attatch the second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"slave %d : success to attatch the second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			savelog(fname);
			memset(str,0,sizeof(str));
			clearlog();
		}

	//num will handle shm
	num = (int *)shmptr;

	//bakery
	for (k = 0; k <max_writing; k++)
		{
			int j;
			do
				{
					p->flag[i] = want_in;
					j = p->turn;
					while(j !=i)
						j=(p->flag[j] != idle)?p->turn : (j+1)%p->num_proc;
					p->flag[i] = in_cs;
					for(j = 0; j<p->num_proc;j++)
						if((j!=i) && (p->flag[j] == in_cs))
							break;
				}while((j<p->num_proc)||(p->turn != i && p->flag[p->turn] != idle));
			p->turn = i;

			//critical section
			sleep(rand()%3);
			*num=*num+1;
			fprintf(stderr,"File modified by process number %d at time %s with sharedNum = %d\n",i,asctime(timeinfo),*num);
			snprintf(str,sizeof(str),"File modified by process number %d,  with sharedNum = %d",i,*num);

			//fprintf(stderr,"shared memory modified by %d th process, %d th modification, at time %s",i,*num,asctime(timeinfo));
			//snprintf(str,sizeof(str), "slave : shared memory modifed by %dth process, %dth modification", i,*num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
			sleep(rand()%3);
			//critical section end
			j = (p->turn +1) % p->num_proc;
			while (p->flag[j] == idle)
				j = (j+1) %p->num_proc;
			p->turn = j;
			p->flag[i] = idle;
			//remainder
		}
	if(shmdt(shmptr) == -1)
		{
			perror("slave : shmdt failed\n");
			snprintf(str,sizeof(str),"slave %d : shmdt failed\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			return 1;
		}
	else
		{
			snprintf(str,sizeof(str),"slave %d : shmdt success\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}

	savelog(fname);
	return 0;
}

//signal handler
void 
sig_handler(int signo)
{
	char str[256];
	fprintf(stderr,"slave : receive a signal,terminating\n",proc_num);
	snprintf(str,sizeof(str),"slave %d : receive a signal, terminating\n",proc_num);
	fflush(stdout);
	create_log(str);
	memset(str,0,sizeof(str));
	kill_shm();
	savelog(fname);
	sleep(1);
	exit (1);
}

//function for killing a shared memory
void 
kill_shm()
{
	char str[256];
	if((shmctl(shmid, IPC_RMID, 0)) == -1)
		{
			perror("slave : fail to kill the shared memory\n");
			snprintf(str,sizeof(str),"slave %d : fail to kill the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
	else
		{
			fprintf(stderr,"slave %d: success to kill the shared memory\n",proc_num);
			snprintf(str,sizeof(str),"slave %d : success to kill the shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
	if((shmctl(turn_id, IPC_RMID, 0)) == -1)
		{
			perror("slave : fail to kill the second shared memory\n");
			snprintf(str,sizeof(str),"slave %d : fail to kill the second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
	else
		{
			fprintf(stderr,"slave %d: success to kill the second shared memory\n",proc_num);
			snprintf(str,sizeof(str),"slave %d : success to kill second shared memory\n",proc_num);
			fflush(stdout);
			create_log(str);
			memset(str,0,sizeof(str));
			savelog(fname);
			clearlog();
		}
}
