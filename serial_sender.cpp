#include "serial_sender.h"
#include <QDebug>
#include <cstdlib>
#include <ctime>

SerialSender::SerialSender(QSerialPort *serial, bool enableSplit)
    : m_serial(serial)
    , m_enableFrameSplit(enableSplit)
{
    // 初始化随机数生成器
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(nullptr)));
        seeded = true;
    }
}

bool SerialSender::sendData(const QByteArray &usartPppFrame)
{
    if (!m_serial || !m_serial->isOpen()) {
        qWarning() << "串口未打开";
        return false;
    }
    
    // 如果启用了帧截断且帧长度大于34字节（包含头尾0x7E）
    if (m_enableFrameSplit && usartPppFrame.size() > 34) {
        QVector<QByteArray> frames = splitFrame(usartPppFrame);
        
        bool allSuccess = true;
        for (const QByteArray &frame : frames) {
            if (!sendToSerial(frame)) {
                allSuccess = false;
            }
            // 可以在这里添加延迟，如果需要的话
            // QThread::msleep(10);
        }
        
        return allSuccess;
    } else {
        return sendToSerial(usartPppFrame);
    }
}

bool SerialSender::sendDateTimeStruct(const DateTimeStruct &data)
{
    QByteArray dataArray = mvb_send::structToByteArray(data);
    QByteArray packet = mvb_send::generateMvbPacket(dataArray, TIME_PORT_TYPE);
    QByteArray packetWithCrc = mvb_send::appendCrc16(packet);
    QByteArray usartPppFrame = mvb_send::unescapeReceivedData(packetWithCrc);
    
    return sendData(usartPppFrame);
}

bool SerialSender::sendTrainStruct(const TrainStruct &data, PD_TYPE portType)
{
    QByteArray dataArray = mvb_send::structToByteArray(data);
    QByteArray packet = mvb_send::generateMvbPacket(dataArray, portType);
    QByteArray packetWithCrc = mvb_send::appendCrc16(packet);
    QByteArray usartPppFrame = mvb_send::unescapeReceivedData(packetWithCrc);
    
    return sendData(usartPppFrame);
}

bool SerialSender::sendRunInfoStruct(const RunInfoStruct &data)
{
    QByteArray dataArray = mvb_send::structToByteArray(data);
    QByteArray packet = mvb_send::generateMvbPacket(dataArray, RUNINFO_PORT_TYPE);
    QByteArray packetWithCrc = mvb_send::appendCrc16(packet);
    QByteArray usartPppFrame = mvb_send::unescapeReceivedData(packetWithCrc);
    
    return sendData(usartPppFrame);
}

QVector<QByteArray> SerialSender::splitFrame(const QByteArray &frame)
{
    QVector<QByteArray> result;
    
    // 去掉头尾的0x7E
    if (frame.size() < 3 || frame[0] != 0x7E || frame[frame.size() - 1] != 0x7E) {
        qWarning() << "无效的UART-PPP帧格式";
        result.append(frame);
        return result;
    }
    
    QByteArray content = frame.mid(1, frame.size() - 2);
    
    // 随机选择分割点（16~25字节）
    int splitPoint = 16 + (rand() % 10);  // 16~25
    
    if (splitPoint >= content.size()) {
        // 如果分割点超过内容长度，不分割
        result.append(frame);
        return result;
    }
    
    // 分割成两部分
    QByteArray part1 = content.left(splitPoint);
    QByteArray part2 = content.mid(splitPoint);
    
    // 为每部分添加头尾0x7E
    QByteArray frame1;
    frame1.append(0x7E);
    frame1.append(part1);
    frame1.append(0x7E);
    
    QByteArray frame2;
    frame2.append(0x7E);
    frame2.append(part2);
    frame2.append(0x7E);
    
    result.append(frame1);
    result.append(frame2);
    
    qDebug() << "帧已分割: 原始" << frame.size() << "字节 -> " 
             << frame1.size() << "字节 + " << frame2.size() << "字节";
    
    return result;
}

bool SerialSender::sendToSerial(const QByteArray &data)
{
    if (!m_serial || !m_serial->isOpen()) {
        qWarning() << "串口未打开";
        return false;
    }
    
    // 显示16进制数据
    qDebug() << "发送的USART-PPP帧数据：" << "\"" + data.toHex(' ') + "\"";
    qDebug() << "尝试发送" << data.size() << "字节数据到串口";
    
    qint64 bytesWritten = m_serial->write(data);
    
    if (bytesWritten == -1) {
        qWarning() << "发送数据失败:" << m_serial->errorString();
        return false;
    }
    
    // 等待数据发送完成
    if (!m_serial->waitForBytesWritten(1000)) {
        qWarning() << "等待发送超时";
        return false;
    }
    
    qDebug() << "成功发送" << bytesWritten << "字节数据到串口";
    
    return true;
}

