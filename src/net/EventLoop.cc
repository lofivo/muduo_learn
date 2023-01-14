#include "src/net/EventLoop.h"
#include "src/logger/Logging.h"
#include "src/net/Channel.h"
#include "src/net/Poller.h"
#include "src/net/TimerQueue.h"
using namespace mymuduo;

// 防止一个线程创建多个EventLoop
// __thread修饰的变量，在不同线程中是相互独立的
__thread EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

EventLoop::EventLoop()
    : looping_(false), quit_(false), threadId_(CurrentThread::tid()),
      poller_(new Poller(this)), timerQueue_(new TimerQueue(this)) {
  LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
  if (t_loopInThisThread) {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;
  } else {
    t_loopInThisThread = this;
  }
}

EventLoop::~EventLoop() {
  assert(!looping_);
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
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  // wakeup();
}

void EventLoop::runInLoop(Functor cb) {
  bool inThread = isInLoopThread();
  LOG_TRACE << "EventLoop::runInLoop in loopThread:"
            << (inThread ? "true" : "false");
  if (isInLoopThread()) {
    cb();
  } else {
    // TODO:queueInLoop(cb);
  }
}

TimerId EventLoop::runAt(const Timestamp &time, const TimerCallback &cb) {
  return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb) {
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback &cb) {
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(cb, time, interval);
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