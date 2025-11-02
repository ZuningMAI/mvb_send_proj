#include <QCoreApplication>
#include "simulation_controller.h"
#include "simulator_config.h"
#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QDir>

void printUsage()
{
    qDebug() << "====================================";
    qDebug() << "    EGWM 设备仿真器 v1.0";
    qDebug() << "====================================";
}

bool checkDataDirectory()
{
    QDir dataDir("data");
    if (!dataDir.exists()) {
        qWarning() << "数据目录不存在: data/";
        qWarning() << "请确保data目录及CSV文件存在";
        return false;
    }
    
    QDir fz601Dir("data/FZ601");
    QDir fz602Dir("data/FZ602");
    
    bool hasFZ601 = fz601Dir.exists();
    bool hasFZ602 = fz602Dir.exists();
    
    if (!hasFZ601 && !hasFZ602) {
        qWarning() << "未找到FZ601或FZ602数据目录";
        return false;
    }
    
    qDebug() << "数据目录检查:";
    if (hasFZ601) {
        qDebug() << "  ✓ FZ601 数据目录存在";
    }
    if (hasFZ602) {
        qDebug() << "  ✓ FZ602 数据目录存在";
    }
    
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    // 打印使用说明
    printUsage();
    
    // 检查数据目录
    if (!checkDataDirectory()) {
        qWarning() << "数据目录检查失败";
        return -1;
    }
    
    // 解析命令行参数
    QString configFile = "config_line_mode.json";
    if (argc > 1) {
        configFile = argv[1];
    }
    
    qDebug() << "";
    qDebug() << "使用配置文件:" << configFile;
    
    // 检查配置文件是否存在
    if (!QFile::exists(configFile)) {
        qWarning() << "配置文件不存在:" << configFile;
        qWarning() << "请创建配置文件或使用 -h 查看帮助";
        return -1;
    }
    
    // 加载配置
    SimulatorConfig config;
    if (!config.loadFromFile(configFile)) {
        qWarning() << "配置文件加载失败";
        return -1;
    }
    
    // 显示配置信息
    qDebug() << "";
    qDebug() << "========== 配置信息 ==========";
    qDebug() << "线路:" << config.getRailwayLine() << "(" << config.getLineID() << "号线)";
    qDebug() << "列车号:" << config.getTrainNum();
    qDebug() << "车辆号:" << config.getTrainInfo();
    qDebug() << "列车载荷:" << config.getTrainLoad() << "吨";
    qDebug() << "串口:" << config.getPortName() << "@" << config.getBaudRate();
    qDebug() << "运行模式:" << (config.getRunMode() == SimulatorConfig::SECTION_MODE ? "区间模式" : "线路模式");
    qDebug() << "数据处理:" << (config.getDataProcessMode() == SimulatorConfig::INTERPOLATION_MODE ? "插值" : "维持");
    if (config.getRunMode() == SimulatorConfig::SECTION_MODE) {
        qDebug() << "CSV文件:" << config.getCsvFile();
    }
    qDebug() << "停站时间:" << config.getStopTimeMin() << "~" << config.getStopTimeMax() << "秒";
    qDebug() << "帧截断:" << (config.isFrameSplitEnabled() ? "启用" : "禁用");
    qDebug() << "==============================";
    qDebug() << "";
    
    // 创建仿真控制器
    SimulationController controller(config);
    
    // 连接信号
    QObject::connect(&controller, &SimulationController::sectionCompleted,
                    [](int index, const QString &name) {
        qDebug() << "";
        qDebug() << "===== 区间完成 =====";
        qDebug() << "区间" << (index + 1) << ":" << name;
        qDebug() << "====================";
        qDebug() << "";
    });
    
    QObject::connect(&controller, &SimulationController::lineCompleted,
                    []() {
        // 完成信息已在 SimulationController 中根据模式显示
        // 这里不需要重复输出
    });
    
    QObject::connect(&controller, &SimulationController::error,
                    [](const QString &msg) {
        qWarning() << "错误:" << msg;
    });
    
    QObject::connect(&controller, &SimulationController::statusUpdate,
                    [](const QString &status) {
        qDebug() << "[状态]" << status;
    });
    
    // 启动仿真
    if (!controller.start()) {
        qWarning() << "仿真启动失败";
        return -1;
    }
    
    qDebug() << "";
    qDebug() << "仿真正在运行...";
    qDebug() << "按 Ctrl+C 停止仿真";
    qDebug() << "";
    
    return a.exec();
}


