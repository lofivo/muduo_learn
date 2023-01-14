#ifndef MYMUDUO_NET_TIMERQUEUE_H
#define MYMUDUO_NET_TIMERQUEUE_H

#include <mutex>
#include <set>
#include <vector>

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
  TimerQueue(EventLoop *loop);
  ~TimerQueue();

  TimerId addTimer(TimerCallback cb, Timestamp when, double interval);

private:
  using Entry = std::pair<Timestamp, Timer*>;
  using TimerList = std::set<Entry>;

  // called when timerfd alarms
  void handleRead();

  
  void addTimerInLoop(Timer *timer);

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
};

} // namespace mymuduo

#endif // MYMUDUO_NET_TIMERQUEUE_H