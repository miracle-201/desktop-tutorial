#include <linux/sem.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

int a;
//读写交替类，但有两个读，所以需要两个信号量来通知
int semid; // 信号灯
// 0指示是否可以计算写入a
// 1指示打印机1是否可以工作
// 2指示打印机2是否可以工作


// P操作定义
void P(int semid, int index) {
  struct sembuf sem;
  sem.sem_num = index;
  sem.sem_op = -1;
  sem.sem_flg = 0;       // 操作标记：0或IPC_NOWAIT等
  semop(semid, &sem, 1); // 1：表示执行命令的个数
  return;
}
// V操作定义
void V(int semid, int index) {
  struct sembuf sem;
  sem.sem_num = index;
  sem.sem_op = 1;
  sem.sem_flg = 0;
  semop(semid, &sem, 1);
  return;
}

void *Calculate() {
  a = 0;
  int i;
  for (i = 1; i <= 100; i++) {
    P(semid, 0);
    a = a + i;
    if (a % 2 == 1) {
      V(semid, 2);
    } else {
      V(semid, 1);
    }
  }
  return;
}
//结束：一种硬编码，一种设定一个全局finish的bool量，每次判断下
void *PrintNum1() {
  while (1) {
    P(semid, 1);
    if (a != 5050) {
      printf("Pinter 1: a = %d\n", a);
      V(semid, 0);
    } else {
      printf("Pinter 1: a = %d\n", a);
      V(semid, 0);
      break;
    }
  }
  return;
}

void *PrintNum2() {
  while (1) {
    P(semid, 2);
    if (a != 4851) {
      printf("Pinter 2: a = %d\n", a);
      V(semid, 0);
    } else {
      printf("Pinter 2: a = %d\n", a);
      V(semid, 0);
      break;
    }
  }
  return;
}

int main() {
  semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666); // 创建信号灯集
  if (semid == -1) {
    printf("create semaphore fail!\n");
    exit(1);
  }
  union semun sem_arg[3];
  sem_arg[0].val = 1;
  sem_arg[1].val = 0;
  sem_arg[2].val = 0;
  if (semctl(semid, 0, SETVAL, sem_arg[0]) == -1) { // 初始化0号信号灯，赋1
    printf("initialize semaphroe 0 fail!\n");
    exit(1);
  }
  if (semctl(semid, 1, SETVAL, sem_arg[1]) == -1) { // 初始化1号信号灯，赋0
    printf("initialize semaphroe 1 fail!\n");
    exit(1);
  }
  if (semctl(semid, 2, SETVAL, sem_arg[2]) == -1) { // 初始化2号信号灯，赋0
    printf("initialize semaphroe 2 fail!\n");
    exit(1);
  }

  pthread_t pcal, pprint1, pprint2; // 创建3个线程
  if (pthread_create(&pcal, NULL, Calculate, NULL) == -1) {
    printf("create pthread pcal fail!\n");
    exit(1);
  }
  if (pthread_create(&pprint1, NULL, PrintNum1, NULL) == -1) {
    printf("create pthread pprint1 fail!\n");
    exit(1);
  }
  if (pthread_create(&pprint2, NULL, PrintNum2, NULL) == -1) {
    printf("create pthread pprint2 fail!\n");
    exit(1);
  }

  // 等待所有线程结束
  if (pthread_join(pcal, NULL) == -1) {
    printf("pthread ppcal finish fail!\n");
    exit(1);
  }
  if (pthread_join(pprint1, NULL) == -1) {
    printf("pthread pprint1 finish fail!\n");
    exit(1);
  }
  if (pthread_join(pprint2, NULL) == -1) {
    printf("pthread pprint2 finish fail!\n");
    exit(1);
  }
  semctl(semid, 0, IPC_RMID, NULL);
  printf("ALL DONE!!!\n");
  exit(0);
}
