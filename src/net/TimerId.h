#ifndef MYMUDUO_NET_TIMERID_H
#define MYMUDUO_NET_TIMERID_H

#include <stdint.h>

namespace mymuduo {
class Timer;

class TimerId {
  friend class TimerQueue;

public:
  TimerId() : timer_(nullptr), sequence_(0) {}
  TimerId(Timer *timer, int64_t seq) : timer_(timer), sequence_(seq) {}

private:
  Timer *timer_;
  int64_t sequence_;
};
} // namespace mymuduo

#endif // MYMUDUO_NET_TIMERID_H