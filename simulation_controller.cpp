#include "simulation_controller.h"
#include <QDebug>
#include <cstdlib>
#include <ctime>
#include <cmath>

SimulationController::SimulationController(const SimulatorConfig &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_serial(nullptr)
    , m_sender(nullptr)
    , m_speedLimiter(nullptr)
    , m_currentSectionIndex(0)
    , m_isRunning(false)
    , m_isPaused(false)
    , m_simulationTime(0.0)
    , m_sectionStartTime(0.0)
    , m_isStopAtStation(false)
    , m_stopStartTime(0.0)
    , m_stopDuration(0.0)
    , m_lifeSignal(0)
    , m_timer512ms(nullptr)
    , m_timer1024ms(nullptr)
    , m_lastUpdateTime(0)
{
    // 初始化随机数生成器
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(nullptr)));
        seeded = true;
    }
    
    // 创建串口对象
    m_serial = new QSerialPort(this);
    
    // 创建定时器
    m_timer512ms = new QTimer(this);
    m_timer1024ms = new QTimer(this);
    
    connect(m_timer512ms, &QTimer::timeout, this, &SimulationController::onTimer512ms);
    connect(m_timer1024ms, &QTimer::timeout, this, &SimulationController::onTimer1024ms);
}

SimulationController::~SimulationController()
{
    stop();
    
    if (m_sender) {
        delete m_sender;
        m_sender = nullptr;
    }
    
    if (m_speedLimiter) {
        delete m_speedLimiter;
        m_speedLimiter = nullptr;
    }
}

bool SimulationController::start()
{
    if (m_isRunning) {
        qWarning() << "仿真已经在运行中";
        return false;
    }
    
    qDebug() << "===== 开始EGWM设备仿真 =====";
    
    // 初始化串口
    if (!initSerial()) {
        emit error("串口初始化失败");
        return false;
    }
    
    // 加载数据
    if (!loadData()) {
        emit error("数据加载失败");
        return false;
    }
    
    // 创建发送器
    m_sender = new SerialSender(m_serial, m_config.isFrameSplitEnabled());
    
    // 初始化仿真状态
    m_currentSectionIndex = 0;
    m_simulationTime = 0.0;
    m_sectionStartTime = 0.0;
    m_isStopAtStation = false;
    m_lifeSignal = 0;
    m_isPaused = false;
    
    // 启动定时器
    m_timer512ms->start(512);
    m_timer1024ms->start(1024);
    
    // 启动计时器
    m_elapsedTimer.start();
    m_lastUpdateTime = 0;
    
    m_isRunning = true;
    
    emit statusUpdate("仿真已启动");
    qDebug() << "定时器已启动，开始发送数据";
    
    return true;
}

void SimulationController::stop()
{
    if (!m_isRunning) {
        return;
    }
    
    // 停止定时器
    if (m_timer512ms) {
        m_timer512ms->stop();
    }
    if (m_timer1024ms) {
        m_timer1024ms->stop();
    }
    
    // 关闭串口
    if (m_serial && m_serial->isOpen()) {
        m_serial->close();
        qDebug() << "串口已关闭";
    }
    
    m_isRunning = false;
    m_isPaused = false;
    
    emit statusUpdate("仿真已停止");
    qDebug() << "===== 仿真已停止 =====";
}

void SimulationController::pause()
{
    if (!m_isRunning || m_isPaused) {
        return;
    }
    
    m_timer512ms->stop();
    m_timer1024ms->stop();
    m_isPaused = true;
    
    emit statusUpdate("仿真已暂停");
    qDebug() << "仿真已暂停";
}

void SimulationController::resume()
{
    if (!m_isRunning || !m_isPaused) {
        return;
    }
    
    m_timer512ms->start(512);
    m_timer1024ms->start(1024);
    m_isPaused = false;
    
    // 重置计时器
    m_lastUpdateTime = m_elapsedTimer.elapsed();
    
    emit statusUpdate("仿真已恢复");
    qDebug() << "仿真已恢复";
}

bool SimulationController::initSerial()
{
    m_serial->setPortName(m_config.getPortName());
    m_serial->setBaudRate(m_config.getBaudRate());
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    
    if (!m_serial->open(QIODevice::ReadWrite)) {
        qWarning() << "无法打开串口" << m_config.getPortName() << ":" << m_serial->errorString();
        return false;
    }
    
    qDebug() << "串口打开成功:" << m_config.getPortName();
    return true;
}

