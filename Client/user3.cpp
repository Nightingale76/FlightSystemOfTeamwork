#include "user3.h"
#include "ui_user3.h"
#include "user2.h"
#include "dbhelper.h"  // 添加这行
#include <QMessageBox>
#include "payverifydialog.h"
user3::user3(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::user3)
{
    ui->setupUi(this);
}

user3::~user3()
{
    delete ui;
}

void user3::on_pushButton_clicked()
{
    this->close();
}

void user3::on_pushButton_2_clicked()
{
    /* 1. 先把所有界面数据读出来，做基础校验 */
    QString name  = ui->nameEdit ->text().trimmed();
    QString idCard= ui->idCardEdit->text().trimmed();
    QString phone = ui->phoneEdit->text().trimmed();

    if (name.isEmpty() || idCard.isEmpty() || phone.isEmpty() ||
        idCard.length()!=18 || phone.length()!=11 || !phone.startsWith("1"))
    {
        QMessageBox::warning(this, "输入错误", "请检查姓名、身份证或手机号格式！");
        return;
    }

    /* 2. 真正去校验实名信息（异步） */
    DBHelper::getInstance()->getUserPayInfo(phone, [=](bool ok, const QJsonObject &info) mutable
                                            {
                                                if (!ok)
                                                {
                                                    QMessageBox::warning(this, "实名认证缺失", "您尚未绑定支付信息，请先实名认证！");
                                                    return;
                                                }

                                                QString realName  = info["real_name"].toString();
                                                QString realIdCard= info["id_card"  ].toString();

                                                if (realName != name || realIdCard != idCard)
                                                {
                                                    QMessageBox::warning(this, "订单信息错误",
                                                                         QString("乘机人信息与实名认证信息不一致！\n"
                                                                                 "实名姓名：%1\n"
                                                                                 "实名身份证：%2\n\n"
                                                                                 "请核对后重新输入。").arg(realName, realIdCard));
                                                    return;
                                                }

                                                /* 3. 校验通过才继续走后面的下单流程 */
                                                continueCreateOrder(name, idCard, phone);
                                            });
}

void user3::handleOrderCreated(const QString& orderId, const QString& phone,
                               const QString& name, const QString& idCard, int amount)
{
    // 1. 先显示订单创建成功消息
    QMessageBox::information(this, "订单创建成功",
                             QString("订单创建成功！\n订单号：%1\n总金额：%2元\n请立即支付完成订单").arg(orderId).arg(amount));

    // 2. 弹出支付验证对话框
    PayVerifyDialog dlg(this);

    // 3. 设置对话框标题，让用户知道在支付哪个订单
    dlg.setWindowTitle(QString("支付订单 - %1").arg(orderId));

    if (dlg.exec() != QDialog::Accepted) {
        QMessageBox::warning(this, "支付取消", "订单已创建但未支付，请稍后在订单管理中完成支付");
        return;
    }

    // 4. 构造支付请求
    QJsonObject payReq;
    payReq["order_id"] = orderId;
    payReq["user_phone"] = phone;
    payReq["real_name"] = dlg.realName();
    payReq["pay_method"] = dlg.payMethod();
    payReq["pay_password"] = dlg.payPassword();

    qDebug() << "【发送支付请求】order_id=" << orderId << "phone=" << phone;

    // 5. 发送支付请求
    DBHelper::getInstance()->sendRequest(
        "payOrder", payReq,
        [this, orderId, phone](const QJsonObject &resp){
            bool ok = resp["success"].toBool();
            QString message = resp["message"].toString();

            if (ok) {
                QMessageBox::information(this, "支付成功",
                                         QString("支付成功！\n订单号：%1\n支付状态已更新").arg(orderId));

            } else {
                QMessageBox::warning(this, "支付失败",
                                     QString("支付失败：%1\n订单号：%2\n请稍后重试").arg(message).arg(orderId));

                // 提示用户稍后在订单管理中支付
                QMessageBox::information(this, "支付提示",
                                         QString("订单已创建但支付失败\n订单号：%1\n您可以在订单管理中稍后支付").arg(orderId));
            }
        });
}
void user3::continueCreateOrder(const QString &name,
                                const QString &idCard,
                                const QString &phone)
{
    bool hasAccident      = ui->insurance1->isChecked();
    bool hasComprehensive = ui->insurance2->isChecked();
    int fee = p;
    QString info = "无保险";
    if (hasAccident)      { fee += 40; info = "航空意外险(40元)"; }
    if (hasComprehensive) { fee += 60; info = "航空综合险(60元)"; }
    if (hasAccident && hasComprehensive) info = "航空意外险+综合险(100元)";

    int ret = QMessageBox::question(this, "确认订单",
                                    QString("订单信息确认：\n\n"
                                            "乘机人姓名：%1\n"
                                            "身份证号：%2\n"
                                            "联系电话：%3\n"
                                            "保险选择：%4\n"
                                            "总费用：%5元\n\n"
                                            "请确认信息是否正确？")
                                        .arg(name, idCard, phone, info).arg(fee));
    if (ret != QMessageBox::Yes) return;

    QJsonObject ord;
    ord["user_id"]        = 1;   // 后续换成登录用户真实 id
    ord["user_phone"]     = phone;
    ord["flight_num"]     = ui->flightnum->text();
    ord["seat_num"]       = "";
    ord["passenger_name"] = name;
    ord["passenger_idcard"]= idCard;
    ord["contact_phone"]  = phone;
    ord["order_amount"]   = fee;
    ord["departure_city"] = ui->departcity->text();

    DBHelper::getInstance()->createUserOrder(
        ord,
        [this, phone, name, idCard, fee](bool ok, const QString &msg, const QString &oid)
        {
            if (!ok) { QMessageBox::warning(this, "下单失败", msg); return; }
            m_lastOrderId = oid;
            handleOrderCreated(oid, phone, name, idCard, fee);
        });
}
