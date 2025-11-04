QT -= gui
QT += core serialport

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 输出目录配置
DESTDIR = $$PWD/bin              # 可执行文件输出到 bin 目录
OBJECTS_DIR = $$PWD/bin/obj      # 对象文件
MOC_DIR = $$PWD/bin/moc          # MOC 文件
RCC_DIR = $$PWD/bin/rcc          # 资源文件
UI_DIR = $$PWD/bin/ui            # UI 文件

HEADERS += \
        dataDef.h \
        mvb_send.h \
        simulator_config.h \
        csv_reader.h \
        data_interpolator.h \
        speed_limiter.h \
        data_generator.h \
        serial_sender.h \
        simulation_controller.h \
        logger.h \
        runinfo_logger.h

SOURCES += \
        main.cpp \
        mvb_send.cpp \
        simulator_config.cpp \
        csv_reader.cpp \
        data_interpolator.cpp \
        speed_limiter.cpp \
        data_generator.cpp \
        serial_sender.cpp \
        simulation_controller.cpp \
        logger.cpp \
        runinfo_logger.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
