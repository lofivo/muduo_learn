#ifndef MYMUDUO_NET_POLLER_H
#define MYMUDUO_NET_POLLER_H

#include "src/base/Timestamp.h"
#include "src/net/EventLoop.h"

#include <unordered_map>
#include <vector>

namespace mymuduo {
class Channel;
class Poller : noncopyable {
public:
  using ChannelList = std::vector<Channel *>;

public:
  Poller(EventLoop *loop);
  virtual ~Poller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  // 调用poll(2)获得当前活动的IO事件，然后填充调用方传入的activeChannels
  // 并返回poll(2) return 的时刻
  virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  // 负责维护和更新pollfds_数组
  virtual void updateChannel(Channel *channel) = 0;

  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  virtual void removeChannel(Channel *channel) = 0;

  virtual bool hasChannel(Channel *channel) const;

  static Poller *newDefaultPoller(EventLoop *loop);
  void assertInLoopThread() const;

protected:
  using ChannelMap = std::unordered_map<int, Channel *>;
  ChannelMap channels_;
private:
  EventLoop *ownerLoop_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_POLLER_H