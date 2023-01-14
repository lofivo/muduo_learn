#include "src/net/EventLoop.h"
#include "src/base/Thread.h"
#include <stdio.h>

void threadFunc()
{
  printf("threadFunc(): pid = %d, tid = %d\n",
         getpid(), mymuduo::CurrentThread::tid());

  mymuduo::EventLoop loop;
  loop.loop();
}

int main()
{
  printf("main(): pid = %d, tid = %d\n",
         getpid(), mymuduo::CurrentThread::tid());

  mymuduo::EventLoop loop;

  mymuduo::Thread thread(threadFunc);
  thread.start();

  loop.loop();
  pthread_exit(NULL);
}
