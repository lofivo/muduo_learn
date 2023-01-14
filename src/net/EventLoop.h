#ifndef MYMUDUO_NET_EVENTLOOP_H
#define MYMUDUO_NET_EVENTLOOP_H
#include <any>
#include <atomic>
#include <functional>
#include <vector>

#include "src/base/CurrentThread.h"
#include "src/base/Timestamp.h"
#include "src/base/noncopyable.h"
#include "src/net/Callbacks.h"
#include "src/net/TimerId.h"

namespace mymuduo {

class Channel;
class Poller;
class TimerQueue;

class EventLoop : noncopyable {
public:
  EventLoop();
  ~EventLoop(); // force out-line dtor, for std::unique_ptr members.

  // 必须在创建EventLoop对象的线程上运行
  void loop();

  void quit();
  
  // internal use only
  void updateChannel(Channel* channel);

  void assertInLoopThread() {
    if (!isInLoopThread()) {
      abortNotInLoopThread();
    }
  }

  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
  void abortNotInLoopThread();

private:
  using ChannelList = std::vector<Channel*>;

  std::atomic<bool> looping_; // atomic
  std::atomic<bool> quit_; // atomic
  const pid_t threadId_; // 记录当前loop所在线程的id
  std::unique_ptr<Poller> poller_;
  Timestamp pollReturnTime_;
  ChannelList activeChannels_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_EVENTLOOP_H
