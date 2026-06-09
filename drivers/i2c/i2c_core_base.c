//   platform_bus: device_register("i2c@10003000")
//   → 匹配到 platform_driver (i2c_controller_driver)
//     → probe() 被调用
//       → 初始化硬件（配置 GPIO、时钟、中断）
//       → 分配 i2c_adapter
//       → 设置 adapter->algo = &my_i2c_algorithm
//       → 调用 i2c_add_adapter(adapter)
//         → 扫描设备树子节点
//         → 为每个子节点创建 i2c_client
//         → client->dev.bus = &i2c_bus_type
//         → 调用 device_register(&client->dev)
//           → 匹配 i2c_bus_type 上的 i2c_driver
//           → 调用 i2c_driver->probe(client)