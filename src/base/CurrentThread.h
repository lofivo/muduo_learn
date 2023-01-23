#ifndef MYMUDUO_BASE_CURRENTTHREAD_H
#define MYMUDUO_BASE_CURRENTTHREAD_H
#include <string.h>
#include <stdint.h>
#include <thread>
namespace mymuduo {
namespace CurrentThread {
uint64_t get_id() {
  std::thread::id id = std::this_thread::get_id();
  uint64_t tid;
  memcpy(&id, &id, 8);
  return tid;
}
} // namespace CurrentThread
}

#endif // MYMUDUO_BASE_CURRENTTHREAD_H