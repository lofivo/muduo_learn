#include "src/net/Channel.h"
#include "src/logger/Logging.h"
#include "src/net/EventLoop.h"

#include <sstream>

#include <poll.h>

using namespace mymuduo;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fdArg)
    : loop_(loop), fd_(fdArg), events_(0), revents_(0), index_(-1) {}

void Channel::update() { loop_->updateChannel(this); }

void Channel::handleEvent(Timestamp receiveTime) {
  if (revents_ & POLLNVAL) {
    LOG_WARN << "Channel::handle_event() POLLNVAL";
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
}