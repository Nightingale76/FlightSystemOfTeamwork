QT = core network sql  # 确保包含sql模块
CONFIG += c++17 console
TARGET = FlightServer
TEMPLATE = app

SOURCES += \
    main.cpp \
    flightserver.cpp \
    clientthread.cpp \
    dbmanager.cpp  # 新增

HEADERS += \
    flightserver.h \
    clientthread.h \
    dbmanager.h  # 新增

# MySQL库路径（根据实际安装位置调整）
win32 {
    LIBS += -L"C:/Program Files/MySQL/MySQL Server 8.0/lib" -llibmysql
    INCLUDEPATH += "C:/Program Files/MySQL/MySQL Server 8.0/include"
}
linux {
    LIBS += -lmysqlclient
    INCLUDEPATH += /usr/include/mysql
}
macos {
    LIBS += -L/usr/local/mysql/lib -lmysqlclient
    INCLUDEPATH += /usr/local/mysql/include
}

# 输出目录
DESTDIR = ./bin
OBJECTS_DIR = ./build/obj
MOC_DIR = ./build/moc

CONFIG += c++17

CONFIG(debug, debug|release) {
    TARGET = FlightServer_d
} else {
    TARGET = FlightServer
}
