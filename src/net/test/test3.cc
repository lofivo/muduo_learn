#include "src/net/Channel.h"
#include "src/net/EventLoop.h"

#include <stdio.h>
#include <sys/timerfd.h>
#include <unistd.h>

mymuduo::EventLoop* g_loop;

void timeout(mymuduo::Timestamp receiveTime)
{
  printf("%s Timeout!\n", receiveTime.toFormattedString().c_str());
  g_loop->quit();
}

int main()
{
  printf("%s started\n", mymuduo::Timestamp::now().toFormattedString().c_str());
  mymuduo::EventLoop loop;
  g_loop = &loop;

  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  mymuduo::Channel channel(&loop, timerfd);
  channel.setReadCallback(timeout);
  channel.enableReading();

  struct itimerspec howlong;
  bzero(&howlong, sizeof howlong);
  howlong.it_value.tv_sec = 5;
  ::timerfd_settime(timerfd, 0, &howlong, NULL);

  loop.loop();

  ::close(timerfd);
}
