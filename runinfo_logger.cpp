#include "runinfo_logger.h"
#include "logger.h"
#include <QDir>
#include <QDateTime>
#include <QDebug>

RunInfoLogger::RunInfoLogger(QObject *parent)
    : QObject(parent)
    , m_enabled(false)
    , m_firstRecordTime(0)
    , m_sectionActive(false)
{
}

RunInfoLogger::~RunInfoLogger()
{
    if (m_sectionActive) {
        endSection();
    }
}

void RunInfoLogger::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled) {
        LOG_INFO("RunInfo数据记录器已启用");
    } else {
        LOG_INFO("RunInfo数据记录器已禁用");
    }
}

bool RunInfoLogger::startSection(const QString &sectionName, const QString &logDir)
{
    if (!m_enabled) {
        return false;
    }

    // 如果有活动的区间，先结束它
    if (m_sectionActive) {
        endSection();
    }

    m_currentSectionName = sectionName;
    m_logDir = logDir;
    m_records.clear();
    m_firstRecordTime = 0;
    
    // 创建日志目录
    QDir dir;
    if (!dir.exists(logDir)) {
        if (!dir.mkpath(logDir)) {
            qWarning() << "无法创建RunInfo日志目录:" << logDir;
            LOG_ERROR(QString("无法创建RunInfo日志目录: %1").arg(logDir));
            return false;
        }
    }

    // 生成CSV文件路径
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    m_csvFilePath = QString("%1/RunInfo_%2_%3.csv")
                        .arg(logDir)
                        .arg(sectionName)
                        .arg(timestamp);

    // 启动计时器
    m_timer.start();
    m_sectionActive = true;

    LOG_INFO(QString("开始记录RunInfo数据 - 区间: %1, 文件: %2").arg(sectionName).arg(m_csvFilePath));
    qDebug() << "RunInfo记录器已启动，区间:" << sectionName;

    return true;
}

void RunInfoLogger::logData(const RunInfoStruct &data, double position, int limitSpeed)
{
    if (!m_enabled || !m_sectionActive) {
        return;
    }

    // 获取当前时间戳
    qint64 currentTime = m_timer.elapsed();
    
    // 如果是第一条记录，设置基准时间
    if (m_firstRecordTime == 0) {
        m_firstRecordTime = currentTime;
    }

    // 计算相对时间（秒）
    double relativeTime = (currentTime - m_firstRecordTime) / 1000.0;

    // 创建记录
    RunInfoRecord record;
    record.relativeTime = relativeTime;
    record.targetDistance = data.targetDistance;
    record.startDistance = data.startDistance;
    record.speed = data.Speed;
    record.position = position;
    record.tractionForce = data.tractionForce;
    record.ebrakeForce = data.ebrakeForce;
    record.airbrakeForce = data.airbrakeForce;
    record.netVoltage = data.netVoltage;
    record.netCurrent = data.netElectric;
    record.limitSpeed = limitSpeed;

    // 添加到记录列表
    m_records.append(record);

    // 调试输出（可选，避免太多输出）
    if (m_records.size() % 10 == 0) {
        qDebug() << "RunInfo已记录" << m_records.size() << "条数据";
    }
}

void RunInfoLogger::endSection()
{
    if (!m_enabled || !m_sectionActive) {
        return;
    }

    // 保存到CSV文件
    if (saveToFile()) {
        LOG_INFO(QString("RunInfo数据已保存 - 区间: %1, 记录数: %2, 文件: %3")
                 .arg(m_currentSectionName)
                 .arg(m_records.size())
                 .arg(m_csvFilePath));
        qDebug() << "RunInfo数据已保存:" << m_csvFilePath << ", 共" << m_records.size() << "条";
    } else {
        LOG_ERROR(QString("RunInfo数据保存失败 - 区间: %1").arg(m_currentSectionName));
        qWarning() << "RunInfo数据保存失败";
    }

    // 清理数据
    clear();
}

void RunInfoLogger::clear()
{
    m_records.clear();
    m_firstRecordTime = 0;
    m_sectionActive = false;
    m_currentSectionName.clear();
    m_csvFilePath.clear();
}

bool RunInfoLogger::saveToFile()
{
    if (m_records.isEmpty()) {
        qDebug() << "没有RunInfo数据需要保存";
        return true;
    }

    QFile file(m_csvFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法创建RunInfo CSV文件:" << m_csvFilePath << ", 错误:" << file.errorString();
        return false;
    }

    // 写入UTF-8 BOM（字节顺序标记）
    // file.write("\xEF\xBB\xBF");
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream.setGenerateByteOrderMark(false);  // 我们已经手动写入BOM了

    // 写入CSV表头
    stream << "时间(s),目标距离(m),起始距离(m),运行速度(km/h),位置(m),牵引力(kN),电制动力(kN),空气制动力(kN),网侧电压(V),网侧电流(A),限速(km/h)\n";

    // 写入数据
    for (const RunInfoRecord &record : m_records) {
        stream << formatCsvLine(record) << "\n";
    }

    stream.flush();
    file.close();
    
    qDebug() << "CSV文件已保存为UTF-8编码（带BOM）:" << m_csvFilePath;
    return true;
}

QString RunInfoLogger::formatCsvLine(const RunInfoRecord &record) const
{
    // 时间：保留3位小数
    QString time = QString::number(record.relativeTime, 'f', 3);
    
    // 目标距离、起始距离：原始值（m）
    QString targetDist = QString::number(record.targetDistance);
    QString startDist = QString::number(record.startDistance);
    
    // 运行速度：原始单位是0.01km/h，转换为km/h（保留2位小数）
    QString speed = QString::number(record.speed * 0.01, 'f', 2);
    
    // 位置：保留1位小数
    QString position = QString::number(record.position, 'f', 1);
    
    // 牵引力、电制动力、空气制动力：原始单位是0.1kN，转换为kN（保留1位小数）
    QString traction = QString::number(record.tractionForce * 0.1, 'f', 1);
    QString ebrake = QString::number(record.ebrakeForce * 0.1, 'f', 1);
    QString airbrake = QString::number(record.airbrakeForce * 0.1, 'f', 1);
    
    // 网侧电压：原始值（V）
    QString voltage = QString::number(record.netVoltage);
    
    // 网侧电流：原始单位是0.1A，转换为A（保留1位小数）
    QString current = QString::number(record.netCurrent * 0.1, 'f', 1);
    
    // 限速：原始值（km/h）
    QString limitSpeed = QString::number(record.limitSpeed);
    
    return QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11")
        .arg(time)
        .arg(targetDist)
        .arg(startDist)
        .arg(speed)
        .arg(position)
        .arg(traction)
        .arg(ebrake)
        .arg(airbrake)
        .arg(voltage)
        .arg(current)
        .arg(limitSpeed);
}

