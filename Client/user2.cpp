#include "user2.h"
#include "ui_user2.h"
#include "FlightItemWidget.h"
#include <QDate>
#include "user1.h"
#include <QJsonObject>  // 添加这行
#include <QJsonArray>

myMainWindow::myMainWindow(MainWindow* prevWindow, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::myMainWindow)

{
    m_prevWindow = prevWindow;
    ui->setupUi(this);
}

myMainWindow::~myMainWindow()
{
    delete ui;
}

void myMainWindow::on_pushButton_clicked()
{
    m_prevWindow->show();
    this->hide();
}

QString myMainWindow::getWeekdayString(const QDate &date)
{
    QString weekDays[] = {"星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日"};
    int dayOfWeek = date.dayOfWeek(); // 返回1-7，1=星期一，7=星期日

    // 调整索引，因为数组从0开始
    return weekDays[dayOfWeek - 1];
}

void myMainWindow::showFlightResults(QSqlQueryModel *model)
{
    // 这个方法可以保留，但建议使用新的JSON方法
    showFlightResultsFromJson(QJsonArray()); // 调用新方法
}

void myMainWindow::showFlightResultsFromJson(const QJsonArray& flights)
{
    ui->listWidget->clear();
    if (flights.isEmpty()) {
        ui->statusbar->showMessage("没有查询到航班数据");
        return;
    }

    // 顶部信息用第一个航班
    QJsonObject first = flights.first().toObject();
    QDateTime dep = QDateTime::fromString(first["departure_time"].toString(), "yyyy-MM-dd hh:mm:ss");
    ui->departure->setText(first["departure_city"].toString());
    ui->arrival->setText(first["arrival_city"].toString());
    ui->date->setText(dep.date().toString("yyyy年MM月dd日"));
    ui->weekday->setText(getWeekdayString(dep.date()));

    ui->listWidget->setSpacing(50);

    for (int i = 0; i < flights.size(); ++i) {
        QJsonObject flight = flights[i].toObject();

        FlightItemWidget* flightWidget = new FlightItemWidget();
        flightWidget->setFlightData(
            flight["flight_number"].toString(),
            flight["airline"].toString(),
            flight["departure_city"].toString(),
            flight["arrival_city"].toString(),
            flight["departure_time"].toString(),
            flight["arrival_time"].toString(),
            QString::number(flight["price"].toDouble()),
            QString::number(flight["seats_available"].toInt())
            );

        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(flightWidget->sizeHint());
        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, flightWidget);

        // 🔥 连信号：一点击就存航班号 & 起飞时间
        connect(flightWidget, &FlightItemWidget::clicked, this,
                [this](const QString &fno, const QString &dtime){
                    m_selectedFlightNo      = fno;
                    m_selectedDepartureTime = dtime;
                    qDebug() << "用户选中航班：" << fno << dtime;
                });
    }

    ui->statusbar->showMessage(QString("共找到 %1 个航班").arg(flights.size()));
}
