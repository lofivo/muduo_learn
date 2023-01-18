#include "src/net/TcpServer.h"
#include "src/net/EventLoop.h"
#include "src/net/InetAddress.h"
#include <stdio.h>
#include <unistd.h>
void onConnection(const mymuduo::TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    printf("onConnection(): tid=%d new connection [%s] from %s\n",
           gettid(),
           conn->name().c_str(),
           conn->peerAddress().toIpPort().c_str());
  }
  else
  {
    printf("onConnection(): tid=%d connection [%s] is down\n",
           gettid(),
           conn->name().c_str());
  }
}

void onMessage(const mymuduo::TcpConnectionPtr& conn,
               mymuduo::Buffer* buf,
               mymuduo::Timestamp receiveTime)
{
  printf("onMessage(): tid=%d received %zd bytes from connection [%s] at %s\n",
         gettid(),
         buf->readableBytes(),
         conn->name().c_str(),
         receiveTime.toFormattedString().c_str());

  conn->send(buf->retrieveAllAsString());
}

int main(int argc, char* argv[])
{
  printf("main(): pid = %d\n", getpid());

  mymuduo::InetAddress listenAddr(9981);
  mymuduo::EventLoop loop;

  mymuduo::TcpServer server(&loop, listenAddr);
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  if (argc > 1) {
    server.setThreadNum(atoi(argv[1]));
  }
  server.start();

  loop.loop();
}
