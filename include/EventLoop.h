#ifndef MYMUDUO_EVENTLOOP_H_
#define MYMUDUO_EVENTLOOP_H_
#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Timestamp.h"
#include "TimerId.h"
#include "Callback.h"
#include "noncopyable.h"

namespace muduo {
class Channel;
class Poller;
class TimerQueue;

class EventLoop : noncopyable {
public:
  using Functor = std::function<void()>;

public:
  EventLoop();
  ~EventLoop();

  // Must be called in the same thread as creation of the object.
  void loop();
  void quit();

  Timestamp pollReturnTime() const { return pollReturnTime_; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  void runInLoop(const Functor &cb);
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  void queueInLoop(const Functor &cb);

  TimerId runAt(const Timestamp &time, const TimerCallback &cb);

  TimerId runAfter(double delay, const TimerCallback &cb);

  TimerId runEvery(double interval, const TimerCallback &cb);

  void cancel(TimerId timerId);

  void cancelAll(); // cancel all timer at this loop

  void wakeup(); // wake up the thread for this loop

  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);

  bool isInLoopThread() const {
    return threadId_ == std::this_thread::get_id();
  }

  void assertInLoopThread() {
    if (!isInLoopThread())
      abortNotInLoopThread();
  }

private:
  void abortNotInLoopThread();
  void handleRead(); // weak up
  void doPendingFunctors();

private:
  using ChannelList = std::vector<Channel *>;

  std::atomic_bool looping_;
  std::atomic_bool quit_;
  const std::thread::id threadId_;
  Timestamp pollReturnTime_; // the time that poller return channels
  std::unique_ptr<Poller> poller_;

  int wakeupFd_; // mainLoop distribute a subLoop to handle channel when
                 // mainLoop gets a new user channel
  std::unique_ptr<Channel> wakeupChannel_;

  ChannelList activeChannels_;
  std::unique_ptr<TimerQueue> timerQueue_;

  std::atomic_bool callingPendingFunctors_; // mark whether loop needs callback
  std::vector<Functor> pendingFunctors_;
  std::mutex mutex_; // protect pendingFunctors
};

} // namespace muduo

#endif // MYMUDUO_EVENTLOOP_H_