#include "ticketrebook.h"
//#include "ui_mainwindow.h"
#include "ui_ticketrebook.h"
#include "user1.h"
#include "dbhelper.h"
ticketrebook::ticketrebook(MainWindow* preWindow,QWidget *parent)
    : prepage(preWindow),QWidget(parent)
    , ui(new Ui::ticketrebook)
{
    ui->setupUi(this);
    loadAvailableFlights();
}

ticketrebook::~ticketrebook()
{
    delete ui;
}

void ticketrebook::on_back_clicked()
{
    prepage->show();
    this->hide();
}


void ticketrebook::on_pushButton_clicked() //退票按钮
{
    // 获取当前激活的tab索引
    int currentTabIndex = ui->tabWidget->currentIndex();

    QString passengerName, idCard, flightNumber, newFlightNumber;//数据库可以用来跟这几个数据比对

    // 根据当前tab获取对应的输入框内容
    if(currentTabIndex == 0) { // 退票tab
        passengerName = ui->refundNameEdit->text();
        idCard = ui->refundIdCardEdit->text();
        flightNumber = ui->refundflightBox->currentText();
    }
    else if(currentTabIndex == 1) { // 改签tab
        passengerName = ui->rebookNameEdit->text();
        idCard = ui->rebookIdCardEdit->text();
        flightNumber = ui->rebookflightBox->currentText();
        // 新增：获取目标航班号
        newFlightNumber = ui->newflightBox->currentText();
    }

    // 验证输入是否为空
    if(passengerName.isEmpty() || idCard.isEmpty() || flightNumber.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请填写完整的乘车人信息！");
        return;
    }

    // 验证身份证号格式（简单验证)
    //这个地方连上数据库可以实现航班号与姓名是否匹配的验证
    if(idCard.length() != 18) {
        QMessageBox::warning(this, "输入错误", "请输入正确的18位身份证号码！");
        return;
    }

    // 根据当前tab执行不同的操作
    if(currentTabIndex == 0) {
        // 执行退票逻辑
        processRefund(passengerName, idCard, flightNumber);
    }
    else if(currentTabIndex == 1) {
        // 执行改签逻辑
        processRebook(passengerName, idCard, flightNumber, newFlightNumber);
    }
}



