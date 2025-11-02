#include "data_generator.h"
#include <QDateTime>
#include <QDebug>
#include <cmath>
#include <cstdlib>

DateTimeStruct DataGenerator::generateTimeData()
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDate date = currentDateTime.date();
    QTime time = currentDateTime.time();
    
    DateTimeStruct dt(
        static_cast<quint8>(date.year() - 2000), // 年份偏移2000年
        static_cast<quint8>(date.month()),
        static_cast<quint8>(date.day()),
        static_cast<quint8>(time.hour()),
        static_cast<quint8>(time.minute()),
        static_cast<quint8>(time.second()),
        true,  // timeset
        true   // timeAvailable
    );
    
    return dt;
}

TrainStruct DataGenerator::generateTrainInfo(quint16 trainNum, quint32 trainInfo, bool trainSetFlag)
{
    TrainStruct train(trainNum, trainInfo, trainSetFlag);
    return train;
}

RunInfoStruct DataGenerator::generateRunInfo(
    const TrajectoryPoint &currentPoint,
    const SectionInfo &currentSection,
    int currentStationID,
    int nextStationID,
    int endStationID,
    double currentPosition,
    int limitSpeed,
    quint16 lifeSignal,
    bool isLastSection,
    double trainLoad)
{
    // 计算目标距离和起始距离
    quint16 targetDistance = 0;
    quint16 startDistance = 0;
    calculateDistances(currentPosition, currentSection.startPosition, 
                      currentSection.endPosition, targetDistance, startDistance);
    
    // 计算牵引力和制动力
    quint16 tractionForce = 0;
    quint16 ebrakeForce = 0;
    quint16 airbrakeForce = 0;
    calculateForces(currentPoint.force, currentPoint.operationMode,
                   tractionForce, ebrakeForce, airbrakeForce);
    
    // 生成随机网侧电压和电流
    quint16 netVoltage = 0;
    quint16 netCurrent = 0;
    generateRandomPower(netVoltage, netCurrent);
    
    // 转换单位
    quint16 trainLoadValue = static_cast<quint16>(trainLoad * 10);  // t -> 0.1t
    quint16 speedValue = static_cast<quint16>(currentPoint.speed * 100);  // km/h -> 0.01km/h
    quint16 limitSpeedValue = static_cast<quint16>(limitSpeed);  // km/h
    
    // 设置标志位
    bool endStationIDValid = !isLastSection || (currentPoint.speed > 0.1);
    bool nextStationIDValid = !isLastSection;
    bool currentStationIDValid = true;
    bool targetDistValid = currentPoint.speed > 0.1;
    bool startDistValid = currentPoint.speed > 0.1;
    bool atoMode = true;
    bool tmc1Active = true;
    bool tmc2Active = false;
    
    // 根据操纵工况设置标志
    bool traction = (currentPoint.operationMode == 0);
    bool coast = (currentPoint.operationMode == 2);
    bool brake = (currentPoint.operationMode == 3);
    
    // 载荷类型（当前只考虑AW0）
    bool loadAW0 = true;
    bool loadAW2 = false;
    bool loadAW3 = false;
    
    RunInfoStruct runInfo(
        lifeSignal,
        6,  // 线路ID
        endStationID,
        nextStationID,
        currentStationID,
        targetDistance,
        startDistance,
        trainLoadValue,
        limitSpeedValue,
        netCurrent,
        netVoltage,
        speedValue,
        tractionForce,
        ebrakeForce,
        airbrakeForce,
        endStationIDValid,
        nextStationIDValid,
        currentStationIDValid,
        targetDistValid,
        startDistValid,
        atoMode,
        tmc1Active,
        tmc2Active,
        coast,
        traction,
        brake,
        loadAW0,
        loadAW2,
        loadAW3
    );
    
    return runInfo;
}

void DataGenerator::calculateDistances(double currentPos, 
                                      double sectionStart, 
                                      double sectionEnd,
                                      quint16 &targetDist, 
                                      quint16 &startDist)
{
    // 目标距离 = 区间终点 - 当前位置
    double target = std::abs(sectionEnd - currentPos);
    targetDist = static_cast<quint16>(std::round(target));
    
    // 起始距离 = 当前位置 - 区间起点
    double start = std::abs(currentPos - sectionStart);
    startDist = static_cast<quint16>(std::round(start));
}

void DataGenerator::calculateForces(double force, int mode,
                                   quint16 &tractionForce,
                                   quint16 &ebrakeForce,
                                   quint16 &airbrakeForce)
{
    // 初始化
    tractionForce = 0;
    ebrakeForce = 0;
    airbrakeForce = 0;
    
    if (force > 0) {
        // 牵引力
        tractionForce = static_cast<quint16>(std::round(force * 10));  // kN -> 0.1kN
    } else if (force < 0) {
        // 制动力
        double totalBrake = std::abs(force);
        
        // 随机决定是否使用空气制动
        // 50%概率只用电制动，50%概率电制动+空气制动
        if (rand() % 2 == 0) {
            // 只用电制动
            ebrakeForce = static_cast<quint16>(std::round(totalBrake * 10));  // kN -> 0.1kN
            airbrakeForce = 0;
        } else {
            // 电制动+空气制动
            // 电制动占60%~80%
            double ebrakeRatio = 0.6 + (rand() % 21) / 100.0;  // 0.6~0.8
            double ebrakePart = totalBrake * ebrakeRatio;
            double airbrakePart = totalBrake - ebrakePart;
            
            ebrakeForce = static_cast<quint16>(std::round(ebrakePart * 10));
            airbrakeForce = static_cast<quint16>(std::round(airbrakePart * 10));
        }
    }
    // else force == 0，所有力都保持为0
}

void DataGenerator::generateRandomPower(quint16 &voltage, quint16 &current)
{
    // 网侧电压：1000V ± 500V
    int voltageBase = 1000;
    int voltageVariation = (rand() % 1001) - 500;  // -50 ~ +50
    voltage = static_cast<quint16>(voltageBase + voltageVariation);
    
    // 网侧电流：100A ~ 500A（根据实际功率合理估算）
    int currentBase = 300;
    int currentVariation = (rand() % 201) - 100;  // -100 ~ +100
    int currentValue = currentBase + currentVariation;
    if (currentValue < 100) currentValue = 100;
    if (currentValue > 500) currentValue = 500;
    
    current = static_cast<quint16>(currentValue * 10);  // A -> 0.1A
}

