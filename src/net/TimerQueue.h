#ifndef MYMUDUO_NET_TIMERQUEUE_H
#define MYMUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>
#include <mutex>

#include "src/net/Callbacks.h"
#include "src/base/Timestamp.h"
#include "src/base/noncopyable.h"

namespace mymuduo {
  class EventLoop;
  class Timer;
  class TimerId;

  class TimerQueue : noncopyable {

  };


}


#endif // MYMUDUO_NET_TIMERQUEUE_H