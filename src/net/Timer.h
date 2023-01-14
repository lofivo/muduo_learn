#ifndef MYMUDUO_NET_TIMER_H
#define MYMUDUO_NET_TIMER_H

#include "src/base/Timestamp.h"
#include "src/base/noncopyable.h"
#include "src/net/Callbacks.h"

#include <atomic>
#include <functional>

namespace mymuduo {

class Timer : noncopyable {
public:
  Timer(TimerCallback cb, Timestamp when, double interval)
      : callback_(std::move(cb)), expiration_(when), interval_(interval),
        repeat_(interval > 0.0), sequence_(++s_numCreated_) {}
  void run() const { callback_(); }

  Timestamp expiration() const { return expiration_; }
  bool repeat() const { return repeat_; }
  int64_t sequence() const { return sequence_; }

  void restart(Timestamp now);

  static int64_t numCreated() { return s_numCreated_.load(); }

private:
  const TimerCallback callback_; // 定时器回调函数
  Timestamp expiration_;         // 下一次的超时时刻
  const double interval_; // 超时时间间隔，如果是一次性定时器，该值为0
  const bool repeat_;      // 是否重复(false 表示是一次性定时器)
  const int64_t sequence_; // 定时器序号

  static std::atomic<int64_t> s_numCreated_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_TIMER_H