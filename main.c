#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "log.h"
#include "pcb.h"


//global var
int shmid;
int turn_id;
char *fname = "test.out";

//prototype for signal handler
void sig_handler(int signo);
void kill_shm();

int 
main(int argc, char *argv[]){
	//init var
	int c, hflag, sflag, lflag, iflag, tflag;
	
	//char *fname = "test.out";	//for -l switch, default is logfile.txt
	int option_index = 0;		//for getopt_long
	int err_flag = 0;		//for getopt_long
	char *strbuff = "3";		//handling default value for -n

	char *endptr;			//for strtol
	long value;			//for strtol

	int pid = getpid();		//for pid
	int sig_num;			//for signal
	int temp_pid;

	int i;				//process nth number
	long process_num = 5;		//number of process
	char *max_writes = "3";		//maximum number of writing
	long timer = 20;		//time to terminate
	char text[20];

	//int shmid;
	int *shmptr;
	pcb *p;

	signal(SIGINT, sig_handler);	//handling signal

	opterr = 0;

	//initialize flags
	hflag = 0;
	sflag = 0;
	lflag = 0;
	iflag = 0;
	tflag = 0;

	//declare long options
	struct option options[] = {
		{"help", 0, 0, 'h'},
		{0,0,0,0}
	};

	//getopt_long will be used to accept long switch
	//short ones will be h,n,l
	if (argc >1){
		while(( c = getopt_long(argc,argv, ":hs:l:i:t:", options,&option_index)) != -1)
			switch (c)
				{
					//-h, --help for information about switch
					case 'h':
						hflag = 1;
						break;
					//-n to print different integer on the logfile
					case 's':
						sflag = 1;

						//strtol to check int or not
						value = strtol(optarg,&endptr, 10);
						if(*endptr !='\0')
							{
								printf("master : you must type digits after -s, not %s",stderr);
								return 1;
							}
						process_num = value;
						if(process_num > MAXP)
							{
								fprintf(stderr,"master : cannot spawn more than %s processes\n",MAXP);
								return 1;
							}
						break;
					//-l to change logfile name
					case 'l':
						lflag = 1;
						if(strncmp(optarg,"-",1) == 0)
							{
								perror("master : you must type file name after -l switch.");
								return 1;
							}
						fname = (char*) malloc(strlen(optarg)+1);
						strcpy(fname,optarg);
						break;
					//-i switch for max_write
					case 'i':
						iflag = 1;
						value = strtol(optarg,&endptr,10);
						if(*endptr != '\0')
							{
								printf("master you must type digits after -i, not %s",stderr);
								return 1;
							}
						max_writes = (char *)malloc(strlen(optarg)+1);
						strcpy(max_writes,optarg);
						break;
					//-t for timer
					case 't':
						tflag = 1;
						value = strtol(optarg,&endptr,10);
						if(*endptr != '\0')
							{
								printf("master : you must type digits after -t, not %s",stderr);
								return 1;
							}
						timer = value;
						break;
					//errors in getopt
					case '?':
						err_flag = 1;
						printf("Unknown option : -%c is found\n", optopt);
						break;
				}
	}
	//if errflag is on, just exit
	if (err_flag ==1)
		{
			return 1;
		}

	//if iflag is on
	//print simple definition and then term
	if (hflag == 1)
		{
			printf("this program is designed for testing log library function\n");
			printf("-h --help\n print options\n");
			printf("-s [value]\n to spawn [value] of slave processes\n");
			printf("-l [filename]\n change logfile name to [filename]\n");
			printf("-i [value]\n change maximum number of writing for slave process to [value]\n");
			printf("-t [value]\n change the termination timer for master process to [value] second\n");
			return 0;
		}

	//shmget to create shared memories
	//first one for counting
	//second for the algorithm
        if ((shmid = shmget((key_t)12348888, sizeof(int), 0600|IPC_CREAT)) == -1)
                {
                        perror("master : fail to create a shared memory\n");
                        create_log("master : fail to create a shared memory\n");
                        savelog(fname);
                        return 1;
                }
        else
                {
                        create_log("master : created a shared memory");
                        savelog(fname);
                        clearlog();
                }

        if ((turn_id = shmget((key_t)88881234, sizeof(int)*20, 0600|IPC_CREAT)) == -1)
                {
                        perror("master : fail to create a shared memory for the peterson algorithm\n");
                        create_log("master : fail to create a shared memory for the peterson algorithm\n");
                        savelog(fname);
                        return 1;
                }
        else
                {
                        create_log("master : created a second shared memory");
                        savelog(fname);
                        clearlog();
                }

	//attatching shared memories
        if ((shmptr = shmat (shmid, NULL, 0)) == -1)
                {
                        perror("master : fail to attatch the shared memory\n");
                        create_log("master : fail to attatch the shared memory");
                        savelog(fname);
                        return 1;
                }
        else
                {
                        create_log("master : success to attatch the shared memory");
                        savelog(fname);
			clearlog();
                }

        if ((p = (pcb *)shmat(turn_id,NULL,0)) == -1)
                {
                        perror("master : fail to attatch the second shared memory\n");
                        create_log("master : fail to attatch the second shared memory");
                        savelog(fname);
                        return 1;
                }
        else
                {
                        create_log("master : success to attatch the second shared memory");
                        savelog(fname);
			clearlog();
                }



	//init pcb
	int k;
	for (k = 0; k<MAXP; k++)
		{
			p->pid[k] = -1;
			p->flag[k] = 0;
		}

	//assign master process to 0th slot
	p->pid[0] = pid;
	p->num_proc = process_num+1;
	i = 0;
	
	//initialize shmptr to 0
	int *cal_num;
	cal_num = (int *) shmptr;
	*cal_num = 0;

	//forking
	for(k = 1; k<process_num+1; k++)
		{
			fflush(stdout);
			create_log("master : forking a child process");
			savelog(fname);
			clearlog();
			temp_pid = fork();
			if (temp_pid == -1)
				{
					perror("master : fail to fork a child process\n");
					create_log("master : fail to fork a child process");
					savelog(fname);
					return 1;
				}
			else if(temp_pid == 0)
				{
					if (lflag == 1)
						{
							snprintf(text,sizeof(text), "%d", k);
							//
							if((execl("./slave", "slave",text, "-i", max_writes, "-l", fname, NULL)) == -1)
								{
									perror("slave : fail to exec the process image\n");
									create_log("slave : fail to exec the process image");
									savelog(fname);
									return 1;
								}
						}
					else
						{
							snprintf(text,sizeof(text), "%d", k);
							//place for child
							//exec
							if((execl("./slave", "slave",text , "-i", max_writes,"-l",fname, NULL)) == -1)
								{
									//getting error 
									perror("slave : fail to exec the process image\n");
									create_log(" slave: fail to exec the process image");
									savelog(fname);
									return 1;
								}
						}
				}
			else
				{
					p->pid[k] = temp_pid;
				}
		}

	//sleep
	sleep(timer);
	//do termination
	for (k = 1; k < process_num+1;k++)
		{
			kill(p->pid[k], SIGKILL);
		}
	kill_shm();
	create_log("master : terminate program");
	savelog(fname);
	return 0;
}

//signal handler
void 
sig_handler(int signo)
{
	fprintf(stderr, "master : receive a signal to terminate");
	create_log("master : receive a signal to terminate");
	savelog(fname);
	clearlog();
	kill_shm();
	sleep(1);
	exit (1);
}

//function for killing a shared memory
void 
kill_shm()
{
	if((shmctl(shmid, IPC_RMID, 0)) == -1)
		{
			perror("master : fail to kill the shared memory\n");
			create_log("master : fail to kill the shared memory");
			savelog(fname);
			clearlog();
		}
	else
		{
			fprintf(stderr,"master : success to kill the shared memory\n");
			create_log("master : success to kill the shared memory");
			savelog(fname);
			clearlog();
		}
	if((shmctl(turn_id, IPC_RMID, 0)) == -1)
		{
			perror("master : fail to kill the second shared memory\n");
			create_log("master : fail to kill the second shared memory");
			savelog(fname);
			clearlog();
		}
	else
		{
			fprintf(stderr,"master : success to kill the second shared memory\n");
			create_log("master : success to kill the second shared memory");
			savelog(fname);
			clearlog();
		}
}
