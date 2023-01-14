#include "src/net/EventLoop.h"
#include "src/base/Thread.h"

// ERROR:在不是创建EventLoop的线程运行loop
// mymuduo::EventLoop* g_loop;

// void threadFunc()
// {
//   g_loop->loop();
// }

// int main()
// {
//   mymuduo::EventLoop loop;
//   g_loop = &loop;
//   mymuduo::Thread t(threadFunc);
//   t.start();
//   t.join();
// }

// ERROR:一个线程创建了2个EventLoop对象
int main() {
  mymuduo::EventLoop loop1, loop2;


  return 0;
}
