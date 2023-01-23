#include "src/net/Socket.h"
#include "src/logger/Logging.h"
#include "src/net/InetAddress.h"

#include <cstring>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace mymuduo;

int Socket::createNonblockingFd() {
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_SYSFATAL << "Socket::createNonblockingFd()";
  }
  return sockfd;
}

int Socket::getSocketError(int sockfd) {
  int optval;
  socklen_t optlen = sizeof optval;
  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  } else {
    return optval;
  }
}

bool Socket::isSelfConnect(int sockfd) {
  sockaddr_in localaddr = InetAddress::getLocalAddr(sockfd);
  sockaddr_in peeraddr = InetAddress::getPeerAddr(sockfd);
  return localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr &&
         localaddr.sin_port == peeraddr.sin_port;
}

Socket::~Socket() {
  if (::close(sockfd_) < 0) {
    LOG_SYSERR << "Socket::~Socket()";
  }
}

void Socket::bindAddress(const InetAddress &localaddr) {
  if (0 != ::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(),
                  sizeof(sockaddr_in))) {
    LOG_FATAL << "Socket::bindAddr(): bind" << sockfd_ << " fail";
  }
}

void Socket::listen() {
  if (::listen(sockfd_, SOMAXCONN) < 0) {
    LOG_SYSFATAL << "Socket::listen()";
  }
}

int Socket::accept(InetAddress *peeraddr) {
  sockaddr_in addr;
  socklen_t len = sizeof addr;
  int connfd =
      ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd >= 0) {
    peeraddr->setSockAddr(addr);
  } else {
    LOG_SYSERR << "Socket::accept()";
  }
  return connfd;
}

void Socket::shutdownWrite() {
  if (::shutdown(sockfd_, SHUT_WR) < 0) {
    LOG_ERROR << "Socket::shutdownWrite error";
  }
}

void Socket::shutdown() {
  if (::shutdown(sockfd_, SHUT_RDWR) < 0) {
    LOG_ERROR << "Socket::shutdown error";
  }
}

// 不启动Nagle算法
void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval,
               sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

// 设置地址复用，其实就是可以使用处于Time-wait的端口
void Socket::setReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval,
                         static_cast<socklen_t>(sizeof(
                             optval))); // TCP_NODELAY包含头文件 <netinet/tcp.h>
  if (ret < 0) {
    LOG_SYSERR << "Socket::setReuseAddr()";
  }
}

// 通过改变内核信息，多个进程可以绑定同一个地址。通俗就是多个服务的ip+port是一样
void Socket::setReusePort(bool on) {
  int optval = on ? 1 : 0;
  int ret = setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval,
                       static_cast<socklen_t>(sizeof(optval)));
  if (ret < 0) {
    LOG_SYSERR << "Socket::setReusePort()";
  }
}

void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  int ret = setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval,
                       static_cast<socklen_t>(sizeof(optval)));
  if (ret < 0) {
    LOG_SYSERR << "Socket::setKeepAlive()";
  }
}
