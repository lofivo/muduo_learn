#ifndef MYMUDUO_NET_TIMERQUEUE_H
#define MYMUDUO_NET_TIMERQUEUE_H

#include <mutex>
#include <set>
#include <vector>
#include <atomic>

#include "src/base/Timestamp.h"
#include "src/base/noncopyable.h"
#include "src/net/Callbacks.h"
#include "src/net/Channel.h"

namespace mymuduo {
class EventLoop;
class Timer;
class TimerId;

class TimerQueue : noncopyable {
public:
  explicit TimerQueue(EventLoop *loop);
  ~TimerQueue();

  TimerId addTimer(TimerCallback cb, Timestamp when, double interval);
  void cancel(TimerId timerId);

private:
  using Entry = std::pair<Timestamp, Timer *>;
  using TimerList = std::set<Entry>;
  using ActiveTimer = std::pair<Timer *, int64_t>;
  using ActiveTimerSet = std::set<ActiveTimer>;

  // called when timerfd alarms
  void handleRead();

  void addTimerInLoop(Timer *timer);
  void cancelInLoop(TimerId timerId);

  // move out all expired timers
  std::vector<Entry> getExpired(Timestamp now);

  void reset(const std::vector<Entry> &expired, Timestamp now);

  bool insert(Timer *timer);

private:
  EventLoop *loop_;
  const int timerfd_;
  Channel timerfdChannel_;
  // Timer list sorted by expiration
  TimerList timers_;

  // for cancel()
  ActiveTimerSet activeTimers_;
  std::atomic_bool callingExpiredTimers_;
  ActiveTimerSet cancelingTimers_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_TIMERQUEUE_H