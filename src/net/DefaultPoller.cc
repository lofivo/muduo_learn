#include "src/net/Poller.h"
#include "src/net/EPollPoller.h"
#include <cstdlib>

using namespace mymuduo;

Poller* Poller::newDefaultPoller(EventLoop *loop) {
  if (::getenv("MUDUO_USE_POLL")) {
    return nullptr;
  } else {
    return new EPollPoller(loop);
  }
}
