#ifndef MYMUDUO_BASE_NONCOPYABLE_H
#define MYMUDUO_BASE_NONCOPYABLE_H

namespace mymuduo {

class noncopyable {
public:
  noncopyable(const noncopyable &) = delete;
  noncopyable &operator=(const noncopyable &) = delete;
  noncopyable(noncopyable &&) = default;
  noncopyable &operator=(noncopyable &&) = default;

protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

} // namespace mymuduo

#endif // MYMUDUO_BASE_NONCOPYABLE_H