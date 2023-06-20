#ifndef PCB_H
#define PCB_H

#define MAXP 19
#define true 1
#define false 0
#define idle 0
#define want_in 1
#define in_cs 2

typedef struct pcb_ {
	int pid [MAXP];
	int flag[MAXP];
	int turn;
	int num_proc;
}pcb;

#define max(X,Y) ((X) < (Y) ? (X) : (Y))

#endif /* PCB_H */
