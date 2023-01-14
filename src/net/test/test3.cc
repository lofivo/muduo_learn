#include "src/net/Channel.h"
#include "src/net/EventLoop.h"

#include <stdio.h>
#include <sys/timerfd.h>

mymuduo::EventLoop* g_loop;
int timerfd;
void timeout(mymuduo::Timestamp receiveTime)
{
  printf("%s Timeout!\n", receiveTime.toFormattedString().c_str());
  // 注意这里必须设置quit
  // 不然timerfd会一直处于可读状态,因为timerfd是CLOCK_MONOTONIC模式，不会调整和复位
  // 就会busy loop,无限输出
  // 除非再次设置超时时间,会把timerfd复位重新定时
  // struct itimerspec howlong;
  // bzero(&howlong, sizeof howlong);
  // howlong.it_value.tv_sec = 5;
  // ::timerfd_settime(timerfd, 0, &howlong, NULL);
  g_loop->quit();
}

int main()
{
  // 创建一个EventLoop对象loop
  mymuduo::EventLoop loop;
  g_loop = &loop;
  // 先去创建一个定时器timerfd
  // CLOCK_MONOTONIC:系统启动后不可改变的单调递增时钟。以固定的速率运行，从不进行调整和复位 ,它不受任何系统time-of-day时钟修改的影响
  // TFD_NONBLOCK:非阻塞
  // TFD_CLOEXEC:设置文件描述符close-on-exec,即使用exec执行程序时,关闭该文件描述符
  timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  // 创建一个Channel对象，绑定了loop和timerfd
  mymuduo::Channel channel(&loop, timerfd);
  // channel设置可读事件回调函数为timeout
  channel.setReadCallback(timeout);
  // enableReading使channel关注可读事件
  // 即设置channel.events_的bit pattern
  // 之后会调用Channel::update()
  // Channel::update()会转而调用loop_->updateChannel(this)
  // 再转而poller_->updateChannel(channel);
  // 这个channel是新建的channel
  // 会创建pollfd pfd,将channel的fd_,events_参数填充进去
  // 然后将pfd加入到Poller::pollfds_数组
  // 然后新建pfd.fd(==channel.fd_)到channel的映射
  channel.enableReading();

  struct itimerspec howlong;
  bzero(&howlong, sizeof howlong);
  howlong.it_value.tv_sec = 5;
  // 设置定时器超时时间为5秒
  ::timerfd_settime(timerfd, 0, &howlong, NULL);
  // 启动事件循环
  loop.loop();

  // 每次事件循环
  // activeChannels_.clear();
  // pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
  // 过了5秒后
  // timerfd超时，设置为可读
  // 事件循环中
  // 执行poller_->poll(kPollTimeMs, &activeChannels_)
  // poller_->poll去调用poll(2),发现了timerfd上的可读事件, poll返回满足对应监听事件的文件描述符总个数
  // 这里会返回1,因为只有一个timerfd可读事件
  // 因为poll返回值>0,说明存在active的channel
  // 转而调用fillActiveChannels(numEvents, activeChannels)
  // 该函数会遍历pollfds_数组
  // 对于pollfds_的每一个pfd元素，如果pfd->revents > 0
  // revents如果满足对应事件会返回非0
  // 说明这个pfd对应的channel是一个active的channel
  // 于是调用channels_.find(pfd->fd)来找到pfd->fd对应的channel
  // 然后调用channel->set_revents(pfd->revents);
  // 将当前活动事件revents保存在Channel中,供Channel::handleEvent()使用
  // 然后将这个channel加入到activeChannels容器中
  // 到这一步，poller_->poll执行完毕，得到poll返回时的时间，以及更新了activeChannels_
  // 然后事件循环中，遍历activeChannels_
  // 执行每个活跃的channel的handleEvent(pollReturnTime_)
  // 这里有一个活跃的channel,它的handleEvent会判断该活跃channel存储的revents_
  // 发现有可读事件, 于是调用可读事件的回调函数readCallback_(receiveTime)
  // 这个可读回调函数就是之前channel.setReadCallback(timeout)设置的
  // 于是会去执行timeout函数

  // 事件结束后，关闭timerfd的文件描述符
  ::close(timerfd);
}
