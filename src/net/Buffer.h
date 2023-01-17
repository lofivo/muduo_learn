#ifndef MYMUDUO_NET_BUFFER_H
#define MYMUDUO_NET_BUFFER_H
#include "src/base/noncopyable.h"
#include <algorithm>
#include <assert.h>
#include <string>
#include <vector>

namespace mymuduo {
class Buffer : noncopyable {
public:
  // prependable 初始大小，readIndex 初始位置
  static const size_t kCheapPrepend = 8;
  // writeable 初始大小，writeIndex 初始位置
  // 刚开始 readerIndex 和 writerIndex 处于同一位置
  static const size_t kInitialSize = 1024;
  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend) {}
  Buffer(Buffer &&) = default;
  Buffer &operator=(Buffer &&) = default;
  size_t readableBytes() const { return writerIndex_ - readerIndex_; }
  size_t writableBytes() const { return buffer_.size() - writerIndex_; }
  size_t prependableBytes() const { return readerIndex_; }
  // 返回缓冲区中可读数据的起始地址
  const char *peek() const { return begin() + readerIndex_; }
  void retrieve(size_t len) {
    // 应用只读取可读缓冲区数据的一部分(读取了len的长度)
    if (len < readableBytes()) {
      // 移动可读缓冲区指针
      readerIndex_ += len;
    }
    // 全部读完 len == readableBytes()
    else {
      retrieveAll();
    }
  }
  void retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }
  std::string retrieveAllAsString() {
    return retrieveAsString(readableBytes());
  }
  std::string retrieveAsString(size_t len) {
    assert(len <= readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }
  void ensureWriteableBytes(size_t len) {
    if (writableBytes() < len) {
      // 扩容
      makeSpace(len);
    }
  }
  void append(const char *data, size_t len) {
    ensureWriteableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
  }
  void append(const std::string &str) { append(str.data(), str.size()); }
  char *beginWrite() { return begin() + writerIndex_; }
  const char *beginWrite() const { return begin() + writerIndex_; }
  // 从fd上读取数据
  ssize_t readFd(int fd, int *saveErrno);
  // 通过fd发送数据
  ssize_t writeFd(int fd, int *saveErrno);

  const char *findCRLF(const char *start = nullptr) const {
    if (start != nullptr) {
      assert(peek() <= start);
      assert(start <= beginWrite());
    } else {
      start = peek();
    }
    const char *crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? NULL : crlf;
  }

private:
  char *begin() { return buffer_.data(); }
  const char *begin() const { return &*buffer_.begin(); }
  void makeSpace(size_t len) {
    // 如果整个buffer都不够用
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
      buffer_.resize(writerIndex_ + len);
    } else { // 如果整个buffer够用，将后面移动到前面继续分配
      size_t readable = readableBytes();
      std::copy(begin() + readerIndex_, begin() + writerIndex_,
                begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
    }
  }

private:
  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;
  static const char kCRLF[];
};
}; // namespace mymuduo

#endif // MYMUDUO_NET_BUFFER_H