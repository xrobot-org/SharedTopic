# SharedTopic

基于 UART 的多 Topic 数据接收与解析模块 / UART-based Module that receives and parses multi-Topic data

## 1. 模块作用 / Purpose

SharedTopic 从 UART 接收 `SharedTopicClient`（或其他使用 LibXR Topic 打包格式的发送端）发来的 Topic 数据包，解析后发布到本地 Topic。

- 构造时，SharedTopic 在 `topic_configs` 给出的 domain 中查找每个 Topic，并注册到 `LibXR::Topic::Server`。Topic 须已由先构造的实例或 BSP 创建，找不到时打印 `Topic not found` 并触发 `ASSERT`。UART 须有可读的 read port。
- 接收线程 `shared_topic`（`MEDIUM` 优先级，栈 2048）用 UART 的阻塞读等待可读事件，被唤醒后把已到达的字节按 `buffer_size` 分块读出，再调用 `Topic::Server::ParseData()`。解析出的 Topic 以普通发布语义发布，并保留数据包中的 envelope timestamp。
- Topic 在接收线程中发布。
- `buffer_size` 同时是解析缓冲区和单次读取块的大小，须不小于需要接收的最大单个 Topic 数据包。

On receipt, SharedTopic takes the Topic packets sent over a UART by `SharedTopicClient` (or any sender using the LibXR Topic packet format), parses them and publishes them to local Topics.

- Upon construction, SharedTopic looks up every Topic of `topic_configs` in its domain and registers it with a `LibXR::Topic::Server`. The Topics must already have been created by an earlier instance or by the BSP; when one is not found, `Topic not found` is logged and an `ASSERT` fails. The UART must have a readable read port.
- The receive thread `shared_topic` (`MEDIUM` priority, stack 2048) waits on a blocking UART read, then drains the bytes that have arrived in chunks of up to `buffer_size` and passes them to `Topic::Server::ParseData()`. Parsed Topics are published with normal publish semantics and keep the envelope timestamp carried in the packet.
- Topics are published from the receive thread.
- `buffer_size` is both the parser buffer and the read chunk size; it must be at least the size of the largest single Topic packet to be received.

## 2. RamFS 命令 / RamFS Command

模块向 RamFS 添加命令 `shared_topic:<uart_name>`（默认 `shared_topic:usart1`）：

- `shared_topic:usart1`：打印用法。
- `shared_topic:usart1 monitor <time_ms> <interval_ms>`：每 `interval_ms` 打印一次接收速率（Mbps），持续 `time_ms`。

The module adds the command `shared_topic:<uart_name>` (default `shared_topic:usart1`) to RamFS:

- `shared_topic:usart1`: print the usage.
- `shared_topic:usart1 monitor <time_ms> <interval_ms>`: print the receive rate (Mbps) every `interval_ms` for `time_ms`.

## 3. 构造接口 / Constructor

```cpp
SharedTopic(LibXR::UART& uart,
            LibXR::RamFS& ramfs,
            const char* uart_name = "usart1",
            uint32_t buffer_size = 256,
            std::initializer_list<TopicConfig> topic_configs = {"topic1", {"topic2", "libxr_def_domain"}});
```

依赖：

- `uart`：接收数据包的 `LibXR::UART`。
- `ramfs`：接收 `shared_topic:<uart_name>` 命令的 `LibXR::RamFS`。

配置参数：

- `uart_name`：RamFS 命令名的后缀，默认 `"usart1"`。
- `buffer_size`：解析缓冲区与读取块的字节数，默认 256。
- `topic_configs`：需要注册并分发的 Topic 列表。每项是 Topic 名（domain 为 `libxr_def_domain`），或 `{topic, domain}`。默认值 `topic1` 与 `topic2` 是占位名称，按实际 Topic 填写。

Dependencies:

- `uart`: the `LibXR::UART` the packets arrive on.
- `ramfs`: the `LibXR::RamFS` that receives the `shared_topic:<uart_name>` command.

Configuration parameters:

- `uart_name`: suffix of the RamFS command name, default `"usart1"`.
- `buffer_size`: size in bytes of the parser buffer and the read chunk, default 256.
- `topic_configs`: list of Topics to register and dispatch. Each item is a Topic name (domain `libxr_def_domain`) or `{topic, domain}`. The defaults `topic1` and `topic2` are placeholder names to be replaced with the actual Topics.

## 4. Topic

| Topic（默认名称） | 方向 | 类型 | 说明 |
| --- | --- | --- | --- |
| `topic_configs` 中的每个 Topic（默认 `topic1`、`topic2`） | 发布 | 该 Topic 注册时的类型 | 收到数据包后以数据包中的时间戳发布到本地 |

| Topic (default name) | Direction | Type | Meaning |
| --- | --- | --- | --- |
| Each Topic of `topic_configs` (default `topic1`, `topic2`) | Publish | the type the Topic was created with | Published locally with the timestamp from the packet when a packet arrives |

## 5. 配置示例 / Configuration Example

`xrobot instance add xrobot-org/SharedTopic` 写入的实例，`uart` 与 `ramfs` 填写为 BSP 通过 `XR_REGISTER`（硬件注册）注册的名称，`topic_configs` 填写为需要接收的 Topic：

An instance written by `xrobot instance add xrobot-org/SharedTopic`, with `uart` and `ramfs` set to names registered by the BSP's `XR_REGISTER` (Registration) and `topic_configs` set to the Topics to receive:

```yaml
modules:
  - module: xrobot-org/SharedTopic
    id: shared_topic
    args:
      - uart: usb_otg_hs_cdc
      - ramfs: ramfs
      - uart_name: "usb_otg_hs_cdc"
      - buffer_size: 256
      - topic_configs: '{"target_euler", "chassis_data", "fire_notify"}'
```

## 6. 依赖与硬件 / Dependencies and Hardware

依赖：LibXR。

硬件：一个具有可读 read port 的 UART，与发送端相连。

Dependencies: LibXR.

Hardware: one UART with a readable read port, connected to the sender.
