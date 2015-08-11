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
#include <errno.h>
#include <string.h>

//Indices for the shared atomic counters
#define ITER_INDX 0
#define NUMPROC_INDX 1
#define SIG_INDX 2


static inline long long unsigned time_ns(struct timespec* const ts) {
  if (clock_gettime(CLOCK_REALTIME, ts)) {
    exit(1);
  }
  return ((long long unsigned) ts->tv_sec) * 1000000000LLU
    + (long long unsigned) ts->tv_nsec;
}

int main(int argc, char** argv) {

  if(argc!=2){
    printf("Usage: ./atimectxsw <NumProcs>");
    return 0;
  }

  const int numProcs=atoi(argv[1]);
  const int iterations = 5000000;

  //Timer
  struct timespec ts;
  long long unsigned start_ns=0;

  //Set CPU affinity, pin to a single core
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(7, &mask);
  if(sched_setaffinity(0, sizeof(mask), &mask)){
    perror("Sched_Affinity not set");
    exit(1);
  }

  //Set the scheduler to Round Robin
  struct sched_param param;
  //60 is higher than certain IRQs
  param.sched_priority=60;
  if(sched_setscheduler(getpid(), SCHED_RR, &param))
    fprintf(stderr, "sched_setscheduler(): %s\n", strerror(errno));
  
  //Sanity check counter
  //int switches=0;

  //Attach shared memory for atomic counters
  const int shm_id = shmget(IPC_PRIVATE, 3*sizeof(int), IPC_CREAT | 0666);
  int* shmPTR = shmat(shm_id, NULL, 0);
  shmPTR[ITER_INDX]=0;//number of iterations done
  shmPTR[NUMPROC_INDX]=0;//number of procs gathered
  shmPTR[SIG_INDX]=0;//parent signal to children


  //Spawn procs
  pid_t pid=getpid();
  for(int i=0; i<numProcs-1; i++){
  pid = fork();
  if(pid<0)
    exit(1);
  else if(pid==0)
    break;
  }

  //gather all procs at the starting line
  __sync_fetch_and_add(&shmPTR[NUMPROC_INDX], 1);
  if(pid==0){
  //children wait for parent's signal
  while(shmPTR[SIG_INDX]!=1)
    sched_yield();
  }
  else{
  //parent waits for children to gather at starting line
  while(shmPTR[NUMPROC_INDX]!=numProcs)
    sched_yield();
  
  //Everybody here? start timer
  start_ns = time_ns(&ts);
  //signal the children
   __sync_fetch_and_add(&shmPTR[SIG_INDX], 1);
  }
  

  //Increment and yield to context switch onto next proc
  while(shmPTR[ITER_INDX]<iterations){
    __sync_fetch_and_add(&shmPTR[ITER_INDX], 1);
    //switches++;
    sched_yield();
  }


  //Print out number of switches done per process, should be equal
  //printf("Pid:%d Switches:%d\n",getpid(),switches);

  //Children terminate
  if(pid==0)
    return 0;

  //Stop timer
  const long long unsigned delta = time_ns(&ts) - start_ns;

  //Cleanup
  while(wait(NULL)>=0);
  
  printf("Switches=%d, Procs=%d\n", shmPTR[0], shmPTR[1]);
  
  printf("%i context switches in %lluns (%.1fns/syscall)\n",
         iterations, delta, (delta / (float) iterations));

  return 0;
}
