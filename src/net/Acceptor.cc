#include "src/net/Acceptor.h"
#include "src/logger/Logging.h"
#include "src/net/EventLoop.h"
#include "src/net/InetAddress.h"

#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace mymuduo;

static int createNonblocking() {
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_FATAL << "createNonblocking error";
  }
  return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr,
                   bool reuseport)
    : loop_(loop), acceptSocket_(createNonblocking()),
      acceptChannel_(loop, acceptSocket_.fd()), listening_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(reuseport);
  acceptSocket_.bindAddress(listenAddr);
  /**
   * TcpServer::start() => Acceptor.listen
   * 有新用户的连接，需要执行一个回调函数
   * 因此向封装了acceptSocket_的channel注册回调函数
   */
  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
  // 把从Poller中感兴趣的事件删除掉
  acceptChannel_.disableAll();
  // 调用EventLoop->removeChannel => Poller->removeChannel
  // 把Poller的ChannelMap对应的部分删除
  acceptChannel_.remove();
}

void Acceptor::listen() {
  loop_->assertInLoopThread();
  listening_ = true;
  acceptSocket_.listen();
  // 将acceptChannel的读事件注册到poller
  acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
  loop_->assertInLoopThread();
  InetAddress peerAddr;
  // 接受新连接
  int connfd = acceptSocket_.accept(&peerAddr);
  // 如果新连接到来
  if (connfd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peerAddr);
    } else {
      if (::close(connfd) < 0) {
        LOG_SYSERR << "Socket::handleRead(), close connected fd error";
      }
    }
  } else {
    LOG_SYSERR << "in Acceptor::handleRead";
    // 当前进程的fd已经用完了
    // 可以调整单个服务器的fd上限
    // 也可以分布式部署
    if (errno == EMFILE) {
      LOG_ERROR << "sockfd reached limit";
      ::close(idleFd_); // 关闭后可能被其他线程抢占fd
      idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
      ::close(idleFd_);
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}
