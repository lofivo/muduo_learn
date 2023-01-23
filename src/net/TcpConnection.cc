#include "src/net/TcpConnection.h"
#include "src/net/Channel.h"
#include "src/net/EventLoop.h"
#include "src/net/Socket.h"

#include <atomic>
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
  send(std::string(static_cast<const char *>(data), len));
}

void TcpConnection::send(const std::string &msg) {
  if (state_ == kConnected) {
    loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, msg));
  }
}

void TcpConnection::send(Buffer *buf) {
  if (state_ == kConnected) {
    loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this,
                               buf->retrieveAllAsString()));
  }
}

void TcpConnection::sendInLoop(const std::string &message) {
  loop_->assertInLoopThread();
  if (state_ == kDisconnected) {
    LOG_WARN << "fd " << channel_->fd() << " disconnected, give up writing";
    return;
  }

  ssize_t n = 0;
  ssize_t remain = message.size();
  if (!channel_->isWriting() &&
      outputBuffer_.readableBytes() == 0) { // 没有待处理的写事件时直接write
    n = ::write(channel_->fd(), message.c_str(), message.size());
    if (n >= 0) {
      remain -= n;
      if (remain == 0 && writeCompleteCallback_) {
        loop_->queueInLoop(std::bind(
            writeCompleteCallback_,
            shared_from_this())); // queueInLoop会在下一次处理pendingFuncs时执行，runInLoop可能会直接执行
      }
    } else {
      if (errno != EWOULDBLOCK) {
        LOG_SYSERR << "TcpConnection::sendInLoop()";
      }
    }
  }

  if (remain > 0) {
    outputBuffer_.append(message.data() + n, remain);
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdown() {
  StateE connected = kConnected;
  if (state_.compare_exchange_strong(connected, kDisconnecting)) {
    assert(state_ == kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();
  // 当前outputBuffer_的数据全部向外发送完成
  if (!channel_->isWriting()) {
    socket_->shutdown();
  }
}

void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading(); // channel -> EPOLLIN

  // new connection callback
  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();
  if (state_ == kConnected) {
    setState(kDisconnected);
    channel_->disableAll(); // 把channel的所有感兴趣的事件从poller中删除掉
    connectionCallback_(shared_from_this());
  }
  channel_->remove(); // 把channel从poller中删除掉
}

void TcpConnection::forceClose() {

  if (state_ == kConnected || state_ == kDisconnecting) {
    LOG_INFO << "forceClose running";
    setState(kDisconnected);
    loop_->queueInLoop(
        std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseInLoop() {
  LOG_INFO << "forceCloseInLoop()";
  if (state_ == kConnected || state_ == kDisconnecting) {
    // as if we received 0 byte in handleRead();
    handleClose();
  }
}

void TcpConnection::setTcpNoDelay(bool on) { socket_->setTcpNoDelay(on); }

void TcpConnection::handleRead(Timestamp receiveTime) {
  loop_->assertInLoopThread();
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
  }
}

void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();

  if (channel_->isWriting()) { // 这里也可用 kConnected | kDisconnecting判断
    ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(),
                        outputBuffer_.readableBytes());
    if (n > 0) {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0) {
        channel_->disableWriting();
        if (writeCompleteCallback_) {
          loop_->queueInLoop(
              std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting) {
          shutdownInLoop();
        }
      }
    } else {
      LOG_SYSERR << "TcpConnection::handleWrite()";
    }
  } else { // 写之前对方就关闭了连接，服务端调用TcpConnection::handleClose()关闭了channel
    LOG_TRACE << "Connection fd = " << channel_->fd()
              << " is down, no more writing";
  }
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  LOG_TRACE << "handleClose fd = " << channel_->fd()
           << " state = " << stateToString();
  assert(state_ == kConnected || state_ == kDisconnecting);
  setState(kDisconnected); // 设置状态为关闭连接状态
  channel_->disableAll();  // 注销Channel所有感兴趣事件

  TcpConnectionPtr connPtr(shared_from_this());
  connectionCallback_(connPtr);
  closeCallback_(connPtr); // 关闭连接的回调
}

void TcpConnection::handleError() {
  int err;
  socklen_t errlen = sizeof(err);
  if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &err, &errlen) < 0) {
    err = errno;
  }
  LOG_ERROR << "TcpConnection::handleError [" << name_
            << "]: SO_ERROR = " << err;
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