bool SimulationController::loadData()
{
    // 加载限速配置
    if (m_config.getRailwayLine() == "FZ601") {
        m_speedLimiter = new SpeedLimiter(SpeedLimiter::loadFZ601Config());
    } else if (m_config.getRailwayLine() == "FZ602") {
        m_speedLimiter = new SpeedLimiter(SpeedLimiter::loadFZ602Config());
    } else {
        qWarning() << "未知的线路:" << m_config.getRailwayLine();
        return false;
    }
    
    // 根据模式加载数据
    if (m_config.getRunMode() == SimulatorConfig::SECTION_MODE) {
        // 区间模式：加载单个CSV文件
        SectionInfo section;
        if (!CSVReader::readCSVFile(m_config.getCsvFile(), section)) {
            qWarning() << "无法读取CSV文件:" << m_config.getCsvFile();
            return false;
        }
        m_sections.append(section);
        
        qDebug() << "区间模式：加载了1个区间";
    } else {
        // 线路模式：加载整条线路的数据
        if (!CSVReader::readLineData(m_config.getRailwayLine(), m_sections)) {
            qWarning() << "无法读取线路数据:" << m_config.getRailwayLine();
            return false;
        }
        
        qDebug() << "线路模式：加载了" << m_sections.size() << "个区间";
    }
    
    if (m_sections.isEmpty()) {
        qWarning() << "没有加载到任何区间数据";
        return false;
    }
    
    return true;
}

void SimulationController::onTimer512ms()
{
    if (m_isPaused) {
        return;
    }
    
    // 更新仿真时间
    updateCurrentState();
    
    // 生命信号递增
    m_lifeSignal++;
    
    // 发送时间数据 (PD 0xFF)
    sendTimeData();
    
    // 发送运行状态数据 (PD 0xA0)
    sendRunInfoData();
}

void SimulationController::onTimer1024ms()
{
    if (m_isPaused) {
        return;
    }
    
    // 发送车号数据 (PD 0xF1~0xF4)
    sendTrainData();
}

void SimulationController::updateCurrentState()
{
    // 计算已过时间（秒）
    qint64 currentTime = m_elapsedTimer.elapsed();
    double deltaTime = (currentTime - m_lastUpdateTime) / 1000.0;
    m_lastUpdateTime = currentTime;
    
    // 如果是第一次调用，不更新时间
    if (deltaTime > 1.0) {
        deltaTime = 0.512;  // 使用定时器周期
    }
    
    // 如果在停站
    if (m_isStopAtStation) {
        m_stopStartTime += deltaTime;
        
        // 检查是否停站结束
        if (m_stopStartTime >= m_stopDuration) {
            m_isStopAtStation = false;
            
            // 切换到下一个区间
            switchToNextSection();
        }
        
        return;
    }
    
    // 更新仿真时间
    m_simulationTime += deltaTime;
    
    // 检查当前区间是否完成
    if (m_currentSectionIndex < m_sections.size()) {
        const SectionInfo &currentSection = m_sections[m_currentSectionIndex];
        
        if (!currentSection.trajectory.isEmpty()) {
            double lastTime = currentSection.trajectory.last().relativeTime;
            
            if (m_simulationTime >= lastTime) {
                // 区间完成
                QString sectionName = QString("%1: %2 -> %3")
                    .arg(currentSection.railwayLine)
                    .arg(currentSection.startStation)
                    .arg(currentSection.endStation);
                
                qDebug() << "区间完成:" << sectionName;
                emit sectionCompleted(m_currentSectionIndex, sectionName);
                
                // 处理停站
                handleStopAtStation();
            }
        }
    }
}

void SimulationController::switchToNextSection()
{
    m_currentSectionIndex++;
    
    if (m_currentSectionIndex >= m_sections.size()) {
        // 所有区间完成
        if (m_config.getRunMode() == SimulatorConfig::LINE_MODE) {
            // 线路模式：完成了整条线路
            qDebug() << "";
            qDebug() << "========================================";
            qDebug() << "      线路所有区间已完成！";
            qDebug() << "========================================";
        } else {
            // 区间模式：只完成了一个区间
            qDebug() << "";
            qDebug() << "========================================";
            qDebug() << "      区间仿真已完成！";
            qDebug() << "========================================";
        }
        
        emit lineCompleted();
        stop();
        return;
    }
    
    // 重置区间时间
    m_simulationTime = 0.0;
    
    qDebug() << "";
    qDebug() << "切换到区间" << (m_currentSectionIndex + 1) << "/" << m_sections.size();
}

