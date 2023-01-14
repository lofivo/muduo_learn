#include "FileUtil.h"
#include "Logging.h"

using namespace mymuduo;

FileUtil::FileUtil(std::string &fileName)
    : fp_(::fopen(fileName.c_str(), "ae")), // 'e' for O_CLOEXEC
      writtenBytes_(0) {
  // 将fd_缓冲区设置为本地的buffer_
  ::setbuffer(fp_, buffer_, sizeof(buffer_));
}

FileUtil::~FileUtil() { ::fclose(fp_); }

void FileUtil::append(const char *data, size_t len) {
  // 记录已经写入的数据大小
  size_t written = 0;

  while (written != len) {
    // 还需写入的数据大小
    size_t remain = len - written;
    size_t n = write(data + written, remain);
    if (n != remain) {
      int err = ferror(fp_);
      if (err) {
        fprintf(stderr, "FileUtil::append() failed %s\n", getErrnoMsg(err));
      }
    }
    // 更新写入的数据大小
    written += n;
  }
  // 记录目前为止写入的数据大小，超过限制会滚动日志
  writtenBytes_ += written;
}

// 刷新文件流
void FileUtil::flush() { ::fflush(fp_); }


size_t FileUtil::write(const char *data, size_t len) {
  // fwrite_unlocked是fwrite的无锁版本
  return ::fwrite_unlocked(data, 1, len, fp_);
}