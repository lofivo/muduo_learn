#include "src/net/Connector.h"
#include "src/logger/Logging.h"
#include "src/net/Channel.h"
#include "src/net/EventLoop.h"
#include "src/net/InetAddress.h"
#include "src/net/Socket.h"

#include <assert.h>
#include <error.h>
#include <unistd.h>

using namespace mymuduo;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop), serverAddr_(serverAddr), connect_(false),
      state_(kDisconnected), retryDelayMs_(kInitRetryDelayMs) {
  LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector() {
  LOG_DEBUG << "dtor[" << this << "]";
  assert(!channel_);
}

void Connector::start() {
  connect_.store(true);
  loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop() {
  if (connect_.load()) {
    connect();
  } else {
    LOG_DEBUG << "do not connect";
  }
}

void Connector::restart() {
  setState(kConnecting);
  retryDelayMs_ = kInitRetryDelayMs;
  startInLoop();
}

void Connector::stop() {
  connect_.store(false);
  loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop() {
  if (state_.load() == kConnecting) {
    setState(kDisconnected);
    int sockfd = removeAndResetChannel();
    retry(sockfd);
  }
}

void Connector::connect() {
  int sockfd = Socket::createNonblockingFd();
  socklen_t len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
  int ret = ::connect(sockfd, (const sockaddr *)serverAddr_.getSockAddr(), len);
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno) {
  case 0:
  case EINPROGRESS:
  case EINTR:
  case EISCONN:
    connecting(sockfd);
    break;

  case EAGAIN:
  case EADDRINUSE:
  case EADDRNOTAVAIL:
  case ECONNREFUSED:
  case ENETUNREACH:
    retry(sockfd);
    break;

  case EACCES:
  case EPERM:
  case EAFNOSUPPORT:
  case EALREADY:
  case EBADF:
  case EFAULT:
  case ENOTSOCK:
    LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
    ::close(sockfd);
    break;

  default:
    LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
    ::close(sockfd);
    break;
  }
}

void Connector::connecting(int sockfd) {
  setState(kConnecting);
  assert(!channel_);

  channel_.reset(new Channel(loop_, sockfd));
  channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
  channel_->setErrorCallback(std::bind(&Connector::handleError, this));
  channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
  channel_->disableAll();
  channel_->remove();
  int sockfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  loop_->queueInLoop(
      std::bind(&Connector::resetChannel, this));
  return sockfd;
}

void Connector::resetChannel() { channel_.reset(); }

// socket writeable doesn't mean connect
void Connector::handleWrite() {
  LOG_TRACE << "Connector::handleWrite " << state_;

  if (state_.load() == kConnecting) {
    int sockfd = removeAndResetChannel();
    int err = Socket::getSocketError(sockfd);
    if (err) {
      LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err;
      retry(sockfd);
    } else if (Socket::isSelfConnect(sockfd)) {
      LOG_WARN << "Connector::handleWrite - Self connect";
      retry(sockfd);
    } else {
      setState(kConnected);
      if (connect_.load()) {
        if (newConnectionCallback_)
          newConnectionCallback_(sockfd);
        else
          ::close(sockfd);
      } else {
        ::close(sockfd);
      }
    }
  } else {
    assert(state_.load() == kDisconnected);
  }
}

void Connector::handleError() {
  LOG_ERROR << "Connector::handleError state=" << state_;
  if (state_.load() == kConnecting) {
    int sockfd = removeAndResetChannel();
    int err = Socket::getSocketError(sockfd);
    LOG_TRACE << "SO_ERROR = " << err;
    retry(sockfd);
  }
}

void Connector::retry(int sockfd) {
  ::close(sockfd);
  setState(kDisconnected);
  if (connect_.load()) {
    LOG_INFO << "Connector::retry - Retry connecting to "
             << serverAddr_.toIpPort() << " in " << retryDelayMs_
             << " milliseconds. ";
    loop_->runAfter(retryDelayMs_ / 1000.0,
                    std::bind(&Connector::startInLoop, shared_from_this()));
    retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
  } else {
    LOG_DEBUG << "do not connect";
  }
}