void SimulationController::handleStopAtStation()
{
    // 生成随机停站时间（30~60秒）
    int minStop = m_config.getStopTimeMin();
    int maxStop = m_config.getStopTimeMax();
    m_stopDuration = minStop + (rand() % (maxStop - minStop + 1));
    
    m_isStopAtStation = true;
    m_stopStartTime = 0.0;
    
    qDebug() << "列车在站台停车，停车时间:" << m_stopDuration << "秒";
}

TrajectoryPoint SimulationController::getCurrentTrajectoryPoint()
{
    if (m_currentSectionIndex >= m_sections.size()) {
        return TrajectoryPoint();
    }
    
    const SectionInfo &currentSection = m_sections[m_currentSectionIndex];
    
    // 如果在停站，返回最后一个点（速度为0）
    if (m_isStopAtStation) {
        if (!currentSection.trajectory.isEmpty()) {
            TrajectoryPoint stopped = currentSection.trajectory.last();
            stopped.speed = 0.0;
            stopped.force = 0.0;
            stopped.operationMode = 3;  // 制动
            return stopped;
        }
    }
    
    // 根据数据处理模式获取当前点
    if (m_config.getDataProcessMode() == SimulatorConfig::INTERPOLATION_MODE) {
        return DataInterpolator::interpolate(currentSection.trajectory, m_simulationTime);
    } else {
        return DataInterpolator::maintain(currentSection.trajectory, m_simulationTime);
    }
}

void SimulationController::sendTimeData()
{
    DateTimeStruct timeData = DataGenerator::generateTimeData();
    
    // 显示数据详情
    timeData.print();
    qDebug() << "发送 \"DateTimeStruct\" 的USART-PPP帧数据";
    
    if (!m_sender->sendDateTimeStruct(timeData)) {
        qWarning() << "发送时间数据失败";
    }
}

void SimulationController::sendTrainData()
{
    TrainStruct trainData = DataGenerator::generateTrainInfo(
        m_config.getTrainNum(),
        m_config.getTrainInfo(),
        true
    );
    
    // 显示数据详情
    trainData.print();
    qDebug() << "发送 \"TrainStruct\" 的USART-PPP帧数据";
    
    if (!m_sender->sendTrainStruct(trainData, TRAIN_PORT_1_TYPE)) {
        qWarning() << "发送车号数据失败";
    }
}

void SimulationController::sendRunInfoData()
{
    if (m_currentSectionIndex >= m_sections.size()) {
        return;
    }
    
    const SectionInfo &currentSection = m_sections[m_currentSectionIndex];
    TrajectoryPoint currentPoint = getCurrentTrajectoryPoint();
    
    // 获取限速
    int limitSpeed = m_speedLimiter->getSpeedLimit(currentPoint.position);
    
    // 获取站点ID
    int currentStationID = getCurrentStationID();
    int nextStationID = getNextStationID();
    int endStationID = getEndStationID();
    
    // 判断是否是最后一个区间
    bool isLastSection = (m_currentSectionIndex == m_sections.size() - 1);
    
    // 生成运行状态数据
    RunInfoStruct runInfo = DataGenerator::generateRunInfo(
        currentPoint,
        currentSection,
        currentStationID,
        nextStationID,
        endStationID,
        currentPoint.position,
        limitSpeed,
        m_lifeSignal,
        isLastSection,
        m_config.getTrainLoad()
    );
    
    // 显示数据详情
    runInfo.print();
    qDebug() << "发送 \"RunInfoStruct\" 的USART-PPP帧数据";
    
    if (!m_sender->sendRunInfoStruct(runInfo)) {
        qWarning() << "发送运行状态数据失败";
    }
}

int SimulationController::getCurrentStationID() const
{
    if (m_currentSectionIndex >= m_sections.size()) {
        return 1;
    }
    return m_sections[m_currentSectionIndex].startStation;
}

int SimulationController::getNextStationID() const
{
    if (m_currentSectionIndex >= m_sections.size()) {
        return 2;
    }
    return m_sections[m_currentSectionIndex].endStation;
}

int SimulationController::getEndStationID() const
{
    // FZ601和FZ602的终点站都是14
    // 在区间模式下，应该返回整条线路的终点站，而不是当前区间的终点站
    if (m_config.getRailwayLine() == "FZ601" || m_config.getRailwayLine() == "FZ602") {
        return 14;
    }
    
    // 如果是其他线路且有数据，使用最后一个区间的终点站
    if (!m_sections.isEmpty()) {
        return m_sections.last().endStation;
    }
    
    return 14;  // 默认值
}

