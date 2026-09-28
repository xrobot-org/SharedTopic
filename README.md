# SharedTopic

SharedTopic 是一个基于 UART 的多 Topic 数据共享与解析服务端模块：从串口接收
`SharedTopicClient`（或其他使用 LibXR Topic 打包格式的发送端）发来的 Topic 数据包，
解析后发布到本地 Topic。

SharedTopic is a UART-based multi-topic data sharing and parsing server module. It
receives Topic packets sent over a UART by `SharedTopicClient` (or any sender using
the LibXR Topic packet format), parses them and publishes them to local Topics.

## 运行方式 / Behaviour

- 构造时在 `topic_configs` 给出的 domain 中查找每个 Topic 并注册到
  `LibXR::Topic::Server`。Topic 必须已经存在（由之前构造的实例或 BSP 创建），
  否则打印 `Topic not found` 并触发 `ASSERT`。UART 必须有可读的 read port。
- 接收线程 `shared_topic`（MEDIUM 优先级，栈 2048）用 UART 的阻塞读等待可读事件，
  被唤醒后把已到达的字节按 `buffer_size` 分块读出，再调用
  `Topic::Server::ParseData()`。解析出的 Topic 以普通发布语义发布，并保留数据包中的
  envelope timestamp。
- 业务 Topic 在接收线程中发布，不在 UART 回调链中发布。
- `buffer_size` 同时是解析缓冲区和单次读取块的大小，必须不小于需要接收的最大单个
  Topic 数据包。

- The constructor looks up every Topic of `topic_configs` in its domain and
  registers it with a `LibXR::Topic::Server`. The Topics must already exist
  (created by an earlier instance or by the BSP); otherwise it logs
  `Topic not found` and fails an `ASSERT`. The UART must have a readable read port.
- The receive thread `shared_topic` (priority MEDIUM, stack 2048) waits on a
  blocking UART read, then drains the bytes that have arrived in chunks of up to
  `buffer_size` and passes them to `Topic::Server::ParseData()`. Parsed Topics are
  published with normal publish semantics and keep the envelope timestamp carried
  in the packet.
- Topics are published from the receive thread, never from the UART callback chain.
- `buffer_size` is both the parser buffer and the read chunk size; it must be at
  least the size of the largest single Topic packet to be received.

## RamFS 命令 / RamFS command

模块在 RamFS 中注册命令 `shared_topic:<uart_name>`（默认 `shared_topic:usart1`）：
The Module registers the command `shared_topic:<uart_name>` (default
`shared_topic:usart1`) in RamFS:

- `shared_topic:usart1`：打印用法 / print usage.
- `shared_topic:usart1 monitor <time_ms> <interval_ms>`：每 `interval_ms` 打印一次
  接收速率（Mbps），持续 `time_ms`。/ print the receive rate (Mbps) every
  `interval_ms` for `time_ms`.

## 依赖 / Dependencies

无其他模块依赖，仅使用 LibXR。
No other Modules; LibXR only.

## 构造接口 / Constructor

```cpp
SharedTopic(LibXR::UART& uart,
            LibXR::RamFS& ramfs,
            const char* uart_name = "usart1",
            uint32_t buffer_size = 256,
            std::initializer_list<TopicConfig> topic_configs = {"topic1", {"topic2", "libxr_def_domain"}});
```

依赖 / Dependencies:

- `uart`：接收数据的 `LibXR::UART`。/ The `LibXR::UART` the packets arrive on.
- `ramfs`：注册 `shared_topic:<uart_name>` 命令的 `LibXR::RamFS`。/ The
  `LibXR::RamFS` that receives the `shared_topic:<uart_name>` command.

配置 / Configuration:

- `uart_name`：仅用作 RamFS 命令名后缀，默认 `"usart1"`。/ Only used as the suffix
  of the RamFS command name, default `"usart1"`.
- `buffer_size`：解析缓冲区与读取块字节数，默认 256。/ Parser buffer and read chunk
  size in bytes, default 256.
- `topic_configs`：需要注册并分发的 Topic 列表。每项可以只写 Topic 名（使用
  `libxr_def_domain`），也可以写 `{topic, domain}`。默认值 `topic1` / `topic2` 只是
  占位，应改为实际的 Topic。/ Topics to register and dispatch. Each item is a Topic
  name (domain `libxr_def_domain`) or `{topic, domain}`. The defaults `topic1` /
  `topic2` are placeholders; replace them with real Topics.

## 使用 / Use

```sh
xrobot module add xrobot-org/SharedTopic
xrobot setup
xrobot instance add xrobot-org/SharedTopic
```

`xrobot instance add` 在 `User/xrobot.yaml` 中写入一个实例，依赖项留空，默认值按源码写出；
把依赖项填为 BSP 中用 `XR_REGISTER` 注册的对象名：
`xrobot instance add` writes an instance to `User/xrobot.yaml` with empty
dependencies and the source defaults; fill the dependencies with the names of the
objects the BSP registers with `XR_REGISTER`:

```yaml
modules:
  - module: xrobot-org/SharedTopic
    id: sharedtopic_0
    args:
      - uart: usart1
      - ramfs: ramfs
      - uart_name: '"usart1"'
      - buffer_size: '256'
      - topic_configs: '{"topic1", {"topic2", "libxr_def_domain"}}'
```

BSP 侧 / BSP side:

```cpp
XR_REGISTER(usart1, LibXR::UART);
XR_REGISTER(ramfs, LibXR::RamFS);
```

填好后再次运行 `xrobot setup`，生成 `User/xrobot_main.hpp`。
Run `xrobot setup` again to generate `User/xrobot_main.hpp`.

`xrobot module show .`（在本仓库中）或 `xrobot module show Modules/xrobot-org/SharedTopic`
（在 BSP 中）打印 manifest 和当前的构造函数。
`xrobot module show .` in this repository, or
`xrobot module show Modules/xrobot-org/SharedTopic` in a BSP, prints the manifest
and the current constructor.
