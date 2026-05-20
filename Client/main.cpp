#include "login.h"
#include "user1.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QDebug>
#include "dbhelper.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //MainWindow w;
   // w.show();

    // 确保在主线程中初始化 DBHelper
    DBHelper::getInstance();

    login w;
    w.show();

    return a.exec();
}
