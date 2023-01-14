#ifndef MYMUDUO_BASE_CURRENTTHREAD_H
#define MYMUDUO_BASE_CURRENTTHREAD_H
#include <sys/syscall.h>
#include <unistd.h>
namespace mymuduo {

namespace CurrentThread {
extern __thread int t_cachedTid; // 保存tid缓冲，避免多次系统调用

void cacheTid();

// 内联函数
inline int tid() {
  if (__builtin_expect(t_cachedTid == 0, 0)) {
    cacheTid();
  }
  return t_cachedTid;
}
} // namespace CurrentThread

} // namespace mymuduo

#endif // MYMUDUO_BASE_CURRENTTHREAD_H