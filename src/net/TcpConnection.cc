#include "src/net/TcpConnection.h"
#include "src/net/Channel.h"
#include "src/net/EventLoop.h"
#include "src/net/Socket.h"

#include <errno.h>
#include <functional>
#include <unistd.h>

using namespace mymuduo;

static EventLoop *CheckLoopNotNull(EventLoop *loop) {
  // 如果传入EventLoop没有指向有意义的地址则出错
  // 正常来说在 TcpServer::start 这里就生成了新线程和对应的EventLoop
  if (loop == nullptr) {
    LOG_FATAL << "TcpConnection Loop is null!";
  }
  return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &nameArg,
                             int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop)), name_(nameArg), state_(kConnecting),
      reading_(true), socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)), localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) // 64M 避免发送太快对方接受太慢
{
  // 下面给channel设置相应的回调函数 poller给channel通知感兴趣的事件发生了
  // channel会回调相应的回调函数
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

  LOG_INFO << "TcpConnection::ctor[" << name_.data() << "] at fd =" << sockfd;
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG_INFO << "TcpConnection::dtor[" << name_ << "] at fd=" << channel_->fd()
           << " state=" << stateToString();
}

const char *TcpConnection::stateToString() const {
  switch (state_.load()) {
  case kDisconnected:
    return "kDisconnected";
  case kConnecting:
    return "kConnecting";
  case kConnected:
    return "kConnected";
  case kDisconnecting:
    return "kDisconnecting";
  default:
    return "unknown state";
  }
}

void TcpConnection::send(const void *data, size_t len) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(data, len);
    } else {
      loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, data, len));
    }
  }
}

void TcpConnection::send(const std::string &msg) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(msg.data(), msg.size());
    } else {
      loop_->runInLoop(
          std::bind(&TcpConnection::sendInLoop, this, msg.data(), msg.size()));
    }
  }
}

void TcpConnection::send(Buffer *buf) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    } else {
      std::string buf_s = buf->retrieveAllAsString();
      loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf_s.data(),
                                 buf_s.size()));
    }
  }
}

void TcpConnection::sendInLoop(const void *data, size_t len) {
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;

  // 之前调用过connection得shutdown，不能再进行发送了
  if (state_ == kDisconnected) {
    LOG_WARN << "disconnected, give up writing";
    return;
  }

  // channel第一次写数据，且缓冲区没有待发送数据
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    nwrote = ::write(channel_->fd(), data, len);
    if (nwrote >= 0) {
      // 判断有没有一次性写完
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_) {
        // 既然一次性发送完事件就不用让channel对epollout事件感兴趣了
        loop_->queueInLoop(
            std::bind(writeCompleteCallback_, shared_from_this()));
      }
    } else // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG_SYSERR << "TcpConnection::sendInLoop";
        if (errno == EPIPE || errno == ECONNRESET) { // SIGPIPE RESET
          faultError = true;
        }
      }
    }
  }

  // 说明一次性并没有发送完数据，剩余数据需要保存到缓冲区中，且需要改channel注册写事件
  if (!faultError && remaining > 0) {
    size_t oldLen = outputBuffer_.readableBytes();
    if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ &&
        highWaterMarkCallback_) {
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(),
                                   oldLen + remaining));
    }
    outputBuffer_.append((char *)data + nwrote, remaining);
    if (!channel_->isWriting()) {
      channel_->enableWriting(); // 这里一定要注册channel的写事件
                                 // 否则poller不会给channel通知epollout
    }
  }
}

void TcpConnection::shutdown() {
  if (state_ == kConnected) {
    setState(kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop() {
  // 当前outputBuffer_的数据全部向外发送完成
  if (!channel_->isWriting()) {
    socket_->shutdownWrite();
  }
}

void TcpConnection::connectEstablished() {
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading(); // channel -> EPOLLIN

  // new connection callback
  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  if (state_ == kConnected) {
    setState(kDisconnected);
    channel_->disableAll(); // 把channel的所有感兴趣的事件从poller中删除掉
    connectionCallback_(shared_from_this());
  }
  channel_->remove(); // 把channel从poller中删除掉
}

void TcpConnection::forceClose() {
  if (state_ == kConnected || state_ == kDisconnecting) {
    setState(kDisconnected);
    loop_->queueInLoop(
        std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseInLoop() {
  if (state_ == kConnected || state_ == kDisconnecting) {
    // as if we received 0 byte in handleRead();
    handleClose();
  }
}

void TcpConnection::handleRead(Timestamp receiveTime) {
  int savedErrno = 0;
  // TcpConnection会从socket读取数据，然后写入inpuBuffer
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0) {
    // 已建立连接的用户，有可读事件发生，调用用户传入的回调操作
    // TODO:shared_from_this
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  } else if (n == 0) {
    // 没有数据，说明客户端关闭连接
    handleClose();
  } else {
    // 出错情况
    errno = savedErrno;
    LOG_SYSERR << "TcpConnection::handleRead() failed";
    handleError();
  }
}

void TcpConnection::handleWrite() {
  if (channel_->isWriting()) {
    int saveErrno = 0;
    ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
    // 正确读取数据
    if (n > 0) {
      outputBuffer_.retrieve(n);
      // 说明buffer可读数据都被TcpConnection读取完毕并写入给了客户端
      // 此时就可以关闭连接，否则还需继续提醒写事件
      if (outputBuffer_.readableBytes() == 0) {
        channel_->disableWriting();
        // 调用用户自定义的写完数据处理函数
        if (writeCompleteCallback_) {
          // 唤醒loop_对应得thread线程，执行写完成事件回调
          loop_->queueInLoop(
              std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting) {
          shutdownInLoop();
        }
      }
    } else {
      LOG_SYSERR << "TcpConnection::handleWrite() failed";
    }
  }
  // state_不为写状态
  else {
    LOG_TRACE << "TcpConnection fd=" << channel_->fd()
              << " is down, no more writing";
  }
}

void TcpConnection::handleClose() {
  LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
  setState(kDisconnected); // 设置状态为关闭连接状态
  channel_->disableAll();  // 注销Channel所有感兴趣事件

  TcpConnectionPtr connPtr(shared_from_this());
  connectionCallback_(connPtr);
  closeCallback_(connPtr); // 关闭连接的回调
}

void TcpConnection::handleError() {
  int optval;
  socklen_t optlen = sizeof(optval);
  int err = 0;
  if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen)) {
    err = errno;
  } else {
    err = optval;
  }
  LOG_ERROR << "cpConnection::handleError name:" << name_.c_str()
            << " - SO_ERROR:" << err;
}

void TcpConnection::defaultConnectionCallback(const TcpConnectionPtr &conn) {
  LOG_TRACE << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
  // do not call conn->forceClose(), because some users want to register message
  // callback only.
}

void TcpConnection::defaultMessageCallback(const TcpConnectionPtr &conn,
                                           Buffer *buffer,
                                           Timestamp receiveTime) {
  buffer->retrieveAll();
}