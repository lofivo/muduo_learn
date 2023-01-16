#include "src/net/TcpServer.h"
#include "src/logger/Logging.h"
#include "src/net/Acceptor.h"
#include "src/net/EventLoop.h"
#include "src/net/SocketsOps.h"

using namespace mymuduo;

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(CHECK_NOTNULL(loop)), name_(listenAddr.toHostPort()),
      acceptor_(new Acceptor(loop, listenAddr)), started_(false),
      nextConnId_(1) {
  acceptor_->setNewConnectionCallback(
      std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &name)
    : loop_(CHECK_NOTNULL(loop)), name_(name),
      acceptor_(new Acceptor(loop, listenAddr)), started_(false),
      nextConnId_(1) {
  acceptor_->setNewConnectionCallback(
      std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {}

void TcpServer::start() {
  if (!started_) {
    started_ = true;
  }

  if (!acceptor_->listenning()) {
    loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
  loop_->assertInLoopThread();
  char buf[32];
  snprintf(buf, sizeof buf, "#%d", nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_ << "] - new connection ["
           << connName << "] from " << peerAddr.toHostPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  TcpConnectionPtr conn(std::make_shared<TcpConnection>(loop_, connName, sockfd,
                                                        localAddr, peerAddr));

  connections_[connName] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->connectEstablished();
}
