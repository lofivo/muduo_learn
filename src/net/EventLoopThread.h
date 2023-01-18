#ifndef MYMUDUO_NET_EVENTLOOPTHREAD_H
#define MYMUDUO_NET_EVENTLOOPTHREAD_H

#include "src/base/Thread.h"
#include "src/base/noncopyable.h"

#include <condition_variable>
#include <functional>
#include <mutex>

namespace mymuduo {
class EventLoop;
class EventLoopThread : noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

public:
  EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                  const std::string &nameArg = std::string());

  ~EventLoopThread();

  EventLoop *startLoop();

private:
  void threadFunc();

private:
  // EventLoopThread::threadFunc 会创建 EventLoop
  EventLoop *loop_;
  bool exiting_;
  Thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_EVENTLOOPTHREAD_H