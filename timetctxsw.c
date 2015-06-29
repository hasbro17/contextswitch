#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <linux/futex.h>

static inline long long unsigned time_ns(struct timespec* const ts) {
  if (clock_gettime(CLOCK_REALTIME, ts)) {
    exit(1);
  }
  return ((long long unsigned) ts->tv_sec) * 1000000000LLU
    + (long long unsigned) ts->tv_nsec;
}

static const int iterations = 500000;

static void* thread(void* ftx) {
  int* futex = (int*) ftx;
  for (int i = 0; i < iterations; i++) {
    sched_yield();
    while (syscall(SYS_futex, futex, FUTEX_WAIT, 0xA, NULL, NULL, 42)) {
      // retry
      sched_yield();
    }
    *futex = 0xB;
    while (!syscall(SYS_futex, futex, FUTEX_WAKE, 1, NULL, NULL, 42)) {
      // retry
      sched_yield();
    }
  }
  return NULL;
}

int main(void) {
  struct timespec ts;
  const int shm_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666);
  int* futex = shmat(shm_id, NULL, 0);
  pthread_t thd;
  if (pthread_create(&thd, NULL, thread, futex)) {
    return 1;
  }
  *futex = 0xA;

  const long long unsigned start_ns = time_ns(&ts);
  for (int i = 0; i < iterations; i++) {
    *futex = 0xA;
    while (!syscall(SYS_futex, futex, FUTEX_WAKE, 1, NULL, NULL, 42)) {
      // retry
      sched_yield();
    }
    sched_yield();
    while (syscall(SYS_futex, futex, FUTEX_WAIT, 0xB, NULL, NULL, 42)) {
      // retry
      sched_yield();
    }
  }
  const long long unsigned delta = time_ns(&ts) - start_ns;

  //Context switch=4*iterations.
  const int nswitches = iterations << 2;
  printf("%i  thread context switches in %lluns (%.1fns/ctxsw)\n",
         nswitches, delta, (delta / (float) nswitches));
  wait(futex);
  return 0;
}
