#include "CurrentThread.h"
namespace mymuduo {
namespace CurrentThread {
__thread int t_cachedTid = 0;

void cacheTid() {
  if (t_cachedTid == 0) {
    t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
  }
}
} // namespace CurrentThread

} // namespace mymuduo