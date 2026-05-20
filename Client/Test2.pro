QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    FlightItemWidget.cpp \
    adminEditPwd.cpp \
    adminwindow.cpp \
    flightsearch.cpp \
    foodselection.cpp \
    login.cpp \
    main.cpp \
    payverifydialog.cpp \
    seatselection.cpp \
    ticketrebook.cpp \
    user1.cpp \
    user2.cpp \
    user3.cpp \
    dbhelper.cpp

HEADERS += \
    FlightItemWidget.h \
    adminEditPwd.h \
    adminwindow.h \
    flightsearch.h \
    foodselection.h \
    login.h \
    payverifydialog.h \
    seatselection.h \
    ticketrebook.h \
    user1.h \
    user2.h \
    user3.h \
    dbhelper.h

FORMS += \
    FlightItemWidget.ui \
    adminEditPwd.ui \
    adminwindow.ui \
    flightsearch.ui \
    foodselection.ui \
    login.ui \
    payverifydialog.ui \
    seatselection.ui \
    ticketrebook.ui \
    user1.ui \
    user2.ui \
    user3.ui \
    loginwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    image.qrc
