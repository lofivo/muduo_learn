#ifndef MYMUDUO_NONCOPYABLE_H_
#define MYMUDUO_NONCOPYABLE_H_
class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;
    noncopyable(noncopyable &&) = default;
    noncopyable &operator=(noncopyable &&) = default;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};
#endif // MYMUDUO_NONCOPYABLE_H_
