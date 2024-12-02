#include <linux/sem.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define THREAD_NUM 5

int total_tickets = 100; // 总票数
int sold_tickets = 0;    // 已售票数
int mutex;               // 互斥信号量

// 定义信号量P操作
void P(int semid, int index) {
  struct sembuf sem;
  sem.sem_num = index;
  sem.sem_op = -1;
  sem.sem_flg = 0;       // 操作标记：0或IPC_NOWAIT等
  semop(semid, &sem, 1); // 1:表示执行命令的个数
  return;
}

// 定义信号量V操作
void V(int semid, int index) {
  struct sembuf sem;
  sem.sem_num = index;
  sem.sem_op = 1;
  sem.sem_flg = 0;
  semop(semid, &sem, 1);
  return;
}

// 售票线程函数
void *sell_ticket(void *arg) {
  // 因为pthread_create给的job函数类型是硬编码的，因为需要对参数做处理，此处使用int*保存转化
  int thread_id = *(int *)arg; // 线程ID
  int local_sold = 0;          // 当前线程售出的票数

  while (1) {
    P(mutex, 0); // 进入临界区
    if (sold_tickets < total_tickets) {
      sold_tickets++;
      local_sold++;
      printf("Thread %d sold %d ticket. Total sold: %d\n", thread_id,
             local_sold, sold_tickets);
    } else {
      V(mutex, 0); // 离开临界区
      printf(
          "Thread %d finished selling. Total tickets sold by this thread: %d\n",
          thread_id, local_sold);
      break;
    }
    V(mutex, 0); // 离开临界区
  }

  free(arg); // 释放动态分配的线程ID内存
  return NULL;
}

int main() {
  // 创建信号量
  mutex = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
  if (mutex == -1) {
    perror("Failed to create semaphore");
    exit(EXIT_FAILURE);
  }

  union semun  sem_arg;
  sem_arg.val = 1; // 初始化信号量为1,使得第一次请求mutex可以正常获得
  if (semctl(mutex, 0, SETVAL, sem_arg) == -1) {
    perror("Failed to initialize semaphore");
    exit(EXIT_FAILURE);
  }

  pthread_t threads[THREAD_NUM];
  for (int i = 0; i < THREAD_NUM; i++) {
    int *thread_id = malloc(sizeof(int)); // 分配线程ID的内存
    if (!thread_id) {
      perror("Failed to allocate memory for thread ID");
      exit(EXIT_FAILURE);
    }
    *thread_id = i + 1;
    if (pthread_create(&threads[i], NULL, sell_ticket, thread_id) != 0) {
      perror("Failed to create thread");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < THREAD_NUM; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("Failed to join thread");
      exit(EXIT_FAILURE);
    }
  }

  // 删除信号量
  if (semctl(mutex, 0, IPC_RMID) == -1) {
    perror("Failed to remove semaphore");
  }

  printf("All tickets sold. Program finished.\n");
  return 0;
}
