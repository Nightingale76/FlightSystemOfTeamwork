#include "SeatSelection.h"
#include "ui_seatselection.h"
#include "user1.h"
#include "dbhelper.h"  // 添加DBHelper头文件，用于获取航班信息

#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QGridLayout>
#include <QDebug>

SeatSelection::SeatSelection(MainWindow* preWindow, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SeatSelection),
    prepage(preWindow)  // 正确初始化 prepage
{
    ui->setupUi(this);

    // 设置窗口标题和大小
    setWindowTitle("在线选座值机");
    setFixedSize(900, 600);

    // 初始化界面
    setupSeatMap();

    // 初始化选中座位为空
    selectedSeat.clear();

    // 连接按钮
    connect(ui->confirmButton, &QPushButton::clicked, this, &SeatSelection::onConfirmSelection);
    connect(ui->backButton, &QPushButton::clicked, this, &SeatSelection::onBackButtonClicked);
connect(ui->searchBtn, &QPushButton::clicked, this, &SeatSelection::onSearchFlight);
    // 初始化选中的座位显示
    updateSelectedSeatDisplay();
}

SeatSelection::~SeatSelection()
{
    delete ui;
}
void SeatSelection::setupSeatMap()
{
    // 清空旧数据
    occupiedSeats.clear();
    seatOccupancy.clear();
    // 不设置 currentFlightNo，等搜索后再赋值
}
void SeatSelection::onSearchFlight()
{QString fn = ui->flightBox->currentText().trimmed();
    if (fn.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先输入航班号！");
        return;
    }

    QString flightSnap = fn;
    currentFlightNo    = flightSnap;

    /* ====== 用通用 sendRequest 发新字段 ====== */
    QJsonObject req;
    req["flight_num"] = flightSnap;   // 航班号放这里
    // departure / arrival / date 留空，让后台只按航班号匹配
    DBHelper::getInstance()->sendRequest(
        "queryFlightsForUser",
        req,
        [this, flightSnap](const QJsonObject& resp) {
            bool ok = resp["success"].toBool();
            QJsonArray flights = resp["data"].toArray();
            qDebug() << "搜索返回：" << flights;
            if (!ok || flights.isEmpty()) {
                QMessageBox::warning(this, "提示", "未找到该航班信息！");
                return;
            }
            QJsonObject f = flights.first().toObject();

            ui->flightNumberLabel->setText("航班号：");
            ui->departureTimeLabel->setText(
                QString("起飞: %1").arg(f["departure_time"].toString()));

            QString gate = f["gate"].toString();
            if (!gate.isEmpty() && gate != "未设置")
                ui->departureTimeLabel->setText(
                    ui->departureTimeLabel->text() + "  登机口: " + gate);

            loadSeatDataFromDB(flightSnap);
        });
}
void SeatSelection::setPassengerName(const QString &username)
{
    currentUsername = username;
    ui->passengerLabel->setText(QString("乘客：%1").arg(username)); // 🔥 只放用户名
    qDebug() << "设置乘客用户名：" << username;
}


