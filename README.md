# SharedTopic

SharedTopic 是一个基于 UART 的多 Topic 数据共享与解析服务端模块。  

SharedTopic is a UART-based multi-topic data sharing and server module.  

---

## 硬件需求 / Required Hardware

- uart_name (如 usart1)
- ramfs

## 构造参数 / Constructor Arguments

- uart_name: 串口设备名 / UART device name (e.g., "usart1")
- task_stack_depth: 任务堆栈大小 / Task stack depth (e.g., 512)
- buffer_size: 接收缓冲区字节数 / RX buffer size (e.g., 256)
- topic_names: 需要注册并分发的 Topic 名称列表 / List of topic names to register (e.g., ["topic1", "topic2"])

## 依赖 / Depends

- 无（No dependencies）
