#ifndef FLIGHTITEMWIDGET_H
#define FLIGHTITEMWIDGET_H

#include <QWidget>
#include "user3.h"
#include <QSqlRecord>
namespace Ui {
class FlightItemWidget;
}

class FlightItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FlightItemWidget(QWidget *parent = nullptr);
    ~FlightItemWidget();
    QString formatTimeForDatabase(const QString &timeStr);
    void setFlightData(const QSqlRecord &record);
    void setFlightData(const QString &flightNumber, const QString &airline,
                       const QString &departureCity, const QString &arrivalCity,
                       const QString &departureTime, const QString &arrivalTime,
                       const QString &price, const QString &seatsAvailable);
signals:
    void clicked(const QString &flightNo,
                 const QString &departureTime);   // 起飞时间

private slots:
    void on_pushButton_clicked();

private:
    user3* newpage;
    Ui::FlightItemWidget *ui;
    QString m_flightNo;
    QString m_departureTime;   // 记得在 setFlightData 里保存
};

#endif // FLIGHTITEMWIDGET_H
