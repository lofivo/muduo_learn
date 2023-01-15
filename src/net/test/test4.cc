#include "src/net/EventLoop.h"

#include <stdio.h>

int cnt = 0;
mymuduo::EventLoop* g_loop;

void printTid()
{
  printf("pid = %d, tid = %d\n", getpid(), mymuduo::CurrentThread::tid());
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

int main()
{
  printTid();
  mymuduo::EventLoop loop;
  g_loop = &loop;

  print("main");
  loop.runAfter(1, std::bind(print, "once1"));
  // loop.runAfter -> EventLoop::runAt -> timerQueue_->addTimer
  // 会新建一个Timer, insert到TimerQueue内
  // addTimer将timerfd加入到TimerQueue的channel里
  // 超时后可写事件，调用TimerQueue::handleRead()
  // handleRead清除该timerfd_上的事件，避免一直触发
  // 得到所有超时定时器
  // 执行每个超时定时器绑定的回调
  // 重启所有非一次性的定时器
  

  loop.runAfter(1.5, std::bind(print, "once1.5"));
  loop.runAfter(2.5, std::bind(print, "once2.5"));
  loop.runAfter(3.5, std::bind(print, "once3.5"));
  loop.runEvery(2, std::bind(print, "every2"));
  loop.runEvery(3, std::bind(print, "every3"));

  loop.loop();
  print("main loop exits");
  sleep(1);
}
