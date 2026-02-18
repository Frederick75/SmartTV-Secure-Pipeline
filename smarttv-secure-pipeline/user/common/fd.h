#pragma once
#include <unistd.h>

class UniqueFd {
public:
  UniqueFd() = default;
  explicit UniqueFd(int fd): fd_(fd) {}
  ~UniqueFd() { reset(); }

  UniqueFd(const UniqueFd&) = delete;
  UniqueFd& operator=(const UniqueFd&) = delete;

  UniqueFd(UniqueFd&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
  UniqueFd& operator=(UniqueFd&& o) noexcept {
    if (this != &o) { reset(); fd_ = o.fd_; o.fd_ = -1; }
    return *this;
  }

  int get() const { return fd_; }
  int release() { int t = fd_; fd_ = -1; return t; }
  void reset(int nfd = -1) { if (fd_ >= 0) ::close(fd_); fd_ = nfd; }
  explicit operator bool() const { return fd_ >= 0; }

private:
  int fd_ = -1;
};
