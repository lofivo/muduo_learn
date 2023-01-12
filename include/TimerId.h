#ifndef MYMUDUO_TIMERID_H_
#define MYMUDUO_TIMERID_H_

#include "Timestamp.h"
#include <atomic>

namespace muduo {
class TimerId {
public:
  TimerId(Timestamp when) : expiration_(when), sequence_(++s_numCreated_) {}
  TimerId(Timestamp when, uint64_t sequence)
      : expiration_(when), sequence_(sequence) {}

  Timestamp getExpirationTime() const { return expiration_; }

  uint64_t getSequence() const { return sequence_; }

protected:
  Timestamp expiration_;
  uint64_t sequence_{0};

  inline static std::atomic_uint64_t s_numCreated_{0};

  friend class CompableTimer;
};

} // namespace muduo
#endif // MYMUDUO_TIMERID_H_