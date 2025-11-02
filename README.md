# EGWM 设备仿真器

这是一个用于模拟EGWM设备通过串口发送MVB协议数据的仿真系统。

## 功能特性

### 核心功能
- ✅ 通过串口发送 UART-PPP 帧格式的 MVB 数据
- ✅ 支持三种端口类型的周期性数据发送：
  - PD 0xFF：时间数据（512ms 周期）
  - PD 0xF1~0xF4：车号数据（1024ms 周期）
  - PD 0xA0：车辆状态数据（512ms 周期）
- ✅ 从 CSV 文件读取列车运行轨迹数据
- ✅ 支持两种运行模式：区间模式和线路模式
- ✅ 支持两种数据处理方式：维持和插值
- ✅ 可选的帧截断功能

### 数据处理
- **插值模式**：对速度、位置进行线性插值，提供平滑的数据变化
- **维持模式**：使用最近的数据点，数据呈阶梯状变化
- **限速查询**：根据位置自动查询限速值
- **力分解**：自动将CSV中的力分解为牵引力和制动力（电制动+空气制动）
- **停站模拟**：每个区间完成后随机停站30-60秒

## 项目结构

```
mvb_send_demo/
├── main.cpp                    // 主程序入口
├── mvb_send_demo.pro           // Qt项目文件
├── README.md                   // 本文件
├── dataDef.h                   // MVB数据结构定义
├── mvb_send.h/cpp              // MVB数据封装
├── simulator_config.h/cpp      // 配置管理
├── csv_reader.h/cpp            // CSV读取器
├── data_interpolator.h/cpp     // 数据插值器
├── speed_limiter.h/cpp         // 限速查询器
├── data_generator.h/cpp        // 数据生成器
├── serial_sender.h/cpp         // 串口发送器
├── simulation_controller.h/cpp // 仿真控制器
└── bin/                        // 编译输出目录
    ├── config.json             // 区间模式配置文件
    ├── config_line_mode.json   // 线路模式配置文件
    ├── README.txt              // 运行说明
    ├── mvb_send_demo.exe       // 编译后的可执行文件
    └── data/                   // CSV数据文件
        ├── FZ601/              // FZ601线路数据（13个区间）
        └── FZ602/              // FZ602线路数据（13个区间）
```

## 编译和运行

### 前置要求
- Qt 5.12 或更高版本
- C++11 或更高标准
- Qt SerialPort 模块

### 编译
```bash
# 使用 qmake
qmake mvb_send_demo.pro
make

# 或使用 Qt Creator
# 直接打开 mvb_send_demo.pro 文件并编译
```

### 运行

编译完成后，可执行文件和配置文件都在 `bin/` 目录下：

#### 区间模式（运行单个区间）
```bash
cd bin
./mvb_send_demo
# 或指定配置文件
./mvb_send_demo config.json
```

#### 线路模式（运行整条线路）
```bash
cd bin
./mvb_send_demo config_line_mode.json
```

## 配置文件

配置文件位于 `bin/` 目录下。

### 区间模式配置示例 (bin/config.json)

```json
{
    "railway": {
        "line": "FZ602",       // 线路名称：FZ601 或 FZ602
        "lineID": 6            // 线路ID
    },
    "train": {
        "trainNum": 1,         // 列车号
        "trainInfo": 6011,     // 车辆号
        "trainLoad": 216.0     // 列车载荷（吨）
    },
    "serial": {
        "portName": "COM1",    // 串口号
        "baudRate": 115200     // 波特率
    },
    "simulation": {
        "runMode": "SECTION",  // 运行模式：SECTION（区间）或 LINE（线路）
        "dataProcessMode": "INTERPOLATION",  // 数据处理：INTERPOLATION（插值）或 MAINTAIN（维持）
        "csvFile": "data/FZ602/interactive_test_FZ602-1-2_dv1e-22025-10-26_18-00-30/OptReslog.2025-10-26_18-00-30-FZ602-1-2-85.csv",
        "stopTimeMin": 30,     // 停站时间最小值（秒）
        "stopTimeMax": 60,     // 停站时间最大值（秒）
        "enableFrameSplit": false  // 是否启用帧截断功能
    }
}
```

### 线路模式配置示例 (bin/config_line_mode.json)

```json
{
    "railway": {
        "line": "FZ602",
        "lineID": 6
    },
    "train": {
        "trainNum": 1,
        "trainInfo": 6011,
        "trainLoad": 216.0
    },
    "serial": {
        "portName": "COM1",
        "baudRate": 115200
    },
    "simulation": {
        "runMode": "LINE",     // 线路模式
        "dataProcessMode": "INTERPOLATION",
        "csvFile": "",         // 线路模式下此字段不使用
        "stopTimeMin": 30,
        "stopTimeMax": 60,
        "enableFrameSplit": false
    }
}
```

## 数据说明

### CSV 文件格式

CSV文件包含列车运行轨迹数据，格式如下：

```
线路：FZ602,优化区间：1->2,设定时间：85,实际时间：84.998,...
相对时间,位置,速度,力,操纵工况
0.000,261.000,0.000,0.000,0
1.503,262.000,4.789,170.000,0
2.123,263.000,6.822,170.000,0
...
```

- **相对时间**：从区间开始的相对时间（秒）
- **位置**：列车位置（米）
- **速度**：列车速度（km/h）
- **力**：牵引力（正值）或制动力（负值）（kN）
- **操纵工况**：0=牵引，1=恒速，2=惰行，3=制动

### 限速配置

系统内置了 FZ601 和 FZ602 的限速配置：

#### FZ602 限速
- 0m ~ 8199m: 95 km/h
- 8199m ~ 12458m: 77 km/h
- 12458m ~ 27138m: 95 km/h
- ... （详见 speed_limiter.cpp）

#### FZ601 限速
- 30878m ~ 29864m: 95 km/h
- 29864m ~ 29745m: 45 km/h
- ... （详见 speed_limiter.cpp）

## MVB 协议数据

### PD 0xFF - 时间数据
- 周期：512ms
- 大小：8字节
- 内容：年、月、日、时、分、秒、时间标志位

### PD 0xF1~0xF4 - 车号数据
- 周期：1024ms
- 大小：8字节
- 内容：列车号、车辆号、设置标志位

### PD 0xA0 - 车辆状态数据
- 周期：512ms
- 大小：32字节
- 内容：
  - 生命信号（每512ms递增）
  - 线路ID、站点ID
  - 目标距离、起始距离
  - 列车载荷、限速值
  - 网侧电压、电流
  - 列车速度
  - 牵引力、电制动力、空气制动力
  - 各种状态标志位


## 常见问题

### 1. 串口打不开
- 检查串口号是否正确
- 检查串口是否被其他程序占用
- Windows下确认COM口存在（设备管理器）

### 2. 找不到CSV文件
- 确认 `bin/data/` 目录存在
- 确认 `bin/data/FZ601/` 或 `bin/data/FZ602/` 目录存在
- 检查配置文件中的 `csvFile` 路径是否正确（相对于 bin 目录）

### 3. 数据不更新
- 检查定时器是否正常启动
- 检查串口发送是否成功
- 查看控制台输出的错误信息

## 调试建议

1. 使用串口调试助手监控发送的数据
2. 查看控制台输出的详细日志
3. 使用区间模式先测试单个区间
4. 检查CSV数据是否完整

## 技术支持

如有问题，请查看：
- 控制台输出的错误信息和日志
- GitHub Issues

## 许可证

[根据项目需要填写]

