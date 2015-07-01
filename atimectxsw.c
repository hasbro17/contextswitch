#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static inline long long unsigned time_ns(struct timespec* const ts) {
  if (clock_gettime(CLOCK_REALTIME, ts)) {
    exit(1);
  }
  return ((long long unsigned) ts->tv_sec) * 1000000000LLU
    + (long long unsigned) ts->tv_nsec;
}

int main(int argc, char** argv) {

  if(argc!=2)
  {
	printf("Usage: ./atimectxsw <NumProcs>");
	return 0;
  }

  const int numProcs=atoi(argv[1]);

  const int iterations = 5000000;
  struct timespec ts;
  long long unsigned start_ns=0;
  
  pid_t pid=getpid();
  pid_t myPid=getpid();

  //Attach Shared Memory for counters
  const int shm_id = shmget(IPC_PRIVATE, 3*sizeof(int), IPC_CREAT | 0666);
  int* shmPTR = shmat(shm_id, NULL, 0);
  shmPTR[0]=0;//number of iterations done
  shmPTR[1]=0;//number of procs gathered
  shmPTR[2]=0;//parent signal to children


  for(int i=0; i<numProcs-1; i++){
	pid = fork();
	if(pid<0)
		exit(1);
	else if(pid==0)
		break;
  }

  myPid=getpid();

  __sync_fetch_and_add(&shmPTR[1], 1);
  //gather all procs at the starting line
  if(pid==0){
	//children wait for parent to signal
	while(shmPTR[2]!=1)
		sched_yield();
  }
  else{
	//parent wait for children to gather
	while(shmPTR[1]!=numProcs)
		sched_yield();
	//start timer
	start_ns = time_ns(&ts);
	//signal the children
	 __sync_fetch_and_add(&shmPTR[2], 1);
  }
  
  //Increment and yield to context switch to next proc
  while(shmPTR[0]<iterations){
	__sync_fetch_and_add(&shmPTR[0], 1);
	printf("pid:%d\n",myPid);
	sched_yield();
  }

  if(pid==0)
	return 0;
  
  const long long unsigned delta = time_ns(&ts) - start_ns;

  while(wait(NULL)>=0);
  
  printf("Switches=%d, Procs=%d\n", shmPTR[0], shmPTR[1]);
  
  printf("%i system calls in %lluns (%.1fns/syscall)\n",
         iterations, delta, (delta / (float) iterations));
  return 0;
}
