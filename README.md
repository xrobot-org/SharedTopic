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

`SharedTopic` 不创建接收线程。模块在 UART `read_port` 上挂 0 字节 read 作为可读事件；
一旦 UART 收到数据，回调会读取当前已有字节并调用 `Topic::Server::ParseDataFromCallback()`。
解析出的 Topic 会继续沿 libxr callback/ISR 语义发布，串口包里的 envelope timestamp 会被保留。

## 依赖 / Depends

- 无（No dependencies）
