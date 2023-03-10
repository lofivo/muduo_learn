#ifndef MYMUDUO_NET_TIMERID_H
#define MYMUDUO_NET_TIMERID_H

#include "src/base/Timestamp.h"
#include <atomic>

namespace mymuduo {
class Timer;

class TimerId {
public:
  friend class TimerQueue;
  TimerId() : timer_(nullptr), sequence_(0) {}

  TimerId(Timer *timer, int64_t seq) : timer_(timer), sequence_(seq) {}

private:
  Timer *timer_;
  int64_t sequence_;
};
} // namespace mymuduo

#endif // MYMUDUO_NET_TIMERID_H