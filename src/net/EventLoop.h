#ifndef MYMUDUO_NET_EVENTLOOP_H
#define MYMUDUO_NET_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <mutex>
#include <vector>
#include <thread>
#include <src/base/Thread.h>
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
  using Functor = std::function<void()>;

public:
  EventLoop();
  ~EventLoop(); // force out-line dtor, for std::unique_ptr members.

  // 必须在创建EventLoop对象的线程上运行
  void loop();

  void quit();

  void runInLoop(Functor cb); // run cb in current thread

  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  void queueInLoop(Functor cb);

  // 用来唤醒该loop所在的线程
  void wakeup();

  // Time when poll returns, usually means data arrivial.
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  // Runs callback at 'time'.
  TimerId runAt(Timestamp time, TimerCallback cb);
  // Runs callback after @c delay seconds.
  TimerId runAfter(double delay, TimerCallback cb);
  // Runs callback every @c interval seconds.
  TimerId runEvery(double interval, TimerCallback cb);

  void cancel(TimerId timerId);

  // internal use only
  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel);

  void assertInLoopThread() {
    if (!isInLoopThread()) {
      abortNotInLoopThread();
    }
  }

  bool isInLoopThread() { return threadId_ == std::this_thread::get_id(); }

  uint64_t getThreadId();

  static EventLoop* getEventLoopOfCurrentThread();

private:
  void abortNotInLoopThread();
  void handleRead(); // waked up
  void doPendingFunctors(); // callback
  void printActiveChannels() const;

private:
  using ChannelList = std::vector<Channel *>;

  std::atomic_bool looping_;                // atomic
  std::atomic_bool quit_;                   // 退出事件循环flag
  std::atomic_bool eventHandling_;
  std::atomic_bool callingPendingFunctors_; // 当前loop是否有需要执行的回调操作
  const std::thread::id threadId_; // 记录当前loop所在线程的id
  Timestamp pollReturnTime_;
  std::unique_ptr<Poller> poller_;
  std::unique_ptr<TimerQueue> timerQueue_;
  int wakeupFd_;
  // 用于处理wakeupFd_上的可读事件，将事件分发给handleRead
  std::unique_ptr<Channel> wakeupChannel_;
  ChannelList activeChannels_; // 活跃的channel
  std::mutex mutex_;           // 用于保护pendingFunctors_线程安全操作
  std::vector<Functor> pendingFunctors_; // 存储loop跨线程需要执行的所有回调操作
};

} // namespace mymuduo

#endif // MYMUDUO_NET_EVENTLOOP_H
