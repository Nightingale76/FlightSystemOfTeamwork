#include "user1.h"
#include "ui_user1.h"
#include "user2.h"
#include "seatselection.h"
#include "flightsearch.h"
#include "ticketrebook.h"
#include "dbhelper.h"  // 添加这行
#include <QMessageBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QRadioButton>
#include <QTextEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    verifyDatabaseData();
    ui->dateEdit->setCalendarPopup(true);
}

void MainWindow::setwelcome(const QString &phone,const QString &nickname)
{
    currentname = phone;
    // 使用DBHelper获取用户订单信息
    DBHelper::getInstance()->getUserOrdersByPhone(phone,
  [this](bool success, const QJsonArray& orders) {
      // 处理响应数据
       if (success) {
          if (!orders.isEmpty()) {
           // 获取最新的订单信息
            QJsonObject orderObj = orders.first().toObject();
             // 从订单中提取需要的信息
             passengername = orderObj["passenger_name"].toString();
               flight_num = orderObj["flight_num"].toString();
              QString createTime = orderObj["create_time"].toString();
               QString seatNum = orderObj["seat_num"].toString();
               passengerIdCard = orderObj["passenger_idcard"].toString();
                  QString userId = orderObj["user_id"].toString();
                  }
                  }
                 });
    ui->welcome->setText(QString("%1,欢迎你！").arg(nickname));
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::on_advice_clicked()
{
    QDialog *complaintDialog = new QDialog(this);
    complaintDialog->setWindowTitle("投诉与建议");
    complaintDialog->setFixedSize(350, 250);

    QVBoxLayout *layout = new QVBoxLayout(complaintDialog);

    // 类型选择
    QHBoxLayout *typeLayout = new QHBoxLayout();
    QLabel *typeLabel = new QLabel("类型:");
    QRadioButton *complaintRadio = new QRadioButton("服务投诉");
    QRadioButton *suggestionRadio = new QRadioButton("建议反馈");
    complaintRadio->setChecked(true);

    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(complaintRadio);
    typeLayout->addWidget(suggestionRadio);
    typeLayout->addStretch();

    // 内容输入
    QLabel *contentLabel = new QLabel("内容:");
    QTextEdit *contentEdit = new QTextEdit();
    contentEdit->setPlaceholderText("请详细描述您的问题或建议...");

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *submitButton = new QPushButton("提交");
    QPushButton *cancelButton = new QPushButton("取消");

    buttonLayout->addStretch();
    buttonLayout->addWidget(submitButton);
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(typeLayout);
    layout->addWidget(contentLabel);
    layout->addWidget(contentEdit);
    layout->addLayout(buttonLayout);

    // 连接提交按钮
    connect(submitButton, &QPushButton::clicked, [=]() {
        QString type = complaintRadio->isChecked() ? "服务投诉" : "建议反馈";
        QString content = contentEdit->toPlainText().trimmed();

        if (content.isEmpty()) {
            QMessageBox::warning(complaintDialog, "提示", "请输入内容！");
            return;
        }

        if (content.length() < 5) {
            QMessageBox::warning(complaintDialog, "提示", "内容太短，请详细描述！");
            return;
        }

        // 提交数据到服务器
        QJsonObject data;
        data["user_phone"] = currentname;
        data["user_name"] = passengername; // 或者你可以再弹窗加个输入框让用户填名字
        data["flight_num"] = "未知航班"; // 你可以从当前订单或输入框获取
        data["type"] = type;
        data["content"] = content;

        DBHelper::getInstance()->submitComplaint(data, [this](bool success, const QString&) {
            QMessageBox::information(this, "提示", success ? "提交成功" : "提交失败");
        });

        complaintDialog->accept();
    });

    connect(cancelButton, &QPushButton::clicked, complaintDialog, &QDialog::reject);

    complaintDialog->exec();
    complaintDialog->deleteLater();
}

void MainWindow::on_searchButton_clicked()
{
    // 获取用户输入的查询条件
    QString departure = ui->departureEdit->text().trimmed();
    QString arrival = ui->arrivalEdit->text().trimmed();
    QDate date = ui->dateEdit->date();

    // 基本输入验证
    if (departure.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入出发地");
        ui->departureEdit->setFocus();
        return;
    }

    if (arrival.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入目的地");
        ui->arrivalEdit->setFocus();
        return;
    }

    if (!date.isValid() || date < QDate::currentDate()) {
        QMessageBox::warning(this, "输入错误", "请输入有效的出发日期");
        ui->dateEdit->setFocus();
        return;
    }
    // 更新状态栏显示搜索信息
    ui->statusbar->showMessage(QString("正在搜索从 %1 到 %2 的机票...").arg(departure).arg(arrival));

    // 使用HTTP请求查询航班
    DBHelper::getInstance()->queryFlightsForUser(departure, arrival, date.toString("yyyy-MM-dd"),
                                                 [this](bool success, const QJsonArray& flights) {
                                                     if (success) {
                                                         if (flights.size() > 0) {
                                                             ui->statusbar->showMessage(QString("找到 %1 个符合条件的航班").arg(flights.size()));
                                                             ui->statusLabel->setText(QString("找到 %1 个航班").arg(flights.size()));
                                                             QJsonObject first = flights.first().toObject(); // ← 大小写正确
                                                             m_currentFlightNo      = first["flight_number"].toString();
                                                             m_currentDepartureTime = first["departure_time"].toString();
                                                             myMainWindow* newPage = new myMainWindow(this);
                                                             newPage->showFlightResultsFromJson(flights);
                                                             newPage->show();
                                                             this->hide();
                                                         } else {
                                                             ui->statusbar->showMessage("未找到符合条件的航班");
                                                             ui->statusLabel->setText("未找到符合条件的航班");
                                                             QMessageBox::information(this, "查询结果", "未找到符合条件的航班，请调整搜索条件");
                                                         }
                                                     } else {
                                                         QMessageBox::critical(this, "查询错误", "航班查询失败！");
                                                         ui->statusbar->showMessage("查询失败");
                                                     }

                                                 });
}

void MainWindow::verifyDatabaseData()
{
    // 不再需要验证数据库数据
    qDebug() << "使用HTTP服务器数据";
}
void MainWindow::on_seatselection_clicked()
{
    SeatSelection *newpage = new SeatSelection(this);
    newpage->setPassengerByPhone(currentname);
    newpage->show();
    this->hide();
}

void MainWindow::on_foodselection_clicked()
{

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("餐食选择");
    dialog->setFixedSize(350, 250);
    dialog->setStyleSheet("QDialog { background-color: #f5f5f5; }"
                          "QLabel { font-weight: bold; color: #333; }"
                          "QLineEdit, QComboBox { padding: 8px; border: 1px solid #ddd; border-radius: 4px; background-color: white; }"
                          "QPushButton { padding: 8px 16px; border: none; border-radius: 4px; font-weight: bold; }"
                          "QPushButton:hover { opacity: 0.9; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *titleLabel = new QLabel("简易航空选餐系统", dialog);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; padding-bottom: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QGridLayout *formLayout = new QGridLayout();
    formLayout->setSpacing(12);

    QLabel *nameLabel = new QLabel("姓名:", dialog);
    QLineEdit *nameEdit = new QLineEdit(dialog);
    nameEdit->setPlaceholderText("请输入您的姓名");
    formLayout->addWidget(nameLabel, 0, 0);
    formLayout->addWidget(nameEdit, 0, 1);

    QLabel *flightLabel = new QLabel("航班号:", dialog);
    QLineEdit *flightEdit = new QLineEdit(dialog);
    flightEdit->setPlaceholderText("请输入航班号");
    formLayout->addWidget(flightLabel, 1, 0);
    formLayout->addWidget(flightEdit, 1, 1);
    // 使用DBHelper获取用户订单信息
    DBHelper::getInstance()->getUserOrdersByPhone(currentname,
                                                  [this](bool success, const QJsonArray& orders) {
                                                      // 处理响应数据
                                                      if (success) {
                                                          if (!orders.isEmpty()) {
                                                              // 获取最新的订单信息
                                                              QJsonObject orderObj = orders.first().toObject();
                                                              // 从订单中提取需要的信息
                                                              passengername = orderObj["passenger_name"].toString();
                                                              flight_num = orderObj["flight_num"].toString();
                                                              QString createTime = orderObj["create_time"].toString();
                                                              QString seatNum = orderObj["seat_num"].toString();
                                                              passengerIdCard = orderObj["passenger_idcard"].toString();
                                                              QString userId = orderObj["user_id"].toString();

                                                          }
                                                      }
                                                  });
    nameEdit->setText(passengername);
    flightEdit->setText(flight_num);
    QLabel *mealLabel = new QLabel("选择餐食:", dialog);
    QComboBox *mealComboBox = new QComboBox(dialog);
    mealComboBox->addItem("新鲜水果餐 - 40元", 40);
    mealComboBox->addItem("西式简餐 - 50元", 50);
    mealComboBox->addItem("饕餮海鲜餐 - 55元", 55);
    formLayout->addWidget(mealLabel, 2, 0);
    formLayout->addWidget(mealComboBox, 2, 1);

    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    QPushButton *confirmButton = new QPushButton("确认选择", dialog);
    confirmButton->setStyleSheet("background-color: #27ae60; color: white;");
    confirmButton->setFixedHeight(35);

    QPushButton *cancelButton = new QPushButton("取消", dialog);
    cancelButton->setStyleSheet("background-color: #95a5a6; color: white;");
    cancelButton->setFixedHeight(35);

    buttonLayout->addWidget(confirmButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(confirmButton, &QPushButton::clicked, dialog, [&]() {
        QString passengerName = nameEdit->text().trimmed();
        QString flightNumber = flightEdit->text().trimmed();

        if (passengerName.isEmpty()) {
            QMessageBox::warning(dialog, "输入错误", "请输入姓名");
            nameEdit->setFocus();
            return;
        }

        if (flightNumber.isEmpty()) {
            QMessageBox::warning(dialog, "输入错误", "请输入航班号");
            flightEdit->setFocus();
            return;
        }

        QString selectedMeal = mealComboBox->currentText().split(" - ").first();
        int selectedPrice = mealComboBox->currentData().toInt();

        QJsonObject data;
        data["user_phone"] = currentname;
        data["user_name"] = passengerName;
        data["flight_num"] = flightNumber;
        data["meal_name"] = selectedMeal;
        data["price"] = selectedPrice;

        DBHelper::getInstance()->submitMealOrder(data, [this](bool success, const QString&) {
            QMessageBox::information(this, "提示", success ? "订餐成功" : "订餐失败");
        });

        dialog->accept();
    });

    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);

    dialog->exec();
    dialog->deleteLater();
}


void MainWindow::on_changeticket_2_clicked()
{
    flightsearch* newpage=new flightsearch(this);
    newpage->show();
    this->hide();
}

void MainWindow::on_idcertify_clicked()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("实名认证");
    dialog->setFixedSize(420, 380);   // 高度多留一点

    // 样式表保持你原来的
    dialog->setStyleSheet(R"(
        QDialog { background-color: #f5f7fa; }
        QLabel { color: #303133; font-size: 15px; font-weight: bold; padding: 5px; }
        QLabel#title { font-size: 20px; color: #409eff; padding: 10px; }
        QLineEdit { padding: 12px 15px; border: 2px solid #dcdfe6; border-radius: 6px; font-size: 15px; background-color: white; min-height: 25px; }
        QLineEdit:focus { border-color: #409eff; }
        QComboBox { min-height: 45px; font-size: 15px; }
        QPushButton#confirm { background-color: #409eff; color: white; min-width: 90px; min-height: 32px; }
        QPushButton#cancel { background-color: #909399; color: white; min-width: 90px; min-height: 32px; }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(0);

    QLabel *titleLabel = new QLabel("实名认证", dialog);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(20);

    QGridLayout *formLayout = new QGridLayout();
    formLayout->setVerticalSpacing(15);
    formLayout->setHorizontalSpacing(15);

    // 姓名
    QLabel *nameLabel = new QLabel("姓名：", dialog);
    QLineEdit *nameEdit = new QLineEdit(dialog);
    nameEdit->setPlaceholderText("请输入真实姓名");
    nameEdit->setMinimumHeight(40);
    formLayout->addWidget(nameLabel, 0, 0);
    formLayout->addWidget(nameEdit, 0, 1);

    // 身份证
    QLabel *idCardLabel = new QLabel("身份证：", dialog);
    QLineEdit *idCardEdit = new QLineEdit(dialog);
    idCardEdit->setPlaceholderText("18位身份证号码");
    idCardEdit->setMaxLength(18);
    idCardEdit->setMinimumHeight(40);
    formLayout->addWidget(idCardLabel, 1, 0);
    formLayout->addWidget(idCardEdit, 1, 1);

    // 支付密码（6位数字）
    QLabel *payLabel = new QLabel("支付密码：", dialog);
    QLineEdit *payEdit = new QLineEdit(dialog);
    payEdit->setMaxLength(6);
    payEdit->setPlaceholderText("6位数字");
    payEdit->setEchoMode(QLineEdit::Password); // 小黑点
    payEdit->setValidator(new QIntValidator(0, 999999, dialog));
    payEdit->setMinimumHeight(40);
    formLayout->addWidget(payLabel, 2, 0);
    formLayout->addWidget(payEdit, 2, 1);

    // 支付方式
    QLabel *methodLabel = new QLabel("支付方式：", dialog);
    QComboBox *methodBox = new QComboBox(dialog);
    methodBox->addItems({"微信", "支付宝"});
    methodBox->setMinimumHeight(40);
    formLayout->addWidget(methodLabel, 3, 0);
    formLayout->addWidget(methodBox, 3, 1);

    formLayout->setColumnStretch(1, 3);
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20);
    btnLayout->setAlignment(Qt::AlignCenter);
    QPushButton *cancelBtn = new QPushButton("取消", dialog);
    cancelBtn->setObjectName("cancel");
    QPushButton *okBtn = new QPushButton("确认认证", dialog);
    okBtn->setObjectName("confirm");
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(okBtn);
    mainLayout->addLayout(btnLayout);

    // 结果变量
    struct Result {
        QString name, idCard, payPwd, method;
    } result;

    // 确认按钮
    connect(okBtn, &QPushButton::clicked, dialog, [&]() {
        result.name   = nameEdit->text().trimmed();
        result.idCard = idCardEdit->text().trimmed();
        result.payPwd = payEdit->text().trimmed();
        result.method = methodBox->currentText();

        if (result.name.isEmpty()) {
            QMessageBox::warning(dialog, "输入错误", "请输入姓名");
            nameEdit->setFocus(); return;
        }
        if (result.idCard.length() != 18) {
            QMessageBox::warning(dialog, "输入错误", "身份证必须为18位");
            idCardEdit->setFocus(); return;
        }
        if (result.payPwd.length() != 6) {
            QMessageBox::warning(dialog, "输入错误", "支付密码必须为6位数字");
            payEdit->setFocus(); return;
        }
        dialog->accept();
    });

    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);

    // 弹窗并上传
    if (dialog->exec() == QDialog::Accepted) {
        QJsonObject payInfo;
        payInfo["real_name"]    = result.name;
        payInfo["id_card"]      = result.idCard;
        payInfo["pay_password"] = result.payPwd;
        payInfo["pay_method"]   = result.method;
       payInfo["user_phone"] = DBHelper::userPhone;

        DBHelper::getInstance()->sendRequest("submitUserPayInfo", payInfo,
                                             [this](const QJsonObject &resp){
                                                 bool ok = resp["success"].toBool();
                                                 QMessageBox::information(this, "提示",
                                                                          ok ? "实名认证成功" : "提交失败：" + resp["message"].toString());
                                             });
    }

    dialog->deleteLater();
}


void MainWindow::on_changeticket_clicked()
{
    ticketrebook* newpage=new ticketrebook(this);
    newpage->show();
    this->hide();
}


void MainWindow::on_pushButton_clicked()
{
    login* newpage=new login();
    newpage->show();
    this->hide();
}


void MainWindow::on_my_order_clicked()
{
    flightsearch* newpage=new flightsearch(this);
    newpage->show();
    this->hide();
}

