# SharedTopic

SharedTopic 是一个基于 UART 的多 Topic 数据共享与解析服务端模块。  

SharedTopic is a UART-based multi-topic data sharing and server module.  

---

## 硬件需求 / Required Hardware

- uart_name (如 usart1)
- ramfs

## 构造参数 / Constructor Arguments

- uart_name: 串口设备名 / UART device name (e.g., "usart1")
- buffer_size: 接收缓冲区字节数 / RX buffer size (e.g., 256)
- topic_configs: 需要注册并分发的 Topic 配置列表。每项可以只写 topic 名，也可以写
  `[topic, domain]`。/ Topic configs to register and dispatch. Each item may be a
  topic name or `[topic, domain]`.

## 运行方式

`SharedTopic` 创建一个接收线程。线程用 UART `read_port` 的阻塞读等待可读事件，
被唤醒后一次性搬走当前已经到达的字节，再逐字节调用 `Topic::Server::ParseData()`。
解析出的 Topic 走普通 `Topic::Publish()` 语义，串口包里的 envelope timestamp 会被保留。

业务 Topic 不在 UART/USB 回调链里发布，避免接收回调里继续触发同步命令等业务逻辑。
逐字节喂 parser 是为了避开 parser 内部固定队列的整批写入边界：即使上一次只收到半包，
下一批数据也不会因为队列剩余空间不足而整批丢失。
`buffer_size` 必须不小于需要接收的最大单个 Topic 串口包。

## 依赖 / Depends

- 无（No dependencies）
