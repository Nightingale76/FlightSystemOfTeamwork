#ifndef USER3_H
#define USER3_H

#include <QWidget>
#include "ui_user3.h"
#include "user2.h"

namespace Ui {
class user3;
}

class user3 : public QWidget
{
    Q_OBJECT

public:
    explicit user3(QWidget *parent = nullptr);
    int p;
    user3(const QString &flightnum,const QString &departcity,const QString &arrivalcity,const QString &departtime,const QString &arrivaltime,const QString &price,const QString date,QWidget *parent = nullptr):QWidget(parent),ui(new Ui::user3)
    {
        ui->setupUi(this);
        ui->flightnum->setText(flightnum);
        ui->departcity->setText(departcity);
        ui->arrivalcity->setText(arrivalcity);
        ui->departtime->setText(departtime);
        ui->arrivaltime->setText(arrivaltime);
        ui->price->setText(price);
        ui->date->setText(date);
        QString tempprice=price;
        tempprice.remove("¥");
        p=tempprice.toInt();
    }
    ~user3();
    void handleOrderCreated(const QString& orderId, const QString& phone,
                            const QString& name, const QString& idCard, int amount);

private slots:
    void on_pushButton_clicked();
    void continueCreateOrder(const QString &name,
                             const QString &idCard,
                             const QString &phone);
    void on_pushButton_2_clicked();

private:
    QString m_lastOrderId;
    Ui::user3 *ui;
};

#endif // USER3_H
