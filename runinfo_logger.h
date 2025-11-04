#ifndef RUNINFO_LOGGER_H
#define RUNINFO_LOGGER_H

#include "dataDef.h"
#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QElapsedTimer>

/**
 * @brief RunInfo数据CSV记录器
 * 
 * 记录每次发送的PD 0xA0状态数据到CSV文件
 */
class RunInfoLogger : public QObject
{
    Q_OBJECT

public:
    // 单条数据记录
    struct RunInfoRecord {
        double relativeTime;      // 相对时间（秒）
        quint16 targetDistance;   // 目标距离（m）
        quint16 startDistance;    // 起始距离（m）
        quint16 speed;            // 运行速度（0.01km/h）
        double position;          // 位置（m）
        quint16 tractionForce;    // 牵引力（0.1kN）
        quint16 ebrakeForce;      // 电制动力（0.1kN）
        quint16 airbrakeForce;    // 空气制动力（0.1kN）
        quint16 netVoltage;       // 网侧电压（V）
        quint16 netCurrent;       // 网侧电流（0.1A）
        quint16 limitSpeed;       // 限速（km/h）
    };

    explicit RunInfoLogger(QObject *parent = nullptr);
    ~RunInfoLogger();

    /**
     * @brief 启用/禁用记录
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief 开始新的区间记录
     * @param sectionName 区间名称，如 "FZ602-12-13"
     * @param logDir 日志目录
     */
    bool startSection(const QString &sectionName, const QString &logDir = "Log");

    /**
     * @brief 记录一条RunInfo数据
     * @param data RunInfo结构体
     * @param position 当前位置（m）
     * @param limitSpeed 限速（km/h）
     */
    void logData(const RunInfoStruct &data, double position, int limitSpeed);

    /**
     * @brief 结束当前区间，保存并清理数据
     */
    void endSection();

    /**
     * @brief 清理所有数据（不保存）
     */
    void clear();

private:
    bool saveToFile();
    QString formatCsvLine(const RunInfoRecord &record) const;

    bool m_enabled;
    QString m_currentSectionName;
    QString m_logDir;
    QString m_csvFilePath;
    QVector<RunInfoRecord> m_records;
    QElapsedTimer m_timer;
    qint64 m_firstRecordTime;  // 第一条记录的时间戳（ms）
    bool m_sectionActive;
};

#endif // RUNINFO_LOGGER_H

