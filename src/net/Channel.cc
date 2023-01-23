#include "src/net/Channel.h"
#include "src/logger/Logging.h"
#include "src/net/EventLoop.h"
#include <poll.h>
#include <sstream>
using namespace mymuduo;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1),
      addedToLoop_(false), eventHandling_(false), tied_(false) {}

Channel::~Channel() {
  assert(!addedToLoop_);
  assert(!eventHandling_);
  if (loop_->isInLoopThread()) { // TODO: ??
    assert(!loop_->hasChannel(this));
  }
}

// 在TcpConnection建立时调用
void Channel::tie(const std::shared_ptr<void> &obj) {
  tie_ = obj;
  tied_ = true;
}

// 在channel所属的EventLoop中，把该channel删除掉
void Channel::remove() {
  assert(isNoneEvent());
  loop_->removeChannel(this);
  addedToLoop_ = false;
}

// 当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
void Channel::update() {
  addedToLoop_ = true;
  // 通过该channel所属的EventLoop，调用poller对应的方法，注册fd的events事件
  loop_->updateChannel(this);
}

std::string Channel::reventsToString() const {
  return eventsToString(fd_, revents_);
}

std::string Channel::eventsToString() const { return eventsToString(fd_, events_); }

std::string Channel::eventsToString(int fd, int ev) {
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN)
    oss << "IN ";
  if (ev & POLLPRI)
    oss << "PRI ";
  if (ev & POLLOUT)
    oss << "OUT ";
  if (ev & POLLHUP)
    oss << "HUP ";
  if (ev & POLLRDHUP)
    oss << "RDHUP ";
  if (ev & POLLERR)
    oss << "ERR ";
  if (ev & POLLNVAL)
    oss << "NVAL ";

  return oss.str();
}

// fd得到poller通知以后，去处理事件
void Channel::handleEvent(Timestamp receiveTime) {
  // 调用了Channel::tie得会设置tid_=true
  // 而TcpConnection::connectEstablished会调用channel_->tie(shared_from_this());
  // 所以对于TcpConnection::channel_
  // 需要多一份强引用的保证以免用户误删TcpConnection对象
  if (tied_) {
    // 变成shared_ptr增加引用计数，防止误删
    std::shared_ptr<void> guard = tie_.lock();
    if (guard) {
      handleEventWithGuard(receiveTime);
    }
  } else {
    handleEventWithGuard(receiveTime);
  }
}

// 根据相应事件执行回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime) {
  eventHandling_ = true;
  LOG_TRACE << reventsToString();
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    if (closeCallback_)
      closeCallback_();
  }

  if (revents_ & POLLNVAL) {
    LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
  }

  if (revents_ & (POLLERR | POLLNVAL)) {
    if (errorCallback_)
      errorCallback_();
  }
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (readCallback_)
      readCallback_(receiveTime);
  }
  if (revents_ & POLLOUT) {
    if (writeCallback_)
      writeCallback_();
  }
  eventHandling_ = false;
}
