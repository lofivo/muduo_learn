#ifndef MYMUDUO_NET_POLLER_H
#define MYMUDUO_NET_POLLER_H

#include "src/base/Timestamp.h"
#include "src/base/noncopyable.h"
#include "src/net/EventLoop.h"

#include <unordered_map>
#include <vector>

struct pollfd;

namespace mymuduo {
class Channel;
class Poller : noncopyable {
public:
  using ChannelList = std::vector<Channel*>;

  Poller(EventLoop *loop);
  ~Poller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  // 调用poll(2)获得当前活动的IO事件，然后填充调用方传入的activeChannels
  // 并返回poll(2) return 的时刻
  Timestamp poll(int timeoutMs, ChannelList *activeChannels);

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  // 负责维护和更新pollfds_数组
  void updateChannel(Channel *channel);

  void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }

private:
  // 遍历pollfds_，找出所有有活动事件的fd，把它对应的Channel填入activeChannels
  void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

  using PollFdList = std::vector<struct pollfd>;
  using ChannelMap = std::unordered_map<int, Channel *>;
  
  EventLoop *ownerLoop_;
  // Poller::poll()不会在每次调用poll(2)之前临时构造pollfd数组
  // 而是把它缓存起来（pollfds_）
  PollFdList pollfds_;
  // fd 到 Channel的映射
  ChannelMap channels_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_POLLER_H