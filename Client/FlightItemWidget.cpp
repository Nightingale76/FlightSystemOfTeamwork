#include "FlightItemWidget.h"
#include "ui_FlightItemWidget.h"
#include <QDateTime>
FlightItemWidget::FlightItemWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FlightItemWidget)
{
    ui->setupUi(this);
    setFixedSize(410,80);
}

FlightItemWidget::~FlightItemWidget()
{
    delete ui;
}
void FlightItemWidget::setFlightData(const QSqlRecord &record)
{
    QString flightNumber = record.value("flight_number").toString();
    QString airline = record.value("airline").toString();
    QString departureCity = record.value("departure_city").toString();
    QString arrivalCity = record.value("arrival_city").toString();
    QString departureTime = record.value("departure_time").toString();
    QString arrivalTime = record.value("arrival_time").toString();
    QString price = record.value("price").toString();
    QString seatsAvailable = record.value("seats_available").toString();
    m_flightNo      = flightNumber;
    m_departureTime = departureTime;
    setFlightData(flightNumber, airline, departureCity, arrivalCity,
                  departureTime, arrivalTime, price, seatsAvailable);
}
QString FlightItemWidget::formatTimeForDatabase(const QString &timeStr)
{
    if (timeStr.isEmpty()) {
        return "--:--";
    }

    // 您的数据库格式: "2025-12-02 08:00:00"
    // 方法1: 使用QDateTime解析
    QDateTime dateTime = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");
    if (dateTime.isValid()) {
        return dateTime.toString("HH:mm");  // 使用HH表示24小时制
    }

    // 方法2: 如果QDateTime解析失败，直接字符串截取
    // 格式: "2025-12-02 08:00:00" -> 截取第11-16个字符 "08:00"
    if (timeStr.length() >= 16) {
        return timeStr.mid(11, 5);  // 从第11个字符开始取5个字符
    }

    return timeStr; // 返回原始字符串
}

void FlightItemWidget::setFlightData(const QString &flightNumber, const QString &airline,
                                     const QString &departureCity, const QString &arrivalCity,
                                     const QString &departureTime, const QString &arrivalTime,
                                     const QString &price, const QString &seatsAvailable)
{
    qDebug() << "原始出发时间:" << departureTime;
    qDebug() << "原始到达时间:" << arrivalTime;

    // 格式化时间 - 针对您的数据库格式
    QString formattedDepTime = formatTimeForDatabase(departureTime);
    QString formattedArrTime = formatTimeForDatabase(arrivalTime);
    QString date=departureTime.left(10);
    qDebug() << "格式化后出发时间:" << formattedDepTime;
    qDebug() << "格式化后到达时间:" << formattedArrTime;


        ui->departure_time->setText(formattedDepTime);
        ui->arrival_time->setText(formattedArrTime);
        ui->departure_place->setText(departureCity);
        ui->arrival_place->setText(arrivalCity);
        ui->price->setText("¥" + price);
        ui->date->setText(date);
        ui->flightNumber->setText(flightNumber);
        ui->seat->setText(QString("剩余%1座").arg(seatsAvailable));
        // 根据座位数量设置颜色
        if (seatsAvailable.toInt() < 10) {
            ui->seat->setStyleSheet("color: red; font-size: 10px;");
        } else {
            ui->seat->setStyleSheet("color: green; font-size: 10px;");
        }
}

void FlightItemWidget::on_pushButton_clicked()
{
    // 发信号——把航班号 & 起飞时间带出去
    emit clicked(m_flightNo, m_departureTime);

    // 你原来的跳转代码保留
    QString flightnum   = ui->flightNumber->text();
    QString departcity  = ui->departure_place->text();
    QString arrivalcity = ui->arrival_place->text();
    QString departtime  = ui->departure_time->text();
    QString arrivaltime = ui->arrival_time->text();
    QString price       = ui->price->text();
    QString date        = ui->date->text();
    newpage = new user3(flightnum, departcity, arrivalcity, departtime, arrivaltime, price,date,nullptr);
    newpage->show();

}

