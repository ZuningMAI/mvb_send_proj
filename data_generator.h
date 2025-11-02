#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "simulator_config.h"
#include "dataDef.h"
#include <QDateTime>

class DataGenerator {
public:
    // 生成时间数据 (PD 0xFF)
    static DateTimeStruct generateTimeData();
    
    // 生成车号数据 (PD 0xF1~0xF4)
    static TrainStruct generateTrainInfo(quint16 trainNum, quint32 trainInfo, bool trainSetFlag = true);
    
    // 生成运行状态数据 (PD 0xA0)
    static RunInfoStruct generateRunInfo(
        const TrajectoryPoint &currentPoint,
        const SectionInfo &currentSection,
        int currentStationID,
        int nextStationID,
        int endStationID,
        double currentPosition,
        int limitSpeed,
        quint16 lifeSignal,
        bool isLastSection,
        double trainLoad = 216.0  // 默认载荷216吨
    );
    
private:
    // 计算目标距离和起始距离
    static void calculateDistances(double currentPos, 
                                   double sectionStart, 
                                   double sectionEnd,
                                   quint16 &targetDist, 
                                   quint16 &startDist);
    
    // 根据力和工况计算牵引力和制动力
    static void calculateForces(double force, int mode,
                               quint16 &tractionForce,
                               quint16 &ebrakeForce,
                               quint16 &airbrakeForce);
    
    // 生成随机网侧电压和电流
    static void generateRandomPower(quint16 &voltage, quint16 &current);
};

#endif // DATA_GENERATOR_H

