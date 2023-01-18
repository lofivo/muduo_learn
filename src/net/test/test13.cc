#include "src/net/EventLoop.h"
#include "src/net/InetAddress.h"
#include "src/net/TcpClient.h"

#include "src/logger/Logging.h"

#include <utility>

#include <stdio.h>
#include <unistd.h>

std::string message = "Hello\n";

void onConnection(const mymuduo::TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(),
           conn->peerAddress().toIpPort().c_str());
    conn->send(message);
  }
  else
  {
    printf("onConnection(): connection [%s] is down\n",
           conn->name().c_str());
  }
}

void onMessage(const mymuduo::TcpConnectionPtr& conn,
               mymuduo::Buffer* buf,
               mymuduo::Timestamp receiveTime)
{
  printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
         buf->readableBytes(),
         conn->name().c_str(),
         receiveTime.toFormattedString().c_str());

  printf("onMessage(): [%s]\n", buf->retrieveAllAsString().c_str());
}

int main()
{
  mymuduo::EventLoop loop;
  mymuduo::InetAddress serverAddr(9981);
  mymuduo::TcpClient client(&loop, serverAddr);

  client.setConnectionCallback(onConnection);
  client.setMessageCallback(onMessage);
  client.enableRetry();
  client.connect();
  loop.loop();
}