// 重载processRebook函数，增加目标航班参数
void ticketrebook::processRebook(const QString &name, const QString &idCard,
                                 const QString &oldFlight, const QString &newFlight)
{
    qDebug() << "执行改签操作:";
    qDebug() << "乘车人:" << name;
    qDebug() << "身份证:" << idCard;
    qDebug() << "原航班号:" << oldFlight;
    qDebug() << "目标航班号:" << newFlight;

    // 发送改签申请到服务器（需要新增网络请求逻辑）
    QJsonObject request;
    request["apply_type"] = "rebook";
    request["passenger_name"] = name;
    request["id_card"] = idCard;
    request["old_flight_num"] = oldFlight;
    request["new_flight_num"] = newFlight;
    request["apply_time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    request["status"] = "pending";

    // 调用DBHelper的submitModifyRequest方法
    DBHelper::getInstance()->submitModifyRequest(request,
                                                 [this](bool success, const QString& msg) {
                                                     if(success) {
                                                         QMessageBox::information(this, "申请提交成功", "改签申请已提交，请等待审核！");
                                                     } else {
                                                         QMessageBox::warning(this, "失败", msg);
                                                     }
                                                 });
}

// 退票逻辑同样需要提交申请（原直接成功改为提交申请）
void ticketrebook::processRefund(const QString &name, const QString &idCard, const QString &flight)
{
    QJsonObject request;
    request["apply_type"] = "refund";
    request["passenger_name"] = name;
    request["id_card"] = idCard;
    request["old_flight_num"] = flight;
    request["new_flight_num"] = "";
    request["apply_time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    request["status"] = "pending";

    DBHelper::getInstance()->submitModifyRequest(request,
                                                 [this](bool success, const QString& msg) {
                                                     if(success) {
                                                         QMessageBox::information(this, "申请提交成功", "退票申请已提交，请等待审核！");
                                                     } else {
                                                         QMessageBox::warning(this, "提交失败", msg);
                                                     }
                                                 });
}

void ticketrebook::on_comfirmbutton_clicked()
{
    // 获取当前激活的tab索引
    int currentTabIndex = ui->tabWidget->currentIndex();

    QString passengerName, idCard, flightNumber, newFlightNumber;//数据库可以用来跟这几个数据比对

    // 根据当前tab获取对应的输入框内容
    if(currentTabIndex == 0) { // 退票tab
        passengerName = ui->refundNameEdit->text();
        idCard = ui->refundIdCardEdit->text();
        flightNumber = ui->refundflightBox->currentText();
    }
    else if(currentTabIndex == 1) { // 改签tab
        passengerName = ui->rebookNameEdit->text();
        idCard = ui->rebookIdCardEdit->text();
        flightNumber = ui->rebookflightBox->currentText();
        // 新增：获取目标航班号
        newFlightNumber = ui->newflightBox->currentText();
    }

    // 验证输入是否为空
    if(passengerName.isEmpty() || idCard.isEmpty() || flightNumber.isEmpty() || newFlightNumber.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请填写完整的乘车人信息！");
        return;
    }

    // 验证身份证号格式（简单验证)
    //这个地方连上数据库可以实现航班号与姓名是否匹配的验证
    if(idCard.length() != 18) {
        QMessageBox::warning(this, "输入错误", "请输入正确的18位身份证号码！");
        return;
    }

    // 根据当前tab执行不同的操作
    if(currentTabIndex == 0) {
        // 执行退票逻辑
        processRefund(passengerName, idCard, flightNumber);
    }
    else if(currentTabIndex == 1) {
        // 执行改签逻辑
        processRebook(passengerName, idCard, flightNumber, newFlightNumber);
    }
}


void ticketrebook::on_pushButton_2_clicked()
{
    QString phone=ui->phoneEdit->text();
    // 使用DBHelper获取用户订单信息
    DBHelper::getInstance()->getUserOrdersByPhone(phone,
                                                  [this](bool success, const QJsonArray& orders) {
                                                      // 处理响应数据
                                                      if (success) {
                                                          if (!orders.isEmpty()) {
                                                              // 获取最新的订单信息
                                                              QJsonObject orderObj = orders.first().toObject();

                                                              // 从订单中提取需要的信息
                                                              QString passengerName = orderObj["passenger_name"].toString();
                                                              QString flightNum = orderObj["flight_num"].toString();
                                                              QString createTime = orderObj["create_time"].toString();
                                                              QString seatNum = orderObj["seat_num"].toString();
                                                              QString passengerIdCard = orderObj["passenger_idcard"].toString();
                                                              QString userId = orderObj["user_id"].toString();
                                                              ui->rebookIdCardEdit->setText(passengerIdCard);
                                                              ui->rebookflightBox->addItem(flightNum);
                                                              ui->rebookNameEdit->setText(passengerName);
                                                              ui->refundNameEdit->setText(passengerName);
                                                              ui->refundflightBox->addItem(flightNum);
                                                              ui->refundIdCardEdit->setText(passengerIdCard);
                                                          }
                                                      }
                                                      else
                                                          {
                                                          QMessageBox::warning(this,"错误提示","没有查询到相关航班信息！");
                                                      }
                                                  });
}

void ticketrebook::loadAvailableFlights()
{
    ui->newflightBox->clear();
    //ui->newflightBox->addItem("请选择航班", ""); // 默认提示项

    // 使用HTTP请求获取航班列表
    DBHelper::getInstance()->queryFlights([this](bool success, const QJsonArray& flights) {
        if (success) {
            for (const QJsonValue& value : flights) {
                QJsonObject flight = value.toObject();
                QString flightNum = flight["flight_num"].toString();
                ui->newflightBox->addItem(flightNum, flightNum);
            }
        }
    });
}

// void ticketrebook::processRefund(const QString &name, const QString &idCard, const QString &flight)
// {
//     // 这里实现具体的退票逻辑
//     qDebug() << "执行退票操作:";
//     qDebug() << "乘车人:" << name;
//     qDebug() << "身份证:" << idCard;
//     qDebug() << "航班号:" << flight;

//     // 显示成功消息
//     QMessageBox::information(this, "退票成功",
//                              QString("退票申请已提交！\n乘车人：%1\n航班号：%2").arg(name).arg(flight));
// }

// void ticketrebook::processRebook(const QString &name, const QString &idCard, const QString &flight)
// {
//     // 这里实现具体的改签逻辑
//     qDebug() << "执行改签操作:";
//     qDebug() << "乘车人:" << name;
//     qDebug() << "身份证:" << idCard;
//     qDebug() << "航班号:" << flight;

//     // 显示成功消息
//     QMessageBox::information(this, "改签成功",
//                              QString("改签申请已提交！\n乘车人：%1\n航班号：%2").arg(name).arg(flight));
// }

