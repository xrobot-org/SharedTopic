#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: 基于 UART 的多 Topic 数据接收与解析模块 / UART-based Module that receives and parses multi-Topic data
depends: []
=== END MANIFEST === */
// clang-format on

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "libxr_def.hpp"
#include "libxr_rw.hpp"
#include "libxr_type.hpp"
#include "logger.hpp"
#include "message.hpp"
#include "ramfs.hpp"
#include "semaphore.hpp"
#include "thread.hpp"
#include "uart.hpp"

/**
 * @brief 基于 UART 的 Topic 数据接收端，解析数据包并发布到本地 Topic。
 *        UART-based Topic receiver; parses the packets and publishes them to local
 *        Topics.
 */
class SharedTopic
{
 public:
  /**
   * @brief 需要注册并分发的 Topic。
   *        A Topic to register and dispatch.
   */
  struct TopicConfig
  {
    const char* name;                         ///< Topic 名称 Topic name
    const char* domain = "libxr_def_domain";  ///< Topic 所在的 domain Domain of the Topic

    /**
     * @brief 使用默认 domain `libxr_def_domain` 构造。
     *        Construct with the default domain `libxr_def_domain`.
     *
     * @param name Topic 名称。
     *             Topic name.
     */
    TopicConfig(const char* name) : name(name) {}

    /**
     * @brief 构造并指定 domain。
     *        Construct with an explicit domain.
     *
     * @param name Topic 名称。
     *             Topic name.
     * @param domain Topic 所在的 domain。
     *               Domain of the Topic.
     */
    TopicConfig(const char* name, const char* domain) : name(name), domain(domain) {}
  };

  /**
   * @brief 构造 SharedTopic：注册 topic_configs 中的 Topic，添加命令并创建接收线程。
   *        Construct SharedTopic: register the Topics of topic_configs, add the RamFS
   *        command and create the receive thread.
   *
   * @param uart 接收数据包的 UART，须有可读的 read port。
   *             UART the packets arrive on; it must have a readable read port.
   * @param ramfs 接收 `shared_topic:<uart_name>` 命令的 RamFS。
   *              RamFS that receives the `shared_topic:<uart_name>` command.
   * @param uart_name RamFS 命令名的后缀。
   *                  Suffix of the RamFS command name.
   * @param buffer_size 解析缓冲区与读取块的字节数。
   *                    Size in bytes of the parser buffer and the read chunk.
   * @param topic_configs 需要注册并分发的 Topic 列表，Topic 须已存在。
   *                      Topics to register and dispatch; they must already exist.
   */
  SharedTopic(LibXR::UART& uart, LibXR::RamFS& ramfs, const char* uart_name = "usart1",
              uint32_t buffer_size = 256,
              std::initializer_list<TopicConfig> topic_configs = {"topic1",
                                                                  {"topic2",
                                                                   "libxr_def_domain"}})
      : uart_(std::addressof(uart)),
        server_(buffer_size),
        rx_buffer_(new uint8_t[buffer_size], buffer_size),
        cmd_name_(new char[sizeof("shared_topic:") + strlen(uart_name)]),
        cmd_file_((strcpy(cmd_name_, "shared_topic:"),
                   strcpy(cmd_name_ + strlen("shared_topic:"), uart_name),
                   LibXR::RamFS::CreateFile(cmd_name_, CommandFunc, this)))
  {
    ASSERT(uart_->read_port_ != nullptr);
    ASSERT(uart_->read_port_->Readable());

    for (auto config : topic_configs)
    {
      auto domain = LibXR::Topic::Domain(config.domain);
      auto topic = LibXR::Topic::Find(config.name, &domain);
      if (topic == nullptr)
      {
        XR_LOG_ERROR("Topic not found: %s/%s", config.domain, config.name);
        ASSERT(false);
      }
      server_.Register(topic);
    }

    ramfs.Add(cmd_file_);

    rx_thread_.Create<SharedTopic*>(this, RxThread, "shared_topic", 2048,
                                    LibXR::Thread::Priority::MEDIUM);
  }

  /**
   * @brief 命令入口：无参数打印用法，`monitor <time_ms> <interval_ms>` 打印接收速率。
   *        Command entry: print the usage without arguments; `monitor <time_ms>
   *        <interval_ms>` prints the receive rate.
   *
   * @param self SharedTopic 实例。
   *             SharedTopic instance.
   * @param argc 参数个数。
   *             Argument count.
   * @param argv 参数列表。
   *             Argument list.
   * @return 成功为 0，参数个数无效为 -1。
   *         0 on success, -1 for an invalid argument count.
   */
  static int CommandFunc(SharedTopic* self, int argc, char** argv)
  {
    if (argc == 1)
    {
      LibXR::STDIO::Printf<"Usage:\r\n">();
      LibXR::STDIO::Printf<
          "  monitor [time_ms] [interval_ms] - test received speed\r\n">();
      return 0;
    }
    else if (argc == 4)
    {
      if (strcmp(argv[1], "monitor") == 0)
      {
        int time = atoi(argv[2]);
        int delay = atoi(argv[3]);
        auto start = self->rx_count_;
        while (time > 0)
        {
          LibXR::Thread::Sleep(delay);
          LibXR::STDIO::Printf<"%f Mbps\r\n">(
              static_cast<float>(self->rx_count_ - start) * 8.0 / 1024.0 / 1024.0 /
              delay * 1000.0);
          time -= delay;
          start = self->rx_count_;
        }
      }
    }
    else
    {
      LibXR::STDIO::Printf<"Error: Invalid arguments.\r\n">();
      return -1;
    }

    return 0;
  }

 private:
  static void RxThread(SharedTopic* self) { self->RunRxLoop(); }

  void RunRxLoop()
  {
    LibXR::ReadOperation wait_op(rx_sem_);
    LibXR::ReadOperation read_op;

    while (true)
    {
      auto read_status = uart_->Read({nullptr, 0}, wait_op);
      if (read_status != LibXR::ErrorCode::OK)
      {
        continue;
      }

      while (uart_->read_port_->Size() > 0)
      {
        auto size = LibXR::min(uart_->read_port_->Size(), rx_buffer_.size_);
        read_status = uart_->Read(LibXR::RawData{rx_buffer_.addr_, size}, read_op);
        if (read_status != LibXR::ErrorCode::OK)
        {
          break;
        }

        server_.ParseData(LibXR::ConstRawData{rx_buffer_.addr_, size});
        rx_count_ += size;
      }
    }
  }

  LibXR::UART* uart_;

  LibXR::Topic::Server server_;

  LibXR::RawData rx_buffer_;

  size_t rx_count_ = 0;

  char* cmd_name_;

  LibXR::RamFS::File cmd_file_;

  LibXR::Semaphore rx_sem_;
  LibXR::Thread rx_thread_;
};