void SeatSelection::createSeatButtons()
{
    // 获取或创建布局
    QGridLayout *seatLayout = nullptr;

    if (!ui->seatGroupBox->layout()) {
        // 如果没有布局，创建新的网格布局
        seatLayout = new QGridLayout(ui->seatGroupBox);
        ui->seatGroupBox->setLayout(seatLayout);
        qDebug() << "创建新的网格布局";
    } else {
        // 如果已有布局，尝试转换
        seatLayout = qobject_cast<QGridLayout*>(ui->seatGroupBox->layout());
        if (!seatLayout) {
            qDebug() << "现有布局不是QGridLayout，删除并重新创建";
            // 删除现有布局和子控件
            QLayout* oldLayout = ui->seatGroupBox->layout();
            delete oldLayout;
            seatLayout = new QGridLayout(ui->seatGroupBox);
            ui->seatGroupBox->setLayout(seatLayout);
        }
    }

    // 清空现有布局内容
    QLayoutItem* item;
    while ((item = seatLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // 设置布局参数
    seatLayout->setContentsMargins(15, 15, 15, 15);
    seatLayout->setHorizontalSpacing(3);
    seatLayout->setVerticalSpacing(3);
    seatLayout->setAlignment(Qt::AlignCenter);

    // 创建列标签（A-F）
    QStringList columns = {"A", "B", "C", "J", "K", "L"};

    // 添加左上角空白标签
    QLabel *cornerLabel = new QLabel("", ui->seatGroupBox);
    cornerLabel->setFixedSize(30, 30);
    seatLayout->addWidget(cornerLabel, 0, 0);

    // 添加列标签
    for(int col = 0; col < columns.size(); col++) {
        QLabel *label = new QLabel(columns[col], ui->seatGroupBox);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(
            "QLabel {"
            " font-weight: bold;"
            " font-size: 12px;"
            " background-color: #f0f0f0;"
            " border: 1px solid #ccc;"
            " border-radius: 3px;"
            "}"
            );
        label->setFixedSize(35, 25);
        seatLayout->addWidget(label, 0, col + 1);

        // 在C列和J列之间添加过道标签
        if(col == 2) {
            QLabel *aisleLabel = new QLabel("← 过道 →", ui->seatGroupBox);
            aisleLabel->setAlignment(Qt::AlignCenter);
            aisleLabel->setStyleSheet(
                "QLabel {"
                " font-size: 10px;"
                " color: #666;"
                "}"
                );
            aisleLabel->setFixedSize(65, 35);
            seatLayout->addWidget(aisleLabel, 0, col + 2);
        }
    }

    // 创建座位按钮（1-15排）
    for(int row = 1; row <= 15; row++) {
        // 添加行标签
        QLabel *rowLabel = new QLabel(QString::number(row), ui->seatGroupBox);
        rowLabel->setAlignment(Qt::AlignCenter);
        rowLabel->setStyleSheet(
            "QLabel {"
            " font-weight: bold;"
            " font-size: 12px;"
            " background-color: #f0f0f0;"
            " border: 1px solid #ccc;"
            " border-radius: 3px;"
            "}"
            );
        rowLabel->setFixedSize(30, 35);
        seatLayout->addWidget(rowLabel, row, 0);

        for(int col = 0; col < columns.size(); col++) {
            QString seat = QString::number(row) + columns[col];
            QPushButton *seatButton = new QPushButton(seat, ui->seatGroupBox);
            seatButton->setFixedSize(35, 35);
            //seatButton->setCheckable(true);
            seatButton->setCursor(Qt::PointingHandCursor);

            // 设置座位状态
            if(seatOccupancy.value(seat, false)) {
                // 已占用座位
                seatButton->setStyleSheet(
                    "QPushButton {"
                    " background-color: #cccccc;"
                    " border: 1px solid #999999;"
                    " border-radius: 3px;"
                    " color: #666666;"
                    " font-size: 9px;"
                    "}"
                    );
                seatButton->setEnabled(false);
            } else {
                // 可用座位
                seatButton->setStyleSheet(
                    "QPushButton {"
                    " background-color: #e8f5e8;"
                    " border: 1px solid #4CAF50;"
                    " border-radius: 3px;"
                    " color: #2E7D32;"
                    " font-size: 9px;"
                    "}"
                    "QPushButton:hover {"
                    " background-color: #d4edda;"
                    " border: 1px solid #45a049;"
                    "}"
                    "QPushButton:checked {"
                    " background-color: #4CAF50;"
                    " color: white;"
                    "}"
                    );

                // 连接点击信号
                connect(seatButton, &QPushButton::clicked, [this, row, col, columns]() {
                    onSeatSelected(row, columns[col].toLatin1().at(0));
                });
            }
            seatLayout->addWidget(seatButton, row, col + 1);

            // 在C列和J列之间添加过道空白
            if(col == 2) {
                QLabel *aisleSpace = new QLabel(" ", ui->seatGroupBox);
                aisleSpace->setFixedSize(65, 35);
                seatLayout->addWidget(aisleSpace, row, col + 2);
            }
        }
    }
    // ----------- 临时调试：检查 1A 按钮是否真的可点 -----------
    QPushButton *test = qobject_cast<QPushButton*>(
        seatLayout->itemAtPosition(1, 1)->widget());   // 1A 在第 1 行第 1 列
    if (test) {
        qDebug() << "1A objectName=" << test->objectName()
        << "enabled=" << test->isEnabled()
        << "geometry=" << test->geometry();
        // 再打印它正上方的控件
        QWidget *above = test->parentWidget()->childAt(test->mapToParent(QPoint(5,5)));
        if (above && above != test) qDebug() << "1A 上方控件=" << above->metaObject()->className();
    }
    // 强制刷新
    ui->seatGroupBox->updateGeometry();
    ui->seatGroupBox->adjustSize();

    qDebug() << "座位图创建完成，共创建" << (15 * columns.size()) << "个座位";

}

void SeatSelection::onSeatSelected(int row, char column)
{
    QString seat = QString::number(row) + QChar(column);
    if (seatOccupancy.value(seat, false)) return;   // 已被占

    // 把上次选中的按钮恢复成绿色
    if (lastSelectedBtn) {
        lastSelectedBtn->setStyleSheet(
            "QPushButton { background-color: #e8f5e8; border:1px solid #4CAF50; "
            "color:#2E7D32; font-size:9px; }"
            "QPushButton:hover { background-color: #d4edda; }");
    }

    // 高亮本次按钮
    lastSelectedBtn = qobject_cast<QPushButton*>(sender());
    if (lastSelectedBtn) {
        lastSelectedBtn->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; "
            "border:1px solid #45a049; font-size:9px; }");
    }

    selectedSeat = seat;
    updateSelectedSeatDisplay();
}

void SeatSelection::updateSelectedSeatDisplay()
{
    if(selectedSeat.isEmpty()) {
        ui->selectedSeatLabel->setText("当前未选择座位");
        ui->selectedSeatLabel->setStyleSheet("color: #666666; font-size: 14px;");
    } else {
        ui->selectedSeatLabel->setText(QString("已选座位：<b>%1</b>").arg(selectedSeat));
        ui->selectedSeatLabel->setStyleSheet("color: #cc0000; font-size: 16px; font-weight: bold;");
    }
}

void SeatSelection::onConfirmSelection()
{
    if (selectedSeat.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一个座位！");
        return;
    }

    /* ===== 点击确认时再查订单、做判断 ===== */
    DBHelper::getInstance()->getUserOrdersByPhone(Phone,
                                                  [this](bool success, const QJsonArray& orders) {
                                                      if (!success || orders.isEmpty()) {
                                                          QMessageBox::warning(this, "选座失败", "未搜索到您的订单信息！");
                                                          return;
                                                      }

                                                      /* 1. 打印调试用（保持和你原来一样） */
                                                      qDebug() << "【选座-订单列表】共" << orders.size() << "条";
                                                      for (const QJsonValue &v : orders)
                                                          qDebug() << "  order_id=" << v.toObject()["order_id"].toString()
                                                                   << "status="   << v.toObject()["order_status"].toString()
                                                                   << "flight="   << v.toObject()["flight_num"].toString()
                                                                   << "seat="     << v.toObject()["seat_num"].toString();

                                                      /* 2. 找当前航班、已支付、未选座的订单 */
                                                      bool ok = false;
                                                      for (const QJsonValue &v : orders) {
                                                          QJsonObject o = v.toObject();
                                                          if (o["flight_num"].toString() == currentFlightNo &&
                                                              o["order_status"].toString() == "paid" &&
                                                              o["seat_num"].toString().isEmpty()) {
                                                              ok = true;
                                                              break;
                                                          }
                                                      }
                                                      if (!ok) {
                                                          QMessageBox::warning(this, "选座失败",
                                                                               "当前航班不存在“已支付且未选座”的订单，\n"
                                                                               "或您已选过座位！");
                                                          return;
                                                      }

                                                      /* 3. 条件满足，真正选座 */
                                                      int ret = QMessageBox::question(this, "确认选座",
                                                                                      QString("确认选择座位 <b>%1</b> 吗？<br>"
                                                                                              "航班：%2<br>乘客：%3")
                                                                                          .arg(selectedSeat)
                                                                                          .arg(currentFlightNo)
                                                                                          .arg(currentPassenger),
                                                                                      QMessageBox::Yes | QMessageBox::No);
                                                      if (ret != QMessageBox::Yes) return;
                                                      DBHelper::getInstance()->selectSeat(currentFlightNo, selectedSeat, Phone,
                                                                                          [this](bool ok, const QString& msg){
                                                                                              qDebug() << "【选座结果】ok=" << ok << "msg=" << msg;
                                                                                              if (ok) {
                                                                                                  QMessageBox::information(this, "成功", msg);
                                                                                                  seatOccupancy[selectedSeat] = true;
                                                                                                  occupiedSeats.append(selectedSeat);
                                                                                                  createSeatButtons();   // 重绘
                                                                                                  selectedSeat.clear();
                                                                                                  updateSelectedSeatDisplay();
                                                                                              } else {
                                                                                                  QMessageBox::critical(this, "选座失败", msg);  // 🔥 详细原因
                                                                                              }
                                                                                          });

                                                  });
}

void SeatSelection::onBackButtonClicked()
{
    if (prepage) {
        prepage->show();
    }
    this->hide();
}

// 新增方法：从数据库获取座位状态
void SeatSelection::loadSeatDataFromDB(const QString &flightNum)
{
    if (flightNum.isEmpty()) {
        qDebug() << "航班号为空，无法加载座位数据";
        return;
    }

    currentFlightNo = flightNum;

    // 调用DBHelper查询座位数据
    DBHelper::getInstance()->querySeats(flightNum, [this](bool success, const QJsonArray& seats) {
        if (success) {
            // 清空现有的占用座位信息
            occupiedSeats.clear();
            seatOccupancy.clear();

            // 解析座位数据
            for (const QJsonValue& seatValue : seats) {
                QJsonObject seatObj = seatValue.toObject();
                QString seatNum = seatObj["seat_num"].toString();
                QString seatStatus = seatObj["seat_status"].toString();
                qDebug() << "seatObj:" << seatObj;
                if (seatStatus == "occupied") {
                    occupiedSeats.append(seatNum);
                    seatOccupancy[seatNum] = true;
                }
            }

            qDebug() << "从数据库加载座位数据，占用座位数：" << occupiedSeats.size();

            // 重新创建座位图
            createSeatButtons();
        } else {
            qDebug() << "加载座位数据失败";
        }
    });
}

// 设置乘客信息（通过手机号）
void SeatSelection::setPassengerByPhone(const QString &phone)
{
    // 这里应该从数据库或缓存中通过手机号获取用户名
    // 暂时使用手机号作为显示名称
    Phone=phone;
    QString currentPhone = phone;
    QString currentUsername = phone;
    ui->passengerLabel->setText(QString("乘客: %1").arg(phone));
    DBHelper::getInstance()->getUserOrdersByPhone(phone,
                                                  [this, currentPhone, currentUsername](bool success, const QJsonArray& orders) {
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
                                                              qDebug() << "【选座-订单列表】共" << orders.size() << "条";
                                                              for (const QJsonValue &v : orders)
                                                                  qDebug() << "  order_id=" << v.toObject()["order_id"].toString()
                                                                           << "status="   << v.toObject()["order_status"].toString()
                                                                           << "flight="   << v.toObject()["flight_num"].toString()
                                                                           << "seat="     << v.toObject()["seat_num"].toString();
                                                              // currentFlightNo=flightNum;
                                                              currentPassenger=passengerName;
                                                              // 更新乘客姓名显示
                                                              if (!passengerName.isEmpty()) {
                                                                  ui->passengerLabel->setText(QString("乘客: %1").arg(passengerName));
                                                              } else if (!userId.isEmpty()) {
                                                                  ui->passengerLabel->setText(QString("乘客: %1").arg(userId));
                                                              }

                                                              // 更新航班信息显示
                                                              if (!flightNum.isEmpty()) {
                                                                  ui->flightNumberLabel->setText("航班号: ");
                                                                  ui->flightBox->addItem(flightNum);
                                                              } else {
                                                                  ui->flightNumberLabel->setText("航班: 未指定");
                                                              }

                                                              // 更新起飞时间显示
                                                              if (!createTime.isEmpty()) {
                                                                  // 提取日期部分
                                                                  QString datePart = createTime.split(" ").first();
                                                                  ui->departureTimeLabel->setText("起飞时间");
                                                              } else {
                                                                  ui->departureTimeLabel->setText("起飞时间: 未指定");
                                                              }
                                                          }
                                                      }
                                                      else
                                                          QMessageBox::warning(this,"错误提示","未搜索到您的航班信息！");
                                                  });

}
// 刷新座位图
void SeatSelection::refreshSeatMap()
{
    if (!currentFlightNo.isEmpty()) {
        loadSeatDataFromDB(currentFlightNo);
    } else {
        setupSeatMap();
    }
}


