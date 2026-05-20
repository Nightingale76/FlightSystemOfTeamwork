#include "flightsearch.h"
#include "ui_flightsearch.h"
#include "user1.h"
#include"dbhelper.h"

//主要用于查询航班是否延误，登机口在哪里
flightsearch::flightsearch(MainWindow* preWindow,QWidget *parent)
    : prepage(preWindow),QWidget(parent)
    , ui(new Ui::flightsearch)
{
    ui->setupUi(this);
    connect(ui->queryBtn, &QPushButton::clicked, this, &flightsearch::on_queryBtn_clicked);
}

flightsearch::~flightsearch()
{
    delete ui;
}

void flightsearch::on_back_clicked()
{
    prepage->show();
    this->hide();
}
void flightsearch::on_queryBtn_clicked()
{
    QString flightNum = ui->flightComboBox->currentText();
    if (flightNum.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入航班号");
        return;
    }

    // 构造请求数据
    QJsonObject requestData;
    requestData["flight_num"] = flightNum;

    // 发送请求
    DBHelper::getInstance()->sendRequest("getFlightStatusAndGate", requestData, [this](const QJsonObject& response) {
        if (response["success"].toBool()) {
            QJsonObject data = response["data"].toObject();
            QString gate = data["gate"].toString();
            QString delayStatus = data["delay_status"].toString();

            ui->gateLabel->setText("登机口：" + (gate.isEmpty() ? "未设置" : gate));
            ui->statusLabel->setText("航班状态：" + delayStatus);

            if (delayStatus == "延误") {
                ui->statusLabel->setStyleSheet("color: red; font-weight: bold;");
            } else {
                ui->statusLabel->setStyleSheet("color: green; font-weight: bold;");
            }
        } else {
            QMessageBox::warning(this, "查询失败", response["message"].toString());
        }
    });
}

void flightsearch::on_pushButton_clicked()
{
    QString phone=ui->phoneEdit->text();
    // 使用DBHelper获取用户订单信息
    DBHelper::getInstance()->getUserOrdersByPhone(phone,
                                                  [this](bool success, const QJsonArray& orders) {
                                                      // 处理响应数据
                                                      if (success) {
    ui->flightComboBox->clear();
    ui->depart_city_edit->clear();
    ui->dateEdit->clear();
    ui->timeEdit->clear();

    if (success && !orders.isEmpty()) {
        for (const QJsonValue& orderValue : orders) {
            QJsonObject orderObj = orderValue.toObject();
            if(orderObj["order_status"]=="paid")
            ui->flightComboBox->addItem(orderObj["flight_num"].toString(),
                                       QVariant::fromValue(orderObj));
        }

        // 显示第一个
        auto displayOrder = [this](const QJsonObject& order) {
            ui->depart_city_edit->setText(order["departure_city"].toString());
            QString time = order["departure_time"].toString();
            ui->dateEdit->setText(time.left(10));
            ui->timeEdit->setText(time.right(8));
        };

        displayOrder(ui->flightComboBox->itemData(0).toJsonObject());

        // 连接信号
        connect(ui->flightComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, displayOrder](int index) {
                    if (index >= 0) {
                        QVariant data = ui->flightComboBox->itemData(index);
                        if (!data.isNull()) {
                            displayOrder(data.toJsonObject());
                        }
                    }
                });

    } else {
        ui->flightComboBox->addItem("无可用航班");
    }
                                                      }
                                                                      });

}

