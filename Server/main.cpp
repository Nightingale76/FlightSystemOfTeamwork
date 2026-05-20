#include <QCoreApplication>
#include <QDebug>
#include "flightserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    app.setApplicationName("航班票务管理系统服务器");
    app.setApplicationVersion("2.0");

    FlightServer server;

    // 启动服务器，监听8080端口
    if (!server.startServer(8080)) {
        qCritical() << "服务器启动失败!";
        return 1;
    }

    qInfo() << "==========================================";
    qInfo() << "航班票务管理系统服务器启动成功!";
    qInfo() << "版本: 2.0 (MySQL数据库版)";
    qInfo() << "端口: 8080";
    qInfo() << "数据源: MySQL数据库";
    qInfo() << "管理员账号: admin, 密码: 123456";
    qInfo() << "服务器已准备好接受远程连接";
    qInfo() << "按 Ctrl+C 停止服务器";
    qInfo() << "==========================================";

    return app.exec();
}
