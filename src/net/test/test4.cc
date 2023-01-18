// copied from mymuduo/net/tests/TimerQueue_unittest.cc

#include "src/net/EventLoop.h"
#include <stdio.h>
#include <unistd.h>

int cnt = 0;
mymuduo::EventLoop* g_loop;

void printTid()
{
  printf("pid = %d, tid = %d\n", getpid(), gettid());
  printf("now %s\n", mymuduo::Timestamp::now().toString().c_str());
}

void print(const char* msg)
{
  printf("msg %s %s\n", mymuduo::Timestamp::now().toString().c_str(), msg);
  if (++cnt == 20)
  {
    g_loop->quit();
  }
}

mymuduo::TimerId toCancel;
void cancelSelf()
{
  print("cancelSelf()");
  g_loop->cancel(toCancel);
}

int main()
{
  printTid();
  mymuduo::EventLoop loop;
  g_loop = &loop;

  print("main");
  loop.runAfter(1, std::bind(print, "once1"));
  loop.runAfter(1.5, std::bind(print, "once1.5"));
  loop.runAfter(2.5, std::bind(print, "once2.5"));
  loop.runAfter(3.5, std::bind(print, "once3.5"));
  mymuduo::TimerId t = loop.runEvery(2, std::bind(print, "every2"));
  loop.runEvery(3, std::bind(print, "every3"));
  loop.runAfter(10, std::bind(&mymuduo::EventLoop::cancel, &loop, t));
  toCancel = loop.runEvery(5, cancelSelf);

  loop.loop();
  print("main loop exits");
  sleep(1);
}
