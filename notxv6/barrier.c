#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;
static int round = 0;

struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

static void barrier()
{
    // 锁定互斥量，确保对共享变量 bstate 的访问是线程安全的
    pthread_mutex_lock(&bstate.barrier_mutex);
    
    // 将 bstate.nthread 加 1，表示当前线程已经到达了障碍点
    bstate.nthread++;
    
    // 检查是否所有线程都已经到达障碍点
    if (bstate.nthread == nthread) {
        // 如果所有线程都已到达，将 nthread 重置为 0，为下一轮的同步做准备
        bstate.nthread = 0;
        
        // 更新回合数，表示一个新的同步回合开始
        bstate.round++;
        
        // 唤醒所有因等待条件变量而阻塞的线程
        pthread_cond_broadcast(&bstate.barrier_cond);
    } else {
        // 如果不是所有线程都到达障碍点，则当前线程等待，直到其他线程到达
        pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
    }
    
    // 解锁互斥量，允许其他线程访问共享变量
    pthread_mutex_unlock(&bstate.barrier_mutex);
}


static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
