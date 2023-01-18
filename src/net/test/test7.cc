#include "src/net/Acceptor.h"
#include "src/net/EventLoop.h"
#include "src/net/InetAddress.h"
#include <stdio.h>
#include <unistd.h>

void newConnection(int sockfd, const mymuduo::InetAddress& peerAddr)
{
  printf("newConnection(): accepted a new connection from %s\n",
         peerAddr.toIpPort().c_str());
  ::write(sockfd, "How are you?\n", 13);
  ::close(sockfd);
}

int main()
{
  printf("main(): pid = %d\n", getpid());

  mymuduo::InetAddress listenAddr(9981);
  mymuduo::EventLoop loop;

  mymuduo::Acceptor acceptor(&loop, listenAddr);
  acceptor.setNewConnectionCallback(newConnection);
  acceptor.listen();

  loop.loop();
}
