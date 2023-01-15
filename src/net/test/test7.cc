#include "src/base/Thread.h"
#include "src/net/Acceptor.h"
#include "src/net/EventLoop.h"
#include "src/net/InetAddress.h"
#include "src/net/SocketsOps.h"
#include <functional>
#include <stdio.h>
#include <string>
#include <time.h>

const std::string currentDateTime() {
  time_t now = time(0);
  struct tm tstruct;
  char buf[80];
  tstruct = *localtime(&now);
  strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

  return buf;
}

void newConnection(int sockfd, const mymuduo::InetAddress &peerAddr) {
  printf("newConnection(): accepted a new connection from %s\n",
         peerAddr.toHostPort().c_str());
  std::string now_time = currentDateTime() + '\n';
  ::write(sockfd, now_time.c_str(), now_time.size());
  mymuduo::sockets::close(sockfd);
}

void newConnection2(int sockfd, const mymuduo::InetAddress &peerAddr) {
  printf("newConnection2(): accepted a new connection from %s\n",
         peerAddr.toHostPort().c_str());
  ::write(sockfd, "hello\n", 6);
  mymuduo::sockets::close(sockfd);
}

void threadFunc() {
  printf("new thread: pid = %d, tid = %d\n", getpid(), gettid());
  mymuduo::InetAddress listenAddr(9984);
  mymuduo::EventLoop loop;
  mymuduo::Acceptor acceptor(&loop, listenAddr);
  acceptor.setNewConnectionCallback(newConnection2);
  acceptor.listen();
  loop.loop();
}

int main() {
  printf("main thread: pid = %d, tid = %d\n", getpid(), gettid());
  mymuduo::InetAddress listenAddr(9983);
  mymuduo::EventLoop loop;
  mymuduo::Acceptor acceptor(&loop, listenAddr);
  acceptor.setNewConnectionCallback(newConnection);
  acceptor.listen();

  mymuduo::Thread t(threadFunc);
  t.start();
  loop.loop();

  return 0;
}
