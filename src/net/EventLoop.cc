#include "src/net/EventLoop.h"
#include "TimerQueue.h"
#include "src/logger/Logging.h"
#include "src/net/Channel.h"
#include "src/net/Poller.h"
#include "src/base/CurrentThread.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <algorithm>

using namespace mymuduo;

thread_local EventLoop *t_loopInThisThread = nullptr;

// default poller timeout value
const int kPollTimeMs = 10000;

// create wakeup fd to notify subReactor's channel
static int createEventFd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

uint64_t EventLoop::getThreadId() {
  uint64_t id;
  memcpy(&id, &threadId_, 8);
  return id;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), eventHandling_(false),
      callingPendingFunctors_(false), threadId_(std::this_thread::get_id()),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)), wakeupFd_(createEventFd()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {
  LOG_DEBUG << "EventLoop created " << this << " in thread " << getThreadId();
  if (t_loopInThisThread) {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << getThreadId();
  } else {
    t_loopInThisThread = this;
  }
  // 设置wakeupfd的事件类型以及发生事件的回调函数
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  // 每一个EventLoop都将监听wakeupChannel的EPOLLIN事件
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
  LOG_DEBUG << "EventLoop " << this << " of thread "
            << getThreadId() << " destructs in thread "
            << CurrentThread::get_id();
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  t_loopInThisThread = nullptr;
}

EventLoop *EventLoop::getEventLoopOfCurrentThread() {
  return t_loopInThisThread;
}

void EventLoop::loop() {
  assert(!looping_);
  assertInLoopThread();
  looping_.store(true);
  quit_.store(false);

  LOG_TRACE << "EventLoop " << this << " start looping";

  while (!quit_.load()) {
    // 清空activeChannels_
    activeChannels_.clear();
    // 获取
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    if (Logger::LogLevel() <= Logger::TRACE) {
      printActiveChannels();
    }
    eventHandling_ = true;
    for (auto &channel : activeChannels_) {
      channel->handleEvent(pollReturnTime_);
    }
    eventHandling_ = false;
    // 执行当前EventLoop事件循环需要处理的回调操作
    doPendingFunctors();
  }
  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_.store(false);
}

void EventLoop::quit() {
  quit_.store(true);
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }

  if (!isInLoopThread() || callingPendingFunctors_.load()) {
    wakeup();
  }
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = ::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
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

void EventLoop::cancel(TimerId timerId) { return timerQueue_->cancel(timerId); }

void EventLoop::updateChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (eventHandling_) {
    assert(std::find(activeChannels_.begin(), activeChannels_.end(), channel) ==
           activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread() {
  LOG_FATAL << "EventLoop " << this << " was created in tid " 
            << getThreadId() << ", current thread id = " << CurrentThread::get_id();
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = ::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_.store(true);

  {
    std::lock_guard<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const Functor &functor : functors) {
    functor();
  }

  callingPendingFunctors_.store(false);
}
void EventLoop::printActiveChannels() const {
  for (const Channel *channel : activeChannels_) {
    LOG_TRACE << "{" << channel->reventsToString() << "} ";
  }
}