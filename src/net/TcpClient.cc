#include "src/net/TcpClient.h"
#include "src/logger/Logging.h"
#include "src/net/Connector.h"
#include "src/net/EventLoop.h"
#include "src/net/Socket.h"

using namespace mymuduo;
static EventLoop *CheckLoopNotNull(EventLoop *loop) {
  if (loop == nullptr) {
    LOG_FATAL << "TcpConnection Loop is null";
  }
  return loop;
}
namespace mymuduo::detail {
void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn) {
  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
} // namespace mymuduo::detail

TcpClient::TcpClient(EventLoop *loop, const InetAddress &serverAddr,
                     const std::string &nameArg)
    : loop_(CheckLoopNotNull(loop)),
      connector_(std::make_shared<Connector>(loop, serverAddr)), name_(nameArg),
      connectionCallback_(TcpConnection::defaultConnectionCallback),
      messageCallback_(TcpConnection::defaultMessageCallback) {
  connector_->setNewConnectionCallback(
      std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
}

TcpClient::~TcpClient() {
  LOG_INFO << "TcpClient::~TcpClient[" << name_ << "] - connector "
           << connector_.get();
  TcpConnectionPtr conn;
  bool unique = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }
  if (conn) {
    CloseCallback cb =
        std::bind(&detail::removeConnection, loop_, std::placeholders::_1);
    loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
    if (unique) {
      conn->forceClose();
    }
  } else {
    connector_->stop();
  }
}

void TcpClient::connect() {
  if (connector_->connected())
    return;
  LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
           << connector_->serverAddress().toIpPort();
  connect_.store(true);
  connector_->start();
}
void TcpClient::disconnect() {
  connect_.store(false);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_) {
      connection_->shutdown();
    }
  }
}
void TcpClient::stop() {
  connect_.store(false);
  connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
  InetAddress peerAddr(InetAddress::getPeerAddr(sockfd));
  char buf[32]{0};
  snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(),
           nextConnId_++);
  std::string connName = name_ + buf;

  InetAddress localAddr(InetAddress::getLocalAddr(sockfd));
  TcpConnectionPtr conn(std::make_shared<TcpConnection>(loop_, connName, sockfd,
                                                        localAddr, peerAddr));
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
  {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_ = conn;
  }
  conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_.reset();
  }

  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  if (retry_.load() && connect_.load()) {
    LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
             << connector_->serverAddress().toIpPort();
    connector_->restart();
  }
}