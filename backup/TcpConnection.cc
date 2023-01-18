#include "src/net/TcpConnection.h"
#include "src/logger/Logging.h"
#include "src/net/Channel.h"
#include "src/net/EventLoop.h"
#include "src/net/Socket.h"

using namespace mymuduo;

static EventLoop *CheckLoopNotNull(EventLoop *loop) {
  if (loop == nullptr) {
    LOG_FATAL << "TcpConnection Loop is null";
  }
  return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &nameArg,
                             int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CHECK_NOTNULL(loop)), name_(nameArg), state_(kConnecting),
      socket_(new Socket(sockfd)), channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr), peerAddr_(peerAddr) {
  LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
            << " fd=" << sockfd;
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
}

TcpConnection::~TcpConnection() {
  LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
            << " fd=" << channel_->fd();
}

void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

void TcpConnection::handleRead() {
  char buf[65536];
  ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
  messageCallback_(shared_from_this(), buf, n);
  // FIXME: close connection if n == 0
}
