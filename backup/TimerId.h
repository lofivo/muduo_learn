#ifndef MYMUDUO_NET_TIMERID_H
#define MYMUDUO_NET_TIMERID_H

#include <stdint.h>
namespace mymuduo {
class Timer;

class TimerId {
  friend class TimerQueue;

public:
  TimerId(Timer *timer = nullptr, int64_t seq = 0) : timer_(timer), sequence_(seq) {}

private:
  Timer *timer_;
  int64_t sequence_;
};
} // namespace mymuduo

#endif // MYMUDUO_NET_TIMERID_H