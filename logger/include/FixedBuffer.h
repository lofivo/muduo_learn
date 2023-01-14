#ifndef MYMUDUO_LOGGER_FIXEDBUFFER_H
#define MYMUDUO_LOGGER_FIXEDBUFFER_H

#include "noncopyable.h"
#include <assert.h>
#include <string.h> // memcpy
#include <string>
#include <strings.h>

namespace mymuduo {
const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template <int SIZE> class FixedBuffer : noncopyable {
public:
  FixedBuffer() : cur_(data_) {}
  // 如果可用空间大于len,向缓冲流追加写入buf指针指向的字符串
  void append(const char *buf, size_t len) {
    if (static_cast<size_t>(avail()) > len) {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }
  // 返回buffer存储的字符串常量指针
  const char *data() const { return data_; }

  // 返回buffer存储的字符串长度
  int length() const { return static_cast<int>(cur_ - data_); }

  // 返回buffer当前要写入的位置
  char *current() { return cur_; }

  // 返回buffer可用空间大小
  int avail() const { return static_cast<int>(end() - cur_); }

  // 将cur指针前进len
  void add(size_t len) { cur_ += len; }

  // 将cur指针指向缓冲区开头
  void reset() { cur_ = data_; }

  // 将缓冲区内容全部置为0
  void bzero() { ::bzero(data_, sizeof(data_)); }

  // 将缓冲区存储的字符串内容转换为std::string并返回
  std::string toString() const { return std::string(data_, length()); }

private:
  const char *end() const { return data_ + sizeof(data_); }
  char data_[SIZE];
  char *cur_;
};
} // namespace mymuduo

#endif // MYMUDUO_LOGGER_FIXEDBUFFER_H