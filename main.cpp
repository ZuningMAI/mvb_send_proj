#include <QCoreApplication>
//#include "dataDef.h"
#include "mvb_send.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <qendian.h>
#include <QTimer>
#include <QDateTime>
#include <QDate>
#include <QTime>

void test()
{
        /*
串口打开成功： "COM2"
收到原始数据： "7e 21 02 00 ff 19 07 1c 0d 1e 2f 00 7d 5d 06 6a 7e" 原始数据长度: 17
PD地址为： "00ff" ----- 255
从帧长度类型: 2
数据: "19 07 1c 0d 1e 2f 00 7d"
----------- 当前时间信息 -----------
当前时间: "25-7-28 13:30:47"
时间设置标志位： false          时间有效标志： true
----------------------------------
----------------------------------
收到原始数据： "7e 21 02 00 ff 19 07 1c 0d 1e 2f 7d 5d 03 43 b0 7e" 原始数据长度: 17
PD地址为： "00ff" ----- 255
从帧长度类型: 2
数据: "19 07 1c 0d 1e 2f 7d 03"
----------- 当前时间信息 -----------
当前时间: "25-7-28 13:30:47"
时间设置标志位： true           时间有效标志： true
----------------------------------
收到原始数据： "7e 21 02 00 ff 19 7d 5d 1c 0d 1e 2f 00 03 e4 08 7e" 原始数据长度: 17
PD地址为： "00ff" ----- 255
从帧长度类型: 2
数据: "19 7d 1c 0d 1e 2f 00 03"
----------- 当前时间信息 -----------
当前时间: "25-125-28 13:30:47"
时间设置标志位： true           时间有效标志： true
----------------------------------
收到原始数据： "7e 21 02 00 ff 19 7d 5d 1c 0d 1e 2f 7d 5e 03 30 62 7e" 原始数据长度: 18
PD地址为： "00ff" ----- 255
从帧长度类型: 2
数据: "19 7d 1c 0d 1e 2f 7e 03"
----------- 当前时间信息 -----------
当前时间: "25-125-28 13:30:47"
时间设置标志位： true           时间有效标志： true

    */
    DateTimeStruct dt(0x19, 0x7D, 0x1c, 0x0d, 0x1e, 0x2f,true,true);
    TrainStruct dt1(0x0001,0x0000177b,true);
    RunInfoStruct dt2(0x0000,0x0001,0x0003,0x0002,0x0001,0x00C8,0x0064,
    0x00B4,0x01C2,0x04E2,0x61A8,0x07D0,0x05DC,0x0000,0x0000,1,1,1,1,1,1,1,1,0,1,0,1,0,0);
    
    PosInfoStruct dt3(0,1,200,100,180,true,true,true,true,true);

    RailwayInfoStruct dt4(25000,60,3,1,2,200,350,180,0,false,false,true,true,true,true);

    CarriageInfoStruct dt5(220,220,220,220,1,1,1,1,1,1,1,1);

    AirbrakePowerStruct dt6(125,125,125,125);
    // 测试structToByteArray函数
    mvb_send sender;
    
    // 测试DateTimeStruct到字节数组的转换
    QByteArray dateTimeArray = mvb_send::structToByteArray(dt);
    qDebug() << "DateTimeStruct字节数组:";
    for (int i = 0; i < dateTimeArray.size(); ++i) {
        qDebug() << "data[" << i << "] =" << "0x" + QString::number(static_cast<quint8>(dateTimeArray.at(i)), 16).toUpper().rightJustified(2, '0');
    }
    QByteArray res1 = mvb_send::generateMvbPacket(dateTimeArray, TIME_PORT_TYPE);
    qDebug() << "\nTIME_PORT_TYPE数据包:";
    for (int i = 0; i < res1.size(); ++i) {
        qDebug() << "packet[" << i << "] =" << "0x" + QString::number(static_cast<quint8>(res1.at(i)), 16).toUpper().rightJustified(2, '0');
    }
    QByteArray rest1 = mvb_send::appendCrc16(res1);
    qDebug() << "\nTIME_PORT_TYPE数据包CRC校验:";
    for (int i = 0; i < rest1.size(); ++i) {
        qDebug() << "packet[" << i << "] =" << "0x" + QString::number(static_cast<quint8>(rest1.at(i)), 16).toUpper().rightJustified(2, '0');
    }

    QByteArray r1 = mvb_send::unescapeReceivedData(rest1);
    qDebug() << "\nTIME_PORT_TYPE的UART-PPP 帧:";
    for (int i = 0; i < r1.size(); ++i) {
        qDebug() << "packet[" << i << "] =" << "0x" + QString::number(static_cast<quint8>(r1.at(i)), 16).toUpper().rightJustified(2, '0');
    }


    // 测试TrainStruct到字节数组的转换
    QByteArray trainArray = mvb_send::structToByteArray(dt5);
    qDebug() << "\n字节数组:";
    for (int i = 0; i < trainArray.size(); ++i) {
        qDebug() << "data[" << i << "] =" << "0x" + QString::number(static_cast<quint8>(trainArray.at(i)), 16).toUpper().rightJustified(2, '0');
    }
    QByteArray res2 = mvb_send::generateMvbPacket(trainArray, CARRIAGEINFO_PORT_1_TYPE);
    qDebug() << "\n数据包:";
    for (int i = 0; i < res1.size(); ++i) {
        qDebug() << "packet[" << i << "] =" << "0x" + QString::number(static_cast<quint8>(res2.at(i)), 16).toUpper().rightJustified(2, '0');
    }
    QByteArray rest2 = mvb_send::appendCrc16(res2);
    qDebug() << "\nCRC16:";
    for (int i = 0; i < rest2.size(); ++i) {
        qDebug() << "packet[" << i << "] =" << "0x" + QString::number(static_cast<quint8>(rest2.at(i)), 16).toUpper().rightJustified(2, '0');
    }
    QByteArray r2 = mvb_send::unescapeReceivedData(rest2);
    qDebug() << "\nUART-PPP 帧:";
    for (int i = 0; i < r2.size(); ++i) {
        qDebug() << "packet[" << i << "] =" << "0x" + QString::number(static_cast<quint8>(r2.at(i)), 16).toUpper().rightJustified(2, '0');
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    

    // 串口对象
    QSerialPort serial;

    // 设置串口参数
    serial.setPortName("COM1");                         // 串口号
    serial.setBaudRate(QSerialPort::Baud115200);        // 波特率
    serial.setDataBits(QSerialPort::Data8);             // 数据类型
    serial.setParity(QSerialPort::NoParity);            // 校验位
    serial.setStopBits(QSerialPort::OneStop);           // 停止位
    serial.setFlowControl(QSerialPort::NoFlowControl);  // 流控

    // 打开串口为读写模式，以便能够发送数据
    if (serial.open(QIODevice::ReadWrite)) {
        qDebug() << "串口打开成功：" << serial.portName();
    } else {
        qDebug() << "无法打开串口：" << serial.errorString();
        return -1;
    }
    
    // 创建MVB发送对象
    mvb_send sender;
    
    // 创建除DateTimeStruct外的所有数据结构实例
    TrainStruct dt1(0x0001, 0x0000177b, true);
    RunInfoStruct dt2(0x0000,0x0001,0x0003,0x0002,0x0001,0x00C8,0x0064,
        0x00B4,0x01C2,0x04E2,0x61A8,0x07D0,0x05DC,0x0000,0x0000,1,1,1,1,1,1,1,1,0,1,0,1,0,0);
    PosInfoStruct dt3(0, 1, 200, 100, 180, true, true, true, true, true);
    RailwayInfoStruct dt4(25000, 60, 3, 1, 2, 200, 350, 180, 0, false, false, true, true, true, true);
    CarriageInfoStruct dt5(220, 220, 220, 220, 1, 1, 1, 1, 1, 1, 1, 1);
    AirbrakePowerStruct dt6(125, 125, 125, 125);
    
    // 连接串口发送数据完成信号，用于调试信息
    QObject::connect(&serial, &QSerialPort::bytesWritten, [](qint64 bytes) {
        qDebug() << "成功发送" << bytes << "字节数据到串口";
    });
    
    // 创建定时器周期性发送数据
    QTimer *timer = new QTimer(&serial);
    int counter = 0;
    
    QObject::connect(timer, &QTimer::timeout, [&]() {
        QByteArray dataArray;
        PD_TYPE portType;
        QString dataName;
        
        // 轮流发送不同类型的数据
        switch(counter % 7) {
            case 0: {
                // 使用当前系统时间创建DateTimeStruct
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
                
                dataArray = mvb_send::structToByteArray(dt);
                portType = TIME_PORT_TYPE;
                dataName = "DateTimeStruct";
                break;
            }
            case 1:
                dataArray = mvb_send::structToByteArray(dt1);
                portType = TRAIN_PORT_1_TYPE;
                dataName = "TrainStruct";
                break;
            case 2:
                dataArray = mvb_send::structToByteArray(dt2);
                portType = RUNINFO_PORT_TYPE;
                dataName = "RunInfoStruct";
                break;
            case 3:
                dataArray = mvb_send::structToByteArray(dt3);
                portType = POSINFO_PORT_TYPE;
                dataName = "PosInfoStruct";
                break;
            case 4:
                dataArray = mvb_send::structToByteArray(dt4);
                portType = RAILWAYINFO_PORT_TYPE;
                dataName = "RailwayInfoStruct";
                break;
            case 5:
                dataArray = mvb_send::structToByteArray(dt5);
                portType = CARRIAGEINFO_PORT_1_TYPE;
                dataName = "CarriageInfoStruct";
                break;
            case 6:
                dataArray = mvb_send::structToByteArray(dt6);
                portType = AIRBRAKEPOWER_PORT_1_TYPE;
                dataName = "AirbrakePowerStruct";
                break;
        }
        
        // 显示正在发送的数据类型
        qDebug() << "发送" << dataName << "的USART-PPP帧数据";
        
        // 生成MVB包
        QByteArray packet = mvb_send::generateMvbPacket(dataArray, portType);
        // 添加CRC16校验
        QByteArray packetWithCrc = mvb_send::appendCrc16(packet);
        // 生成USART-PPP帧
        QByteArray usartPppFrame = mvb_send::unescapeReceivedData(packetWithCrc);
        
        // 显示将要发送的数据
        qDebug() << "发送的USART-PPP帧数据：" << usartPppFrame.toHex(' ');
        
        // 发送USART-PPP帧
        qint64 bytesWritten = serial.write(usartPppFrame);
        if (bytesWritten == -1) {
            qDebug() << "发送数据失败：" << serial.errorString();
        } else {
            qDebug() << "尝试发送" << bytesWritten << "字节数据到串口";
        }
        
        counter++;
    });
    
    // 启动定时器，每1000毫秒发送一次数据
    timer->start(1000);
    
    // 立即发送一次数据（可选）
    QTimer::singleShot(0, [&]() {
        // 使用当前系统时间发送DateTimeStruct的USART-PPP帧
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
        
        QByteArray dateTimeArray = mvb_send::structToByteArray(dt);
        QByteArray packet = mvb_send::generateMvbPacket(dateTimeArray, TIME_PORT_TYPE);
        QByteArray packetWithCrc = mvb_send::appendCrc16(packet);
        QByteArray usartPppFrame = mvb_send::unescapeReceivedData(packetWithCrc);
        
        // 显示将要发送的数据
        qDebug() << "发送DateTimeStruct的USART-PPP帧数据：" << usartPppFrame.toHex(' ');
        
        // 发送USART-PPP帧
        qint64 bytesWritten = serial.write(usartPppFrame);
        if (bytesWritten == -1) {
            qDebug() << "发送数据失败：" << serial.errorString();
        } else {
            qDebug() << "尝试发送" << bytesWritten << "字节数据到串口";
        }
    });

    return a.exec();
}


