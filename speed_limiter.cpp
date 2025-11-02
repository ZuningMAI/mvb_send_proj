#include "speed_limiter.h"
#include <QDebug>
#include <cmath>

SpeedLimiter::SpeedLimiter()
{
}

SpeedLimiter::SpeedLimiter(const QVector<SpeedLimitEntry> &limits)
    : m_speedLimits(limits)
{
}

int SpeedLimiter::getSpeedLimit(double position) const
{
    if (m_speedLimits.isEmpty()) {
        qWarning() << "限速配置为空";
        return 80;  // 默认限速
    }
    
    // 遍历限速配置，找到包含当前位置的区间
    for (const SpeedLimitEntry &entry : m_speedLimits) {
        // 处理正向区间 (start < end)
        if (entry.start < entry.end) {
            if (position >= entry.start && position <= entry.end) {
                return entry.limit;
            }
        }
        // 处理反向区间 (start > end)，如FZ601
        else if (entry.start > entry.end) {
            if (position <= entry.start && position >= entry.end) {
                return entry.limit;
            }
        }
    }
    
    // 如果没有找到，返回最后一个限速值或默认值
    if (!m_speedLimits.isEmpty()) {
        return m_speedLimits.last().limit;
    }
    
    qWarning() << "未找到位置" << position << "的限速配置";
    return 80;  // 默认限速
}

QVector<SpeedLimitEntry> SpeedLimiter::loadFZ602Config()
{
    // FZ602限速配置
    QVector<SpeedLimitEntry> limits;
    limits.append(SpeedLimitEntry(0, 8199, 95, 0));
    limits.append(SpeedLimitEntry(8199, 12458, 77, 0));
    limits.append(SpeedLimitEntry(12458, 27138, 95, 0));
    limits.append(SpeedLimitEntry(27138, 27187, 45, 0));
    limits.append(SpeedLimitEntry(27187, 28579, 70, 0));
    limits.append(SpeedLimitEntry(28579, 29713, 95, 0));
    limits.append(SpeedLimitEntry(29713, 29940, 47, 0));
    limits.append(SpeedLimitEntry(29862, 31000, 68, 0));
    
    qDebug() << "加载FZ602限速配置，共" << limits.size() << "个区间";
    return limits;
}

QVector<SpeedLimitEntry> SpeedLimiter::loadFZ601Config()
{
    // FZ601限速配置（注意：FZ601是反向的，start > end）
    QVector<SpeedLimitEntry> limits;
    limits.append(SpeedLimitEntry(30878, 29864, 95, 0));
    limits.append(SpeedLimitEntry(29864, 29745, 45, 0));
    limits.append(SpeedLimitEntry(29745, 28460, 60, 0));
    limits.append(SpeedLimitEntry(28460, 27190, 95, 0));
    limits.append(SpeedLimitEntry(27190, 27070, 45, 0));
    limits.append(SpeedLimitEntry(27070, 24259, 75, 0));
    limits.append(SpeedLimitEntry(24259, 14891, 95, 0));
    limits.append(SpeedLimitEntry(14891, 10881, 75, 0));
    limits.append(SpeedLimitEntry(10881, 0, 95, 0));
    
    qDebug() << "加载FZ601限速配置，共" << limits.size() << "个区间";
    return limits;
}

void SpeedLimiter::setSpeedLimits(const QVector<SpeedLimitEntry> &limits)
{
    m_speedLimits = limits;
}

