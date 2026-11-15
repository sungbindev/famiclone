/*
 * ------------------------------------------------------------------------
 * 名称：stream-process-mac.cpp
 * 说明：流式读取子进程输出的 macOS 实现：posix_spawn，标准输入接空设备，标准输出与标准错误各用一个管道。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：管道带 FD_CLOEXEC，子进程只经 dup2 得到标准输出与标准错误；标准错误由后台线程读完；尚未在苹果电脑上编译与测试。
 * ------------------------------------------------------------------------
 */

#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <thread>

#include "platform/process/stream-process.hpp"

extern char** environ;

namespace ember::platform {

namespace {

/** 标准错误最多保留的字节数。 */
constexpr std::size_t ERROR_OUTPUT_LIMIT = std::size_t{64} * 1024;

/** 新建带 FD_CLOEXEC 的管道。 */
void createPipe(int fds[2]) {
  if (pipe(fds) != 0) throw ProcessError(std::string("无法创建管道：") + std::strerror(errno));
  fcntl(fds[0], F_SETFD, FD_CLOEXEC);
  fcntl(fds[1], F_SETFD, FD_CLOEXEC);
}

void closeFd(int& fd) {
  if (fd >= 0) close(fd);
  fd = -1;
}

class MacStreamProcess final : public StreamProcess {
 public:
  MacStreamProcess(pid_t pid, int output, int error) : pid_(pid), output_(output), error_(error) {
    errorReader_ = std::thread([this] {
      char buffer[4096];
      for (;;) {
        const auto count = ::read(error_, buffer, sizeof(buffer));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        std::lock_guard lock(errorMutex_);
        errorText_.append(buffer, static_cast<std::size_t>(count));
        if (errorText_.size() > ERROR_OUTPUT_LIMIT) errorText_.erase(0, errorText_.size() - ERROR_OUTPUT_LIMIT);
      }
    });
  }

  ~MacStreamProcess() override {
    terminate();
    int status = 0;
    while (waitpid(pid_, &status, 0) < 0 && errno == EINTR) {
    }
    if (errorReader_.joinable()) errorReader_.join();
    closeFd(output_);
    closeFd(error_);
  }

  std::size_t read(std::uint8_t* buffer, std::size_t size) override {
    for (;;) {
      const auto count = ::read(output_, buffer, size);
      if (count < 0 && errno == EINTR) continue;
      return count > 0 ? static_cast<std::size_t>(count) : 0;
    }
  }

  void terminate() override { kill(pid_, SIGKILL); }

  [[nodiscard]] std::string errorOutput() override {
    std::lock_guard lock(errorMutex_);
    return errorText_;
  }

 private:
  pid_t pid_;
  int output_;
  int error_;
  std::thread errorReader_;
  std::mutex errorMutex_;
  std::string errorText_;
};

}  // namespace

std::unique_ptr<StreamProcess> startStreamProcess(const std::filesystem::path& program, const std::vector<std::string>& arguments) {
  int output[2] = {-1, -1};
  int error[2] = {-1, -1};
  createPipe(output);
  createPipe(error);

  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
  posix_spawn_file_actions_adddup2(&actions, output[1], STDOUT_FILENO);
  posix_spawn_file_actions_adddup2(&actions, error[1], STDERR_FILENO);

  const std::string programPath = program.string();
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>(programPath.c_str()));
  for (const auto& argument : arguments) argv.push_back(const_cast<char*>(argument.c_str()));
  argv.push_back(nullptr);

  pid_t pid = 0;
  const int spawned = posix_spawn(&pid, programPath.c_str(), &actions, nullptr, argv.data(), environ);
  posix_spawn_file_actions_destroy(&actions);
  closeFd(output[1]);
  closeFd(error[1]);
  if (spawned != 0) {
    closeFd(output[0]);
    closeFd(error[0]);
    throw ProcessError("无法启动程序 " + program.filename().string() + "：" + std::strerror(spawned));
  }
  return std::make_unique<MacStreamProcess>(pid, output[0], error[0]);
}

}  // namespace ember::platform
