#ifndef MYMUDUO_NET_EPOLLPOLLER_H
#define MYMUDUO_NET_EPOLLPOLLER_H
#include "src/net/Poller.h"
#include <sys/epoll.h>
#include <vector>

namespace mymuduo {
class EPollPoller : public Poller {
public:
  EPollPoller(EventLoop *loop);
  ~EPollPoller();
  
  Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
  void updateChannel(Channel* channel) override;
  void removeChannel(Channel* channel) override;

private:
  // 填写活跃的连接
  void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
  // 更新channel通道，本质是调用了epoll_ctl
  void update(int operation, Channel *channel);

private:
  using EventList = std::vector<epoll_event>;
  // 默认监听事件数量
  static const int kInitEventListSize = 16;
  int epollfd_; // epoll_create在内核创建空间返回的fd
  EventList events_; // 用于存放epoll_wait返回的所有发生的事件的文件描述符
};
} // namespace mymuduo

#endif // MYMUDUO_NET_EPOLLPOLLER_H