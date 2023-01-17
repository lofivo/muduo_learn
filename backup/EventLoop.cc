#include "src/net/EventLoop.h"
#include "src/logger/Logging.h"
#include "src/net/Channel.h"
#include "src/net/Poller.h"
#include "src/net/TimerQueue.h"

#include <sys/eventfd.h>

using namespace mymuduo;

// 防止一个线程创建多个EventLoop
// __thread修饰的变量，在不同线程中是相互独立的
__thread EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

static int createEventfd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()), poller_(new Poller(this)),
      timerQueue_(new TimerQueue(this)), wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {
  LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
  if (t_loopInThisThread) {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;
  } else {
    t_loopInThisThread = this;
  }
  // 设置wakeupfd的事件类型以及发生事件的回调函数
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  // 每一个EventLoop都将监听wakeupChannel的EPOLLIN事件
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
  assert(!looping_);
  ::close(wakeupFd_);
  t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false;

  while (!quit_) {
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    for (ChannelList::iterator it = activeChannels_.begin();
         it != activeChannels_.end(); ++it) {
      (*it)->handleEvent(pollReturnTime_);
    }
    // 执行当前EventLoop事件循环需要处理的回调操作
    /**
     * IO thread：mainLoop accept fd 打包成 chennel 分发给 subLoop
     * mainLoop实现注册一个回调，交给subLoop来执行，wakeup subLoop
     * 之后，让其执行注册的回调操作 这些回调函数在 std::vector<Functor>
     * pendingFunctors_; 之中
     */
    doPendingFunctors();
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  /**
   * TODO:生产者消费者队列派发方式和muduo的派发方式
   * 有可能是别的线程调用quit(调用线程不是生成EventLoop对象的那个线程)
   * 比如在工作线程(subLoop)中调用了IO线程(mainLoop)
   * 这种情况会唤醒主线程
   */
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb) {
  bool inThread = isInLoopThread();
  LOG_TRACE << "EventLoop::runInLoop in loopThread:"
            << (inThread ? "true" : "false");
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(cb); // 使用了std::move
  }

  // 唤醒相应的，需要执行上面回调操作的loop线程
  if (!isInLoopThread() || callingPendingFunctors_) {
    // 唤醒loop所在的线程
    wakeup();
  }
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb) {
  return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb) {
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb) {
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::updateChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::abortNotInLoopThread() {
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = write(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    LOG_ERROR << "EventLoop::wakeup writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    std::unique_lock<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const Functor &functor : functors) {
    functor();
  }

  callingPendingFunctors_ = false;
}