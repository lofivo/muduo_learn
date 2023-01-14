#include "src/net/Poller.h"
#include "src/logger/Logging.h"
#include "src/net/Channel.h"

#include <assert.h>
#include <poll.h>

using namespace mymuduo;

Poller::Poller(EventLoop *loop) : ownerLoop_(loop) {}

Poller::~Poller() {}

Timestamp Poller::poll(int timeoutMs, ChannelList *activeChannels) {
  // XXX pollfds_ shouldn't change
  int numEvents = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs);
  Timestamp now(Timestamp::now());
  if (numEvents > 0) {
    LOG_TRACE << numEvents << " events happended";
    fillActiveChannels(numEvents, activeChannels);
  } else if (numEvents == 0) {
    LOG_TRACE << " nothing happended";
  } else {
    LOG_SYSERR << "Poller::poll()";
  }
  return now;
}

void Poller::fillActiveChannels(int numEvents,
                                ChannelList *activeChannels) const {
  for (PollFdList::const_iterator pfd = pollfds_.begin();
       pfd != pollfds_.end() && numEvents > 0; ++pfd) {
    if (pfd->revents > 0) {
      // 为了提前结束循环，每找到一个活动fd就递减numEvents
      // 当numEvents减为0表示活动fd都找完了，因此可以直接退出循环
      --numEvents;
      ChannelMap::const_iterator ch = channels_.find(pfd->fd);
      assert(ch != channels_.end());
      Channel *channel = ch->second;
      assert(channel->fd() == pfd->fd);
      // 当前活动事件revents会保存在Channel中
      // 供Channel::handleEvent()使用
      channel->set_revents(pfd->revents);
      // pfd->revents = 0;
      activeChannels->push_back(channel);
    }
  }
}


void Poller::updateChannel(Channel *channel) {
  assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
  // index_初始值为-1,说明这个channel是一个新的channel对象
  if (channel->index() < 0) {
    // a new one, add to pollfds_
    assert(channels_.find(channel->fd()) == channels_.end());
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size()) - 1;
    channel->set_index(idx);
    channels_[pfd.fd] = channel;
  } else { // 更新已有的Channel对象
    // update existing one
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    struct pollfd &pfd = pollfds_[idx];
    assert(pfd.fd == channel->fd() || pfd.fd == -1);
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    if (channel->isNoneEvent()) {
      // ignore this pollfd
      pfd.fd = -1;
    }
  }
}