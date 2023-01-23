#include "src/net/Poller.h"
#include "src/net/Channel.h"

using namespace mymuduo;

Poller::Poller(EventLoop *loop) : ownerLoop_(loop) {}

Poller::~Poller() = default;

// 判断参数channel是否在当前poller当中
bool Poller::hasChannel(Channel* channel) const
{
  assertInLoopThread();
  ChannelMap::const_iterator it = channels_.find(channel->fd());
  return it != channels_.end() && it->second == channel;
}

void Poller::assertInLoopThread() const {
  ownerLoop_->assertInLoopThread();
}