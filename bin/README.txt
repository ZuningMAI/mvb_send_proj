=======================================
    EGWM 设备仿真器 - 运行说明
=======================================

目录结构:
bin/
├── mvb_send_demo.exe    (可执行文件)
├── config.json          (区间模式配置)
├── config_line_mode.json (线路模式配置)
├── data/                (CSV数据目录)
│   ├── FZ601/          (FZ601线路数据)
│   └── FZ602/          (FZ602线路数据)
└── README.txt          (本文件)

运行方法:
=========

1. 区间模式
   mvb_send_demo.exe
   或
   mvb_send_demo.exe config.json

2. 线路模式
   mvb_send_demo.exe config_line_mode.json

配置说明:
=========

修改 config.json 中的设置:
- portName: 串口号 (如 COM1, COM2)
- csvFile: CSV文件路径 (相对于bin目录)
- runMode: SECTION(区间) 或 LINE(线路)
- dataProcessMode: MAINTAIN(维持) 或 INTERPOLATION(插值)

注意事项:
=========

1. 确保 data/ 目录存在且包含CSV文件
2. 串口号要与实际设备匹配
3. CSV文件路径相对于bin目录
4. 双击运行或命令行运行均可

常见问题:
=========

Q: 找不到CSV文件?
A: 检查 data/ 目录是否存在，路径是否正确

Q: 串口打不开?
A: 检查串口号，确认没有被其他程序占用

Q: 数据目录不存在?
A: 确保 bin/data/ 目录存在且包含FZ601或FZ602子目录

=======================================

