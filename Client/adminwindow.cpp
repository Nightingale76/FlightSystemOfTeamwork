#include "adminwindow.h"
#include "ui_adminwindow.h"
#include "dbhelper.h"
#include "adminEditPwd.h"
#include "login.h"

#include <QGridLayout>
#include <QMessageBox>
#include <QDebug>
#include <QDialog>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QInputDialog>

AdminWindow::AdminWindow(const QString& adminAccount, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AdminWindow)
    , m_currentAdminAccount(adminAccount) // 初始化当前管理员账号
    , m_flightSearchNum("")  // 初始搜索框为空
    , m_sortField("create_time")  // 航班默认按创建时间排序
    , m_rebookFilterType("all")  // 默认筛选全部类型
    , m_rebookFilterStatus("all") // 默认筛选全部状态
{
    ui->setupUi(this);

    ui->InfoManagementTab->setCurrentWidget(ui->flightTable);
    ui->departTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->arriveTimeEdit->setDateTime(QDateTime::currentDateTime());

    // 1. 初始化排序下拉框选项
    ui->sortComboBox->addItem("出发地", "departure_city");
    ui->sortComboBox->addItem("目的地", "arrival_city");
    ui->sortComboBox->addItem("出发时间", "departure_time");
    ui->sortComboBox->addItem("到达时间", "arrival_time");
    ui->sortComboBox->addItem("基础价格", "base_price");
    ui->sortComboBox->addItem("航空公司", "airline_company");
    ui->sortComboBox->addItem("创建时间", "create_time");
    // 2. 设置默认选中项：按创建时间排序（对应第7项，索引6）
    ui->sortComboBox->setCurrentIndex(6);


    // 初始加载数据
    queryFlightDataFromDB();  // 加载航班
    querySeatDataFromDB(); // 加载座位
    queryUserDataFromDB();    // 加载用户
    queryOrderDataFromDB(); // 加载订单
    initSeatManagement();     //初始化座位信息界面下拉框
    // ========== 用户支付信息页初始化 ==========
    initPayInfoTable();     // 列头 + 列宽
    queryPayInfoFromDB();   // 首次加载数据
    queryComplaintsFromDB();
    queryMealOrdersFromDB();

    on_btnRefreshrebook_clicked();

    // ========== 新增：加载 QSS 样式 ==========
    QString qss = R"(
        /* 全局样式 */
        QMainWindow {
            background-color: #F8F9FA;
            font-family: "微软雅黑", "Source Han Sans CN";
            font-size: 14px;
            color: #212121;
        }

        /* 标签页 */
        QTabWidget::pane {
            border: 1px solid #E0E0E0;
            background-color: #FFFFFF;
        }
        QTabBar::tab {
            padding: 8px 16px;
            margin-right: 2px;
            border-bottom-left-radius: 4px;
            border-bottom-right-radius: 4px;
            color: #757575;
        }
        QTabBar::tab:selected, QTabBar::tab:hover {
            color: #1E88E5;
        }
        QTabBar::tab:selected {
            border-bottom: 2px solid #1E88E5;
        }

        /* 表格 */
        QTableWidget {
            background-color: #FFFFFF;
            border: 1px solid #E0E0E0;
            gridline-color: #E0E0E0;
            border-radius: 4px;
        }
        QHeaderView::section {
            background-color: #1E88E5;
            color: #FFFFFF;
            padding: 8px;
            border: none;
        }
        QTableWidget::item:alternate {
            background-color: #F5F5F5;
        }
        QTableWidget::item:hover {
            background-color: #E3F2FD;
        }
        QTableWidget::item:selected {
            background-color: #BBDEFB;
            color: #212121;
        }

        /* 按钮 */
        QPushButton {
            height: 32px;
            padding: 0 16px;
            border-radius: 4px;
            border: none;
            background-color: #1E88E5;
            color: #FFFFFF;
        }
        QPushButton:hover {
            background-color: #2196F3;
        }
        QPushButton:pressed {
            background-color: #1976D2;
        }
        QPushButton#deleteFlightBtn, QPushButton#banUserBtn {
            background-color: #D32F2F;
        }
        QPushButton#deleteFlightBtn:hover, QPushButton#banUserBtn:hover {
            background-color: #C62828;
        }
        QPushButton#btnClearFlight, QPushButton#btnClearInput {
            background-color: #FFFFFF;
            border: 1px solid #1E88E5;
            color: #1E88E5;
        }

        /* 输入框 */
        QLineEdit, QDateEdit, QComboBox, QDoubleSpinBox {
            height: 32px;
            padding: 0 10px;
            border: 1px solid #E0E0E0;
            border-radius: 4px;
            background-color: #FFFFFF;
        }
        QLineEdit:focus, QDateEdit:focus, QComboBox:focus, QDoubleSpinBox:focus {
            border-color: #1E88E5;
            box-shadow: 0 0 0 2px rgba(30, 136, 229, 0.2);
        }

        QMenuBar {
            background-color: #FFFFFF; /* 与标签栏背景一致 */
            border-bottom: 1px solid #E0E0E0; /* 底部分隔线，衔接标签栏 */
            font-size: 14px;
            height: 36px; /* 固定高度，适配整体风格 */
        }

        /* 主菜单项（如“菜单”“航班信息管理”） */
        QMenuBar::item {
            padding: 0 16px; /* 左右间距，增大点击区域 */
            color: #212121; /* 文字主色 */
            background-color: transparent; /* 默认透明 */
        }

        /* 鼠标 hover 主菜单项 */
        QMenuBar::item:hover {
            background-color: #E3F2FD; /* 浅蓝背景，与表格 hover 呼应 */
            color: #1E88E5; /* 主色调文字，突出反馈 */
        }

        /* 主菜单项选中（展开下拉时） */
        QMenuBar::item:selected {
            background-color: #BBDEFB; /* 深蓝背景，与表格选中一致 */
            color: #1E88E5;
        }
    )";

    this->setStyleSheet(qss); // 应用样式到主窗口及所有子控件
    // ========== 用户支付信息页刷新按钮 ==========
    connect(ui->btnRefreshPayInfo, &QPushButton::clicked,
            this, &AdminWindow::queryPayInfoFromDB);
}

AdminWindow::~AdminWindow()
{
    delete ui;
}

void AdminWindow::queryComplaintsFromDB() {
    DBHelper::getInstance()->getAllComplaints([this](bool success, const QJsonArray& complaints) {
        if (success) {
            loadComplaintsToTable(complaints);
        }
    });
}
void AdminWindow::loadComplaintsToTable(const QJsonArray& complaints) {
    ui->complaintsTable->setRowCount(0);
    for (int i = 0; i < complaints.size(); ++i) {
        QJsonObject c = complaints[i].toObject();
        ui->complaintsTable->insertRow(i);

        // 第0列：用户名（显示用户名而非手机号）
        ui->complaintsTable->setItem(i, 0, new QTableWidgetItem(c["user_name"].toString()));
        // 第1列：手机号
        ui->complaintsTable->setItem(i, 1, new QTableWidgetItem(c["user_phone"].toString()));
        // 第2列：类型（投诉/建议）
        ui->complaintsTable->setItem(i, 2, new QTableWidgetItem(c["type"].toString()));
        // 第3列：内容（去掉航班号信息）
        ui->complaintsTable->setItem(i, 3, new QTableWidgetItem(c["content"].toString()));
        // 第4列：提交时间
        ui->complaintsTable->setItem(i, 4, new QTableWidgetItem(c["submit_time"].toString()));
    }

    //设置列宽
    ui->complaintsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->complaintsTable->setColumnWidth(0, 150);   // 用户名
    ui->complaintsTable->setColumnWidth(1, 150);  // 手机号
    ui->complaintsTable->setColumnWidth(2, 80);  // 类型
    ui->complaintsTable->setColumnWidth(3, 500);  // 内容
    ui->complaintsTable->setColumnWidth(4, 180);  // 提交时间
    // 可选：设置列最小宽度
    ui->complaintsTable->horizontalHeader()->setMinimumSectionSize(60);
}

void AdminWindow::queryMealOrdersFromDB() {
    DBHelper::getInstance()->getAllMealOrders([this](bool success, const QJsonArray& orders) {
        if (success) {
            loadMealOrdersToTable(orders);
        }
    });
}
void AdminWindow::loadMealOrdersToTable(const QJsonArray& orders) {
    ui->mealTable->setRowCount(0);
    for (int i = 0; i < orders.size(); ++i) {
        QJsonObject o = orders[i].toObject();
        ui->mealTable->insertRow(i);

        // 第0列：用户名
        ui->mealTable->setItem(i, 0, new QTableWidgetItem(o["user_name"].toString()));
        // 第1列：手机号
        ui->mealTable->setItem(i, 1, new QTableWidgetItem(o["user_phone"].toString()));
        // 第2列：航班号（对应UI的"航班"列）
        ui->mealTable->setItem(i, 2, new QTableWidgetItem(o["flight_num"].toString()));
        // 第3列：航班时间（使用获取到的时间）
        ui->mealTable->setItem(i, 3, new QTableWidgetItem(o["flight_time"].toString()));
        // 第4列：预定餐食（对应UI的"预定餐食"列）
        ui->mealTable->setItem(i, 4, new QTableWidgetItem(o["meal_name"].toString()));
        // 第5列：提交时间（对应UI的"提交时间"列）
        ui->mealTable->setItem(i, 5, new QTableWidgetItem(o["order_time"].toString()));
    }

    ui->mealTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->mealTable->setColumnWidth(0, 150);   // 用户名
    ui->mealTable->setColumnWidth(1, 150);  // 手机号
    ui->mealTable->setColumnWidth(2, 100);  // 航班号
    ui->mealTable->setColumnWidth(3, 200);  // 航班时间
    ui->mealTable->setColumnWidth(4, 150);  // 餐名
    ui->mealTable->setColumnWidth(5, 200);  // 提交时间
    // 可选：设置列最小宽度
    ui->mealTable->horizontalHeader()->setMinimumSectionSize(60);
}

// ========== 航班信息页加载==========
// 查询航班数据
void AdminWindow::queryFlightDataFromDB()
{
    DBHelper::getInstance()->queryFlightsWithFilterAndSort(
        m_flightSearchNum,
        m_sortField,
        [this](bool success, const QJsonArray& flights) {
            onFlightsQueryResult(success, flights); // 复用原有回调，不改动表格渲染
        }
    );
}

//加了个提醒功能
void AdminWindow::onFlightsQueryResult(bool success, const QJsonArray& flights)
{
    if (success) {
        loadFlightDataToTable(flights);

        // ========== 新增：检查即将起飞且未设置登机口的航班 ==========
        QDateTime currentTime = QDateTime::currentDateTime();
        QStringList imminentFlightNums;

        for (const QJsonValue& flightVal : flights) {
            QJsonObject flight = flightVal.toObject();
            QString gate = flight["gate"].toString();

            // 检查登机机口是否未设置
            if (gate.isEmpty() || gate == "未设置") {
                // 解析起飞时间
                QString departTimeStr = flight["departure_time"].toString();
                QDateTime departTime = QDateTime::fromString(departTimeStr, "yyyy-MM-ddTHH:mm:ss");
                if (!departTime.isValid()) {
                    departTime = QDateTime::fromString(departTimeStr, "yyyy-MM-dd HH:mm:ss");
                }

                if (departTime.isValid()) {
                    // 计算时间差（分钟）
                    qint64 minutesDiff = currentTime.secsTo(departTime) / 60;
                    // 筛选30分钟内且未起飞的航班
                    if (minutesDiff >= 0 && minutesDiff <= 30) {
                        imminentFlightNums.append(flight["flight_num"].toString());
                    }
                }
            }
        }

        // 显示提示对话框
        if (!imminentFlightNums.isEmpty()) {
            QString message = QString("以下列航班将在30分钟内起飞，请及时时设置登机口：\n%1")
                                  .arg(imminentFlightNums.join("\n"));
            QMessageBox::information(this, "登机口提醒", message, QMessageBox::Ok);
        }
        // =======================================================

    } else {
        QMessageBox::critical(this, "查询错误", "航班数据查询失败！", QMessageBox::Ok);
    }
}

// 填充航班数据到表格（新增登机口列和数据填充）
void AdminWindow::loadFlightDataToTable(const QJsonArray& flights)
{
    //清空表格旧数据
    ui->flightTable->setRowCount(0);

    //无数据直接返回
    if (flights.isEmpty()) {
        return;
    }

    //逐行填充数据到表格
    for (int i = 0; i < flights.size(); ++i) {
        QJsonObject rowData = flights[i].toObject();
        ui->flightTable->insertRow(i);

        ui->flightTable->setItem(i, 0, new QTableWidgetItem(rowData["flight_num"].toString()));
        ui->flightTable->setItem(i, 1, new QTableWidgetItem(rowData["departure_city"].toString()));
        ui->flightTable->setItem(i, 2, new QTableWidgetItem(rowData["arrival_city"].toString()));
        //ui->flightTable->setItem(i, 3, new QTableWidgetItem(rowData["departure_time"].toString()));
        //ui->flightTable->setItem(i, 4, new QTableWidgetItem(rowData["arrival_time"].toString()));
        ui->flightTable->setItem(i, 5, new QTableWidgetItem(QString::number(rowData["base_price"].toDouble(), 'f', 2)));
        ui->flightTable->setItem(i, 6, new QTableWidgetItem(rowData["airline_company"].toString()));
        //ui->flightTable->setItem(i, 8, new QTableWidgetItem(rowData["create_time"].toString()));

        // 新增：登机口数据填充
        QString gate = rowData["gate"].toString();
        ui->flightTable->setItem(i, 7, new QTableWidgetItem(gate.isEmpty() ? "未设置" : gate));

        // 修改后（格式化时间为“年-月-日 时:分”）
        QString departTimeStr = rowData["departure_time"].toString();
        QDateTime departTime = QDateTime::fromString(departTimeStr, "yyyy-MM-ddTHH:mm:ss"); // 解析数据库时间格式
        if (departTime.isValid()) {
            // 格式化为“2025-12-05 08:30”
            ui->flightTable->setItem(i, 3, new QTableWidgetItem(departTime.toString("yyyy-MM-dd HH:mm")));
        } else {
            // 解析失败时显示原始字符串
            ui->flightTable->setItem(i, 3, new QTableWidgetItem(departTimeStr));
        }

        // 到达时间同理
        QString arriveTimeStr = rowData["arrival_time"].toString();
        QDateTime arriveTime = QDateTime::fromString(arriveTimeStr, "yyyy-MM-ddTHH:mm:ss");
        if (arriveTime.isValid()) {
            ui->flightTable->setItem(i, 4, new QTableWidgetItem(arriveTime.toString("yyyy-MM-dd HH:mm")));
        } else {
            ui->flightTable->setItem(i, 4, new QTableWidgetItem(arriveTimeStr));
        }

        // 创建时间同理
        QString createTimeStr = rowData["create_time"].toString();
        QDateTime createTime = QDateTime::fromString(createTimeStr, "yyyy-MM-ddTHH:mm:ss");
        if (createTime.isValid()) {
            ui->flightTable->setItem(i, 8, new QTableWidgetItem(createTime.toString("yyyy-MM-dd HH:mm")));
        } else {
            ui->flightTable->setItem(i, 8, new QTableWidgetItem(createTimeStr));
        }
    }

    //调整列宽
    ui->flightTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    // 按比例设置初始宽度
    ui->flightTable->setColumnWidth(0, 100);   // 航班号
    ui->flightTable->setColumnWidth(1, 70);  // 出发地
    ui->flightTable->setColumnWidth(2, 70);  // 目的地
    ui->flightTable->setColumnWidth(3, 180);  // 出发时间
    ui->flightTable->setColumnWidth(4, 180);   // 到达时间
    ui->flightTable->setColumnWidth(5, 100);   // 价格
    ui->flightTable->setColumnWidth(6, 150);   // 公司
    ui->flightTable->setColumnWidth(7, 60);   // 登机口
    ui->flightTable->setColumnWidth(8, 180);   // 创建时间
    // 可选：设置列最小宽度
    ui->flightTable->horizontalHeader()->setMinimumSectionSize(60);
}


// ========== 用户信息页加载 ==========
void AdminWindow::queryUserDataFromDB()
{
    DBHelper::getInstance()->queryUsers([this](bool success, const QJsonArray& users) {
        onUsersQueryResult(success, users);
    });
}

void AdminWindow::onUsersQueryResult(bool success, const QJsonArray& users)
{
    if (success) {
        loadUserDataToTable(users);
    } else {
        QMessageBox::critical(this, "查询错误", "用户数据查询失败！", QMessageBox::Ok);
    }
}

// 填充查询结果到表格
void AdminWindow::loadUserDataToTable(const QJsonArray& users)
{
    // 清空表格旧数据
    ui->userTable->setRowCount(0);

    // 逐行填充数据到表格
    for (int i = 0; i < users.size(); ++i) {
        QJsonObject rowData = users[i].toObject();

        // 添加新行
        ui->userTable->insertRow(i);

        // 设置列数据
        ui->userTable->setItem(i, 0, new QTableWidgetItem(rowData["user_nickname"].toString()));
        ui->userTable->setItem(i, 1, new QTableWidgetItem(rowData["user_phone"].toString()));
        //ui->userTable->setItem(i, 2, new QTableWidgetItem(rowData["create_time"].toString()));
        //ui->userTable->setItem(i, 3, new QTableWidgetItem(rowData["update_time"].toString()));

        // 修改后
        QString userCreateTimeStr = rowData["create_time"].toString();
        QDateTime userCreateTime = QDateTime::fromString(userCreateTimeStr, "yyyy-MM-dd HH:mm:ss");
        if (userCreateTime.isValid()) {
            ui->userTable->setItem(i, 2, new QTableWidgetItem(userCreateTime.toString("yyyy-MM-dd HH:mm")));
        } else {
            ui->userTable->setItem(i, 2, new QTableWidgetItem(userCreateTimeStr));
        }

        QString userUpdateTimeStr = rowData["update_time"].toString();
        QDateTime userUpdateTime = QDateTime::fromString(userUpdateTimeStr, "yyyy-MM-dd HH:mm:ss");
        if (userUpdateTime.isValid()) {
            ui->userTable->setItem(i, 4, new QTableWidgetItem(userUpdateTime.toString("yyyy-MM-dd HH:mm")));
        } else {
            ui->userTable->setItem(i, 4, new QTableWidgetItem(userUpdateTimeStr));
        }

        // 状态列
        bool isBanned = rowData["is_banned"].toBool();
        ui->userTable->setItem(i, 3, new QTableWidgetItem(isBanned ? "封禁" : "正常"));
    }

    //调整列宽
    ui->userTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->userTable->setColumnWidth(0, 200);   // 用户名
    ui->userTable->setColumnWidth(1, 200);  // 手机号
    ui->userTable->setColumnWidth(2, 250);  // 注册时间
    ui->userTable->setColumnWidth(3, 100);  // 状态
    ui->userTable->setColumnWidth(4, 250);  // 更新时间
    // 可选：设置列最小宽度
    ui->userTable->horizontalHeader()->setMinimumSectionSize(60);

}

// ========== 座位信息页加载 ==========
void AdminWindow::querySeatDataFromDB(const QString& flightNum)
{
    DBHelper::getInstance()->querySeats(flightNum, [this](bool success, const QJsonArray& seats) {
        onSeatsQueryResult(success, seats);
    });
}

void AdminWindow::onSeatsQueryResult(bool success, const QJsonArray& seats)
{
    if (success) {
        loadSeatDataToTable(seats);
    } else {
        QMessageBox::critical(this, "查询错误", "座位数据查询失败！", QMessageBox::Ok);
    }
}

void AdminWindow::loadSeatDataToTable(const QJsonArray& seats)
{
    ui->tableSeat->setRowCount(0); // 清空旧数据

    // 初始化统计变量
    int total = 0;
    int occupied = 0;
    int available = 0;

    for (int i = 0; i < seats.size(); ++i) {
        QJsonObject rowData = seats[i].toObject();
        ui->tableSeat->insertRow(i);

        ui->tableSeat->setItem(i, 0, new QTableWidgetItem(rowData["flight_num"].toString()));
        ui->tableSeat->setItem(i, 1, new QTableWidgetItem(rowData["seat_num"].toString()));

        // 舱位类型转换（数据库英文 -> 界面中文）
        QString seatType = rowData["seat_type"].toString();
        QString seatTypeText;
        if (seatType == "first_class")      seatTypeText = "头等舱";
        else if (seatType == "business_class") seatTypeText = "商务舱";
        else if (seatType == "economy_class")  seatTypeText = "经济舱";
        else seatTypeText = seatType; // 未知类型保持原值

        ui->tableSeat->setItem(i, 2, new QTableWidgetItem(seatTypeText));
        ui->tableSeat->setItem(i, 3, new QTableWidgetItem(QString::number(rowData["seat_price"].toDouble(), 'f', 2)));

        // 座位状态转换（数据库英文 -> 界面中文）
        QString seatStatus = rowData["seat_status"].toString();
        QString seatStatusText;
        if (seatStatus == "available")  {
            seatStatusText = "可选";
            available++; // 统计可选座位
        }
        else if (seatStatus == "occupied") {
            seatStatusText = "已占";
            occupied++; // 统计已占座位
        }
        else {
            seatStatusText = seatStatus; // 未知状态保持原值
        }

        QTableWidgetItem* statusItem = new QTableWidgetItem(seatStatusText);
        ui->tableSeat->setItem(i, 4, statusItem);

        //ui->tableSeat->setItem(i, 5, new QTableWidgetItem(rowData["create_time"].toString()));

        // ========== 格式化创建时间（列5） ==========
        QString createTimeStr = rowData["create_time"].toString();
        QDateTime createTime = QDateTime::fromString(createTimeStr, "yyyy-MM-dd HH:mm:ss");
        // 解析成功则格式化显示，失败则显示原始字符串
        if (createTime.isValid()) {
            ui->tableSeat->setItem(i, 5, new QTableWidgetItem(createTime.toString("yyyy-MM-dd HH:mm")));
        } else {
            ui->tableSeat->setItem(i, 5, new QTableWidgetItem(createTimeStr));
        }

        total++; // 累计总座位数

    }

    //调整列宽
    ui->tableSeat->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableSeat->setColumnWidth(0, 160);   // 航班号
    ui->tableSeat->setColumnWidth(1, 120);  // 座位号
    ui->tableSeat->setColumnWidth(2, 120);  // 类型
    ui->tableSeat->setColumnWidth(3, 150);  // 价格
    ui->tableSeat->setColumnWidth(4, 120);   // 状态
    ui->tableSeat->setColumnWidth(5, 200);   // 时间
    // 可选：设置列最小宽度
    ui->tableSeat->horizontalHeader()->setMinimumSectionSize(60);

    // 更新统计标签显示
    ui->lblSeatStats->setText(QString("共 %1 个座位，已占 %2 个，可选 %3 个")
                                  .arg(total)
                                  .arg(occupied)
                                  .arg(available));

}

// ========== 订单信息页加载==========
void AdminWindow::queryOrderDataFromDB(
    const QString& name,
    const QString& idCard,
    const QString& flightNum,
    QDate startDate,
    QDate endDate,
    const QString& statusFilter)
{
    QJsonObject filters;
    if (!name.isEmpty()) filters["passenger_name"] = name;
    if (!idCard.isEmpty()) filters["passenger_idcard"] = idCard;
    if (!flightNum.isEmpty()) filters["flight_num"] = flightNum;
    if (startDate.isValid() && endDate.isValid()) {
        filters["start_date"] = startDate.toString("yyyy-MM-dd");
        filters["end_date"] = endDate.toString("yyyy-MM-dd");
    }
    if (!statusFilter.isEmpty()) filters["status_filter"] = statusFilter;

    DBHelper::getInstance()->queryOrders(filters, [this](bool success, const QJsonArray& orders) {
        onOrdersQueryResult(success, orders);
    });
}

void AdminWindow::onOrdersQueryResult(bool success, const QJsonArray& orders)
{
    if (success) {
        loadOrderDataToTable(orders);
    } else {
        QMessageBox::critical(this, "查询错误", "订单数据查询失败！", QMessageBox::Ok);
    }
}

void AdminWindow::loadOrderDataToTable(const QJsonArray& orders)
{
    ui->orderTable->setRowCount(0); // 清空旧数据

    for (int i = 0; i < orders.size(); ++i) {
        QJsonObject rowData = orders[i].toObject();
        ui->orderTable->insertRow(i);

        ui->orderTable->setItem(i, 0, new QTableWidgetItem(rowData["order_id"].toString()));
        ui->orderTable->setItem(i, 1, new QTableWidgetItem(rowData["user_phone"].toString())); // 显示手机号
        ui->orderTable->setItem(i, 2, new QTableWidgetItem(rowData["flight_num"].toString())); // 显示航班号
        ui->orderTable->setItem(i, 3, new QTableWidgetItem(rowData["seat_num"].toString()));   // 显示座位号
        ui->orderTable->setItem(i, 4, new QTableWidgetItem(rowData["passenger_name"].toString()));
        ui->orderTable->setItem(i, 5, new QTableWidgetItem(rowData["passenger_idcard"].toString()));
        //ui->orderTable->setItem(i, 6, new QTableWidgetItem(rowData["order_status"].toString()));
        ui->orderTable->setItem(i, 7, new QTableWidgetItem(QString::number(rowData["order_amount"].toDouble(), 'f', 2)));
        //ui->orderTable->setItem(i, 8, new QTableWidgetItem(rowData["create_time"].toString()));
        //ui->orderTable->setItem(i, 9, new QTableWidgetItem(rowData["pay_time"].toString()));

        // ========== 新增：订单状态中英文映射（改为中文显示） ==========
        QString orderStatus = rowData["order_status"].toString();
        QString statusText;
        if (orderStatus == "unpaid") {
            statusText = "未付款";
        } else if (orderStatus == "paid") {
            statusText = "已付款";
        } else if (orderStatus == "cancelled") {
            statusText = "已取消";
        } else {
            statusText = "未知状态"; // 兼容异常状态
        }
        ui->orderTable->setItem(i, 6, new QTableWidgetItem(statusText));

        // ========== 格式化创建时间（列8） ==========
        QString createTimeStr = rowData["create_time"].toString();
        QDateTime createTime = QDateTime::fromString(createTimeStr, "yyyy-MM-dd HH:mm:ss");
        // if (!createTime.isValid()) {
        //     // 若上面格式解析失败，尝试不带T的格式（如"2025-12-05 13:52:44"）
        //     createTime = QDateTime::fromString(createTimeStr, "yyyy-MM-dd HH:mm:ss");
        // }
        if (createTime.isValid()) {
            ui->orderTable->setItem(i, 8, new QTableWidgetItem(createTime.toString("yyyy-MM-dd HH:mm")));
        } else {
            ui->orderTable->setItem(i, 8, new QTableWidgetItem(createTimeStr));
        }

        // ========== 格式化支付时间（列9） ==========
        QString payTimeStr = rowData["pay_time"].toString();
        // 兼容空支付时间（未支付订单）
        if (payTimeStr.isEmpty() || payTimeStr == "null") {
            ui->orderTable->setItem(i, 9, new QTableWidgetItem("未支付"));
        } else {
            QDateTime payTime = QDateTime::fromString(payTimeStr, "yyyy-MM-dd HH:mm:ss");
            if (payTime.isValid()) {
                ui->orderTable->setItem(i, 9, new QTableWidgetItem(payTime.toString("yyyy-MM-dd HH:mm")));
            } else {
                ui->orderTable->setItem(i, 9, new QTableWidgetItem(payTimeStr));
            }
        }

    }

    // 订单表列宽设置（允许拖动+初始宽度）
    ui->orderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->orderTable->setColumnWidth(0, 150);  // 订单ID
    ui->orderTable->setColumnWidth(1, 150);  // 手机号
    ui->orderTable->setColumnWidth(2, 80);  // 航班号
    ui->orderTable->setColumnWidth(3, 60);   // 座位号
    ui->orderTable->setColumnWidth(4, 80);  // 乘客姓名
    ui->orderTable->setColumnWidth(5, 180);  // 身份证号
    ui->orderTable->setColumnWidth(6, 60);   // 订单状态
    ui->orderTable->setColumnWidth(7, 80);  // 订单金额
    ui->orderTable->setColumnWidth(8, 150);  // 创建时间
    ui->orderTable->setColumnWidth(9, 150);  // 支付时间
    ui->orderTable->horizontalHeader()->setMinimumSectionSize(60);

    updateOrderCount(orders.size());
}


// ========== 航班信息页操作==========

void AdminWindow::on_addFlightBtn_clicked()
{
    // 获取输入数据
    QString flightNum = ui->flightNumEdit->text().trimmed();
    QString fromCity = ui->fromCityEdit->text().trimmed();
    QString toCity = ui->toCityEdit->text().trimmed();
    QString departTime = ui->departTimeEdit->dateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString arriveTime = ui->arriveTimeEdit->dateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString price = ui->basePriceEdit->text().trimmed();
    QString airline = ui->airlineEdit->text().trimmed();
    QString gate = ui->gateEdit->text().trimmed();

    // 输入验证
    if (flightNum.isEmpty() || fromCity.isEmpty() || toCity.isEmpty() || price.isEmpty() || airline.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "航班信息不能为空！", QMessageBox::Ok);
        return;
    }

    // ========== 新增：校验航班号是否重复 ==========
    // 先查询所有航班，检查航班号是否已存在
    DBHelper::getInstance()->queryFlights([this, flightNum, fromCity, toCity, departTime, arriveTime, price, airline, gate](bool success, const QJsonArray& flights) {
        if (!success) {
            QMessageBox::critical(this, "查询错误", "校验航班号失败，请重试！", QMessageBox::Ok);
            return;
        }

        // 遍历航班列表，检查是否存在相同航班号
        bool isDuplicate = false;
        for (const QJsonValue& val : flights) {
            QJsonObject flight = val.toObject();
            if (flight["flight_num"].toString() == flightNum) {
                isDuplicate = true;
                break;
            }
        }

        if (isDuplicate) {
            // 航班号重复，提示失败
            QMessageBox::critical(this, "添加失败", "该航班号已存在，请勿重复添加！", QMessageBox::Ok);
            return;
        }

        // 航班号唯一，继续添加流程
        QJsonObject flightData;
        flightData["flight_num"] = flightNum;
        flightData["departure_city"] = fromCity;
        flightData["arrival_city"] = toCity;
        flightData["departure_time"] = departTime;
        flightData["arrival_time"] = arriveTime;
        flightData["base_price"] = price.toDouble();
        flightData["airline_company"] = airline;
        flightData["total_seats"] = 200; // 默认值
        flightData["gate"] = gate;

        // 发送添加航班请求
        DBHelper::getInstance()->addFlight(flightData, [this](bool success, const QString& message) {
            onAddFlightResult(success, message);
        });
    });
}

void AdminWindow::onAddFlightResult(bool success, const QString& message)
{
    if (success) {
        QMessageBox::information(this, "成功", "航班添加成功！", QMessageBox::Ok);
        queryFlightDataFromDB(); // 刷新表格
        on_btnClearFlight_clicked(); // 重置输入框
    } else {
        QMessageBox::critical(this, "失败", "航班添加失败：" + message, QMessageBox::Ok);
    }
}

void AdminWindow::on_updateFlightBtn_clicked()
{
    // 获取当前选中行的原航班号（关键：用于识别要修改的旧航班号）
    int currentRow = ui->flightTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "错误", "请选中要修改的航班！", QMessageBox::Ok);
        return;
    }
    QString oldFlightNum = ui->flightTable->item(currentRow, 0)->text(); // 原航班号
    if (oldFlightNum.isEmpty()) {
        QMessageBox::warning(this, "错误", "选中的航班信息异常！", QMessageBox::Ok);
        return;
    }

    // 获取更新数据（新的航班号和其他信息）
    QString newFlightNum = ui->flightNumEdit->text().trimmed();
    QString fromCity = ui->fromCityEdit->text().trimmed();
    QString toCity = ui->toCityEdit->text().trimmed();
    QString departTime = ui->departTimeEdit->dateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString arriveTime = ui->arriveTimeEdit->dateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString price = ui->basePriceEdit->text().trimmed();
    QString airline = ui->airlineEdit->text().trimmed();
    QString gate = ui->gateEdit->text().trimmed();

    // 输入验证
    if (newFlightNum.isEmpty() || fromCity.isEmpty() || toCity.isEmpty() || price.isEmpty() || airline.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "航班信息不能为空！", QMessageBox::Ok);
        return;
    }

    // ========== 新增：如果修改了航班号，校验新航班号是否重复（排除自身） ==========
    if (newFlightNum != oldFlightNum) {
        DBHelper::getInstance()->queryFlights([this, oldFlightNum, newFlightNum, fromCity, toCity, departTime, arriveTime, price, airline, gate](bool success, const QJsonArray& flights) {
            if (!success) {
                QMessageBox::critical(this, "查询错误", "校验航班号失败，请重试！", QMessageBox::Ok);
                return;
            }

            // 检查新航班号是否已存在（排除原航班号）
            bool isDuplicate = false;
            for (const QJsonValue& val : flights) {
                QJsonObject flight = val.toObject();
                QString existFlightNum = flight["flight_num"].toString();
                if (existFlightNum == newFlightNum && existFlightNum != oldFlightNum) {
                    isDuplicate = true;
                    break;
                }
            }

            if (isDuplicate) {
                QMessageBox::critical(this, "修改失败", "新航班号已存在，无法修改！", QMessageBox::Ok);
                return;
            }

            // 新航班号唯一，继续更新流程
            updateFlightData(oldFlightNum, newFlightNum, fromCity, toCity, departTime, arriveTime, price, airline, gate);
        });
    } else {
        // 未修改航班号，直接更新
        updateFlightData(oldFlightNum, newFlightNum, fromCity, toCity, departTime, arriveTime, price, airline, gate);
    }
}

// 封装更新航班数据的逻辑（避免代码重复）
void AdminWindow::updateFlightData(const QString& oldFlightNum, const QString& newFlightNum,
                                  const QString& fromCity, const QString& toCity,
                                  const QString& departTime, const QString& arriveTime,
                                  const QString& price, const QString& airline, const QString& gate)
{
    // 准备更新数据
    QJsonObject flightData;
    flightData["old_flight_num"] = oldFlightNum; // 传递原航班号，供服务端识别
    flightData["new_flight_num"] = newFlightNum; // 新航班号
    flightData["departure_city"] = fromCity;
    flightData["arrival_city"] = toCity;
    flightData["departure_time"] = departTime;
    flightData["arrival_time"] = arriveTime;
    flightData["base_price"] = price.toDouble();
    flightData["airline_company"] = airline;
    flightData["gate"] = gate;

    // 发送更新航班请求
    DBHelper::getInstance()->updateFlight(flightData, [this](bool success, const QString& message) {
        onUpdateFlightResult(success, message);
    });
}


void AdminWindow::onUpdateFlightResult(bool success, const QString& message)
{
    if (success) {
        QMessageBox::information(this, "成功", "航班更新成功！", QMessageBox::Ok);
        queryFlightDataFromDB();
    } else {
        QMessageBox::critical(this, "失败", "航班更新失败：" + message, QMessageBox::Ok);
    }
}

void AdminWindow::on_deleteFlightBtn_clicked()
{
    int currentRow = ui->flightTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "错误", "请选中要删除的航班！", QMessageBox::Ok);
        return;
    }

    QString flightNum = ui->flightTable->item(currentRow, 0)->text();
    if (QMessageBox::question(this, "确认", "确定要删除航班 " + flightNum + " 吗？",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // 发送删除航班请求
    DBHelper::getInstance()->deleteFlight(flightNum, [this](bool success, const QString& message) {
        onDeleteFlightResult(success, message);
    });
}

void AdminWindow::onDeleteFlightResult(bool success, const QString& message)
{
    if (success) {
        QMessageBox::information(this, "成功", "航班删除成功！", QMessageBox::Ok);
        queryFlightDataFromDB();
        on_btnClearFlight_clicked();
    } else {
        QMessageBox::critical(this, "失败", "航班删除失败：" + message, QMessageBox::Ok);
    }
}

void AdminWindow::on_flightTable_itemClicked(QTableWidgetItem *item)
{
    if (!item) return;
    int row = item->row();

    // 从表格选中行加载数据到编辑框
    ui->flightNumEdit->setText(ui->flightTable->item(row, 0)->text());
    ui->fromCityEdit->setText(ui->flightTable->item(row, 1)->text());
    ui->toCityEdit->setText(ui->flightTable->item(row, 2)->text());
    ui->departTimeEdit->setDateTime(QDateTime::fromString(ui->flightTable->item(row, 3)->text(), "yyyy-MM-dd HH:mm"));
    ui->arriveTimeEdit->setDateTime(QDateTime::fromString(ui->flightTable->item(row, 4)->text(), "yyyy-MM-dd HH:mm"));
    ui->basePriceEdit->setText(ui->flightTable->item(row, 5)->text());
    ui->airlineEdit->setText(ui->flightTable->item(row, 6)->text());

    // 新增：加载登机口信息到gateEdit（与航空公司编辑框逻辑一致）
    QString gate = ui->flightTable->item(row, 7)->text();
    if (gate != "未设置") {
        ui->gateEdit->setText(gate);
    } else {
        ui->gateEdit->clear();
    }
}

void AdminWindow::on_btnClearFlight_clicked()
{
    // 重置输入框
    ui->flightNumEdit->clear();
    ui->fromCityEdit->clear();
    ui->toCityEdit->clear();
    ui->departTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->arriveTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->basePriceEdit->clear();
    ui->airlineEdit->clear();
    ui->gateEdit->clear();
}

void AdminWindow::on_refreshFlightsBtn_clicked()
{
    queryFlightDataFromDB();
}

void AdminWindow::on_sortComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    // 获取选中的排序字段
    m_sortField  = ui->sortComboBox->currentData().toString();
    // 触发查询（排序变化时刷新）
    queryFlightDataFromDB();
}


void AdminWindow::on_searchBtn_clicked()
{
    m_flightSearchNum  = ui->flightNumsearch->text().trimmed();
    queryFlightDataFromDB();
}


// ========== 用户信息页操作 ==========

void AdminWindow::on_refreshUsersBtn_clicked()
{
    queryUserDataFromDB();
}

// 1. 重置密码功能
void AdminWindow::on_rstUserPwdBtn_clicked()
{
    QModelIndexList selectedRows = ui->userTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要重置密码的用户");
        return;
    }

    int selectedRow = selectedRows.first().row();
    QString phone = ui->userTable->item(selectedRow, 1)->text();
    QString username = ui->userTable->item(selectedRow, 0)->text();

    int reply = QMessageBox::question(this,
                                      "确认重置密码",
                                      QString("确定要将用户【%1】（手机号：%2）的密码重置为默认密码【123456】吗？")
                                              .arg(username, phone),  // 把多个参数直接放到同一个arg()里
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 生成当前时间（用于服务器端更新）
        QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");

        // 发送请求时携带时间参数
        QJsonObject data;
        data["phone"] = phone;
        data["new_password"] = "123456";
        data["update_time"] = currentTime; // 新增：传递当前时间到服务器

        DBHelper::getInstance()->sendRequest("userResetPassword", data,
                                             [this, selectedRow, currentTime](const QJsonObject& response) {
                                                 if (response["success"].toBool()) {
                                                     // 同步更新客户端表格时间
                                                     ui->userTable->setItem(selectedRow, 4, new QTableWidgetItem(currentTime));
                                                     QMessageBox::information(this, "成功", "密码已重置为默认密码：123456");
                                                     on_refreshUsersBtn_clicked();
                                                 } else {
                                                     QMessageBox::critical(this, "失败", "密码重置失败：" + response["message"].toString());
                                                 }
                                             });
    }
}

// 2. 封禁/解封用户功能
void AdminWindow::on_banUserBtn_clicked()
{
    int selectedRow = ui->userTable->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要操作的用户");
        return;
    }

    QString phone = ui->userTable->item(selectedRow, 1)->text();
    QString username = ui->userTable->item(selectedRow, 0)->text(); // 获取用户名
    if (phone.isEmpty()) {
        QMessageBox::warning(this, "错误", "无法获取用户信息");
        return;
    }

    QString currentStatus = ui->userTable->item(selectedRow, 3)->text();
    bool isBanning = (currentStatus == "正常");
    // 拼接确认文案，明确操作类型和用户信息
    QString confirmText = isBanning
                              ? QString("确定要封禁用户【%1】（手机号：%2）吗？封禁后该用户将无法登录系统！")
                              : QString("确定要解封用户【%1】（手机号：%2）吗？解封后该用户可正常登录系统！");

    // 显示确认对话框
    int reply = QMessageBox::question(this,
                                      "确认操作",
                                      confirmText.arg(username,phone),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No); // 默认选中No，降低误操作风险

    if (reply != QMessageBox::Yes) {
        return; // 用户取消操作
    }

    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    QJsonObject data;
    data["phone"] = phone;
    data["is_banned"] = isBanning;
    data["update_time"] = currentTime; // 新增：传递当前时间到服务器

    DBHelper::getInstance()->sendRequest("updateUserStatus", data,
                                         [this, selectedRow, isBanning, currentTime](const QJsonObject& response) {
                                             if (response["success"].toBool()) {
                                                 ui->userTable->setItem(selectedRow, 3, new QTableWidgetItem(isBanning ? "封禁" : "正常"));
                                                 // 同步更新客户端表格时间
                                                 ui->userTable->setItem(selectedRow, 4, new QTableWidgetItem(currentTime));
                                                 QMessageBox::information(this, "成功", isBanning ? "用户已封禁" : "用户已解封");
                                                 on_refreshUsersBtn_clicked();
                                             } else {
                                                 QMessageBox::warning(this, "失败", response["message"].toString());
                                             }
                                         });
}


// ========== 座位信息页操作 ==========

void AdminWindow::initSeatManagement()
{
    // 1. 初始化航班编号下拉框（加载所有航班）
    loadFlightNumbersToCombo();

    // 2. 初始化舱位类型下拉框（中文显示，存储数据库对应值）
    ui->cboSeatType->addItem("头等舱", "first_class");
    ui->cboSeatType->addItem("商务舱", "business_class");
    ui->cboSeatType->addItem("经济舱", "economy_class");

    // 3. 初始化座位状态下拉框（中文显示，存储数据库对应值）
    ui->cboSeatStatus->addItem("可选", "available");
    ui->cboSeatStatus->addItem("已占", "occupied");

    // 4. 设置表格支持多选（批量操作）
    ui->tableSeat->setSelectionMode(QAbstractItemView::ExtendedSelection); // 支持Ctrl/Shift多选
    ui->tableSeat->setSelectionBehavior(QAbstractItemView::SelectRows);    // 整行选择

}

// 加载所有航班编号到下拉框
void AdminWindow::loadFlightNumbersToCombo()
{
    ui->cboFlightNum->clear();
    ui->cboFlightNum->addItem("请选择航班", ""); // 默认提示项

    // 使用HTTP请求获取航班列表
    DBHelper::getInstance()->queryFlights([this](bool success, const QJsonArray& flights) {
        if (success) {
            for (const QJsonValue& value : flights) {
                QJsonObject flight = value.toObject();
                QString flightNum = flight["flight_num"].toString();
                ui->cboFlightNum->addItem(flightNum, flightNum);
            }
        }
    });
}

void AdminWindow::on_tableSeat_itemSelectionChanged()
{
    // 获取所有选中的项
    QList<QTableWidgetItem*> selectedItems = ui->tableSeat->selectedItems();
    if (selectedItems.isEmpty()) {
        // 无选中项时清空编辑区（可选）
        //on_btnClearInput_clicked();
        return;
    }

    // 取第一行的行号（单选时有效，多选时默认加载第一行数据）
    int row = selectedItems.first()->row();

    // 加载数据到编辑框（与之前逻辑一致）
    ui->txtFlightNum->setText(ui->tableSeat->item(row, 0)->text());
    ui->txtSeatNum->setText(ui->tableSeat->item(row, 1)->text());
    double price = ui->tableSeat->item(row, 3)->text().toDouble();
    ui->spinSeatPrice->setValue(price);

    // 舱位类型反向转换（中文 -> 数据库值）
    QString typeText = ui->tableSeat->item(row, 2)->text();
    if (typeText == "头等舱") ui->cboSeatType->setCurrentIndex(0);
    else if (typeText == "商务舱") ui->cboSeatType->setCurrentIndex(1);
    else if (typeText == "经济舱") ui->cboSeatType->setCurrentIndex(2);

    // 座位状态反向转换（中文 -> 数据库值）
    QString statusText = ui->tableSeat->item(row, 4)->text();
    if (statusText == "可选") ui->cboSeatStatus->setCurrentIndex(0);
    else if (statusText == "已占") ui->cboSeatStatus->setCurrentIndex(1);

}

void AdminWindow::on_btnClearInput_clicked()
{
    ui->txtFlightNum->clear();
    ui->txtSeatNum->clear();
    ui->spinSeatPrice->clear();
    ui->cboSeatType->setCurrentIndex(0);
    ui->cboSeatStatus->setCurrentIndex(0);
    ui->tableSeat->clearSelection(); // 取消表格选中状态
}

void AdminWindow::on_btnRefreshSeat_clicked()
{
    QString flightNum = ui->cboFlightNum->currentData().toString();
    querySeatDataFromDB(flightNum);
}

void AdminWindow::on_btnAddSeat_clicked()
{
    QString flightNum = ui->txtFlightNum->text().trimmed();
    QString seatNum = ui->txtSeatNum->text().trimmed();
    QString seatType = ui->cboSeatType->currentData().toString(); // 直接获取数据库对应值
    double seatPrice = ui->spinSeatPrice->value();
    QString seatStatus = ui->cboSeatStatus->currentData().toString(); // 直接获取数据库对应值

    // 输入验证
    if (flightNum.isEmpty()) {
        QMessageBox::warning(this, "提示", "航班号不能为空！");
        return;
    }
    if (seatNum.isEmpty()) {
        QMessageBox::warning(this, "提示", "座位号不能为空！");
        ui->txtSeatNum->setFocus();
        return;
    }
    if (seatPrice <= 0) {
        QMessageBox::warning(this, "提示", "座位价格必须大于0！");
        ui->spinSeatPrice->setFocus();
        return;
    }

    // 准备座位数据
    QJsonObject seatData;
    seatData["flight_num"] = flightNum;
    seatData["seat_num"] = seatNum;
    seatData["seat_type"] = seatType;
    seatData["seat_price"] = seatPrice;
    seatData["seat_status"] = seatStatus;

    // 发送添加座位请求
    DBHelper::getInstance()->addSeat(seatData, [this](bool success, const QString& message) {
        onAddSeatResult(success, message);
    });
}

void AdminWindow::onAddSeatResult(bool success, const QString& message)
{
    if (success) {
        QMessageBox::information(this, "成功", "座位添加成功！", QMessageBox::Ok);
        on_btnRefreshSeat_clicked(); // 刷新表格
        on_btnClearInput_clicked();  // 清空输入
    } else {
        QMessageBox::critical(this, "失败", "添加失败：" + message, QMessageBox::Ok);
    }
}

void AdminWindow::on_cboFlightNum_currentIndexChanged(int index)
{
    // 忽略默认提示项（如"请选择航班"，通常索引为0）
    if (index == 0) {
        on_btnClearInput_clicked();
        // 可选：加载所有航班的座位数据（若需要）
        querySeatDataFromDB("");
        return;
    }

    // 1. 获取选中的航班号（从下拉框的当前数据中获取）
    QString flightNum = ui->cboFlightNum->currentData().toString();

    // 2. 填充文本框 txtFlightNum
    ui->txtFlightNum->setText(flightNum);

    // 3. 自动加载该航班的座位数据到表格
    querySeatDataFromDB(flightNum);
}

void AdminWindow::on_btnUpdateSeat_clicked()
{
    // 1. 获取编辑区数据
    QString flightNum = ui->txtFlightNum->text().trimmed();
    QString seatNum = ui->txtSeatNum->text().trimmed();
    QString seatType = ui->cboSeatType->currentData().toString();
    double seatPrice = ui->spinSeatPrice->value();
    QString seatStatus = ui->cboSeatStatus->currentData().toString();

    // 2. 输入验证
    if (flightNum.isEmpty() || seatNum.isEmpty()) {
        QMessageBox::warning(this, "提示", "航班号和座位号不能为空！");
        return;
    }
    if (seatPrice <= 0) {
        QMessageBox::warning(this, "提示", "座位价格必须大于0！");
        ui->spinSeatPrice->setFocus();
        return;
    }

    // 准备座位数据
    QJsonObject seatData;
    seatData["flight_num"] = flightNum;
    seatData["seat_num"] = seatNum;
    seatData["seat_type"] = seatType;
    seatData["seat_price"] = seatPrice;
    seatData["seat_status"] = seatStatus;

    // 发送更新座位请求
    DBHelper::getInstance()->updateSeat(seatData, [this](bool success, const QString& message) {
        onUpdateSeatResult(success, message);
    });
}

void AdminWindow::onUpdateSeatResult(bool success, const QString& message)
{
    if (success) {
        QMessageBox::information(this, "成功", "座位修改成功！", QMessageBox::Ok);
        on_btnRefreshSeat_clicked(); // 刷新表格
    } else {
        QMessageBox::critical(this, "失败", "修改失败：" + message, QMessageBox::Ok);
    }
}

void AdminWindow::on_btnBatchSet_clicked()
{
    // 1. 检查是否选中行
    QList<QTableWidgetItem*> selectedItems = ui->tableSeat->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先在表格中选择需要批量设置的座位！");
        return;
    }

    // 2. 获取选中的所有座位（去重行）
    QSet<int> selectedRows; // 用Set去重，避免同一行多次处理
    foreach (QTableWidgetItem* item, selectedItems) {
        selectedRows.insert(item->row());
    }
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "提示", "未选中有效座位！");
        return;
    }

    // 3. 弹窗让用户选择目标状态（仅示例"座位状态"，可扩展其他字段）
    QDialog batchDialog(this);
    batchDialog.setWindowTitle("批量设置座位状态");
    batchDialog.resize(300, 150);

    // 弹窗中的下拉框（复用状态选项）
    QComboBox* cboBatchStatus = new QComboBox(&batchDialog);
    cboBatchStatus->addItem("可选", "available");
    cboBatchStatus->addItem("已占", "occupied");
    cboBatchStatus->setCurrentIndex(0);

    // 弹窗中的按钮
    QPushButton* btnConfirm = new QPushButton("确认", &batchDialog);
    QPushButton* btnCancel = new QPushButton("取消", &batchDialog);

    // 布局
    QVBoxLayout* mainLayout = new QVBoxLayout(&batchDialog);
    mainLayout->addWidget(new QLabel("请选择目标座位状态：", &batchDialog));
    mainLayout->addWidget(cboBatchStatus);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(btnConfirm);
    btnLayout->addWidget(btnCancel);
    mainLayout->addLayout(btnLayout);

    // 绑定弹窗按钮事件
    connect(btnConfirm, &QPushButton::clicked, &batchDialog, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, &batchDialog, &QDialog::reject);

    // 显示弹窗，用户选择后继续
    if (batchDialog.exec() != QDialog::Accepted) {
        return; // 用户取消
    }
    QString targetStatus = cboBatchStatus->currentData().toString();

    // 4. 准备批量更新的座位列表
    QJsonArray seatList;
    foreach (int row, selectedRows) {
        // 从表格中获取航班号和座位号
        QString flightNum = ui->tableSeat->item(row, 0)->text();
        QString seatNum = ui->tableSeat->item(row, 1)->text();

        QJsonObject seatInfo;
        seatInfo["flight_num"] = flightNum;
        seatInfo["seat_num"] = seatNum;
        seatList.append(seatInfo);
    }

    // 5. 发送批量更新请求
    DBHelper::getInstance()->batchUpdateSeats(seatList, targetStatus,
                                              [this, selectedRows](bool success, const QString& message) {
                                                  onBatchUpdateSeatsResult(success, message);
                                              });
}

void AdminWindow::onBatchUpdateSeatsResult(bool success, const QString& message)
{
    if (success) {
        QMessageBox::information(this, "批量操作完成", "批量更新成功！", QMessageBox::Ok);
        on_btnRefreshSeat_clicked(); // 刷新表格显示最新状态
    } else {
        QMessageBox::critical(this, "批量操作失败", "批量更新失败：" + message, QMessageBox::Ok);
    }
}


// ========== 订单信息页==========

void AdminWindow::on_btnSelectOrder_clicked()
{
    // 1. 获取筛选条件
    QString name = ui->txtPassengerName->text().trimmed();       // 乘客姓名关键词
    QString idCard = ui->txtIdCard->text().trimmed();             // 身份证关键词
    QString flightNum = ui->txtFlightNumOrder->text().trimmed();  // 航班编号
    QDate startDate = ui->dateStart->date();                      // 开始时间
    QDate endDate = ui->dateEnd->date();                          // 结束时间

    // 关键修复：默认日期（2000/1/1 - 2001/1/1）视为无时间范围
    bool isDefaultDate = (startDate == QDate(2000, 1, 1) && endDate == QDate(2001, 1, 1));
    if (isDefaultDate) {
        startDate = QDate(); // 重置为无效日期（无范围）
        endDate = QDate();
    }

    // 时间范围校验：如果填了一个日期，提示补全
    if ((startDate.isValid() && !endDate.isValid()) || (!startDate.isValid() && endDate.isValid())) {
        QMessageBox::warning(this, "提示", "请同时选择开始时间和结束时间，或都不选！");
        return;
    }
    // 2. 校验时间范围（开始时间不能晚于结束时间）
    if (startDate > endDate) {
        QMessageBox::warning(this, "提示", "开始时间不能晚于结束时间！");
        return;
    }

    // 获取状态筛选（单选框）
    QString statusFilter = "";
    if (ui->rbtnUnpaidOrder->isChecked()) {
        statusFilter = "unpaid";
    } else if (ui->rbtnPaidOrder->isChecked()) {
        statusFilter = "paid";
    } else if (ui->rbtnCancelledOrder->isChecked()) {
        statusFilter = "cancelled";
    }

    // 3. 调用查询函数（带条件）
    queryOrderDataFromDB(name, idCard, flightNum, startDate, endDate, statusFilter);
}


void AdminWindow::on_btnClearOrder_clicked()
{
    // 清空文本输入框
    ui->txtPassengerName->clear();
    ui->txtIdCard->clear();
    ui->txtFlightNumOrder->clear();

    ui->dateStart->setDate(QDate());     // 重置开始日期（无效日期）
    ui->dateEnd->setDate(QDate());       // 重置结束日期（无效日期）

    // 重置单选框为"全部订单"
    ui->rbtnAllOrder->setChecked(true);

    // 3. 显示全部订单（无任何筛选条件）
    queryOrderDataFromDB();  // 所有参数用默认值（无筛选）
}

// 更新订单数量标签
void AdminWindow::updateOrderCount(int count)
{
    ui->lblOrderCount->setText(QString("共 %1 条订单").arg(count));
}

// 在初始化和刷新时调用
void AdminWindow::initOrderManagement()
{
    // ... 其他初始化代码 ...
    // 初始加载全部订单
    on_btnSelectOrder_clicked();
}
void AdminWindow::on_btnRefreshOrder_clicked()
{
    on_btnSelectOrder_clicked();
}


void AdminWindow::on_rbtnAllOrder_clicked()
{
    // 无状态筛选，直接用当前页面的条件查询
    filterOrderByStatus("");
}


void AdminWindow::on_rbtnUnpaidOrder_clicked()
{
    filterOrderByStatus("unpaid");
}


void AdminWindow::on_rbtnPaidOrder_clicked()
{
    filterOrderByStatus("paid");
}


void AdminWindow::on_rbtnCancelledOrder_clicked()
{
    filterOrderByStatus("cancelled");
}

void AdminWindow::filterOrderByStatus(const QString& status)
{
    QString name = ui->txtPassengerName->text().trimmed();
    QString idCard = ui->txtIdCard->text().trimmed();
    QString flightNum = ui->txtFlightNumOrder->text().trimmed();

    // 关键：判断用户是否手动选择了时间范围（若为默认初始值，视为无范围）
    QDate startDate = ui->dateStart->date();
    QDate endDate = ui->dateEnd->date();

    bool isDefaultDate = (startDate == QDate(2000, 1, 1) && endDate == QDate(2001, 1, 1));
    if (isDefaultDate) {
        startDate = QDate(); // 重置为无效日期（无范围）
        endDate = QDate();
    }

    // 调用查询（带状态和有效条件）
    queryOrderDataFromDB(name, idCard, flightNum, startDate, endDate, status);
}


void AdminWindow::on_actionInfo_triggered()
{
    // 这里需要从服务器获取管理员信息，但目前服务器没有提供这个API
    // 暂时使用本地存储的信息
    QString info = QString("管理员账号：%1\n").arg(m_currentAdminAccount)
                   + QString("中文姓名：系统管理员\n")
                   + QString("注册时间：系统启动时间\n");

    QMessageBox::information(this, "个人信息", info, QMessageBox::Ok);
}

void AdminWindow::on_actionPwdChange_triggered()
{
    // 弹出密码修改对话框，并传入当前登录的管理员账号
    AdminEditPwd pwdDialog(m_currentAdminAccount, this);

    // 显示对话框，等待用户操作
    if (pwdDialog.exec() == QDialog::Accepted) {
        // 如果用户成功修改密码，关闭主窗口并跳转到登录界面
        this->close();
        login* loginWin = new login();
        loginWin->show();
    }
    // 如果用户取消，则什么都不做
}


void AdminWindow::on_actionLoginExit_triggered()
{
    if (QMessageBox::question(this, "确认切换", "是否要切换账号？",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // 关闭当前主窗口，打开登录页

    login* loginWin = new login();
    loginWin->show();
    this->hide();
}


void AdminWindow::on_actionSystemExit_triggered()
{
    if (QMessageBox::question(this, "确认退出", "是否要退出航班票务管理系统？",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // 退出应用程序
    qApp->quit();
}


void AdminWindow::on_actionAbout_triggered()
{
    QString aboutInfo = "航班票务管理系统 v1.0\n"
                        "开发框架：Qt 5.x + HTTP Server\n"
                        "功能说明：用于管理员管理航班、座位、用户及订单信息\n"
                        "数据存储：内存数据结构\n"
                        "版权所有：© 2025 航班票务管理系统开发团队";

    QMessageBox::about(this, "关于系统", aboutInfo);
}


void AdminWindow::on_actionHelpDoc_triggered()
{
    QString helpInfo = "航班票务管理系统 - 管理员操作指南\n\n"
                       "【核心功能】\n"
                       "1. 航班管理：新增/修改/删除航班，支持按航班号筛选、多维度排序，可设置登机口\n"
                       "2. 座位管理：按航班筛选座位，支持新增/修改座位，可批量设置座位状态/舱位类型\n"
                       "3. 用户管理：查看用户列表，重置密码（默认123456）、封禁/解封用户账号\n"
                       "4. 订单管理：多条件筛选订单（姓名/身份证/航班/时间/支付状态），自动统计订单数量\n"
                       "5. 投诉建议：查看用户提交的投诉/建议，含提交时间、内容等完整信息\n"
                       "6. 餐食订单：查看用户预定的航班餐食信息，含航班号、餐食类型等\n\n"
                       "【操作注意事项】\n"
                       "航班操作：新增/修改航班时，系统会校验航班号唯一性，请勿重复添加\n"
                       "座位操作：批量设置仅支持修改舱位类型/状态，座位号不可批量修改，价格需大于0\n"
                       "用户操作：重置密码、封禁/解封用户需谨慎\n"
                       "通用规则：所有编辑/删除操作需先选中表格行，新增除外；时间格式统一为yyyy-MM-dd HH:mm\n\n"
                       "【管理员中心】\n"
                       "- 修改密码：需验证原密码，修改后需重新登录\n"
                       "- 切换账号：确认后返回登录页；\n退出系统：直接关闭整个应用程序";

    QMessageBox::information(this, "帮助文档", helpInfo, QMessageBox::Ok);
}


void AdminWindow::on_refreshmealBtn_clicked()
{
    queryMealOrdersFromDB();
}


void AdminWindow::on_refreshComplainsBtn_clicked()
{
    queryComplaintsFromDB();

}
// 初始化支付信息表格列头、列宽
void AdminWindow::initPayInfoTable()
{
    QStringList headers = {"手机号", "真实姓名", "身份证号", "支付方式", "支付密码", "提交时间"};
    ui->payInfoTable->setColumnCount(headers.size());
    ui->payInfoTable->setHorizontalHeaderLabels(headers);

    // 列宽
    ui->payInfoTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->payInfoTable->setColumnWidth(0, 150);  // 手机号
    ui->payInfoTable->setColumnWidth(1, 100);  // 姓名
    ui->payInfoTable->setColumnWidth(2, 180);  // 身份证
    ui->payInfoTable->setColumnWidth(3,  80);  // 支付方式
    ui->payInfoTable->setColumnWidth(4, 100);  // 密码（****）
    ui->payInfoTable->setColumnWidth(5, 160);  // 提交时间
    ui->payInfoTable->horizontalHeader()->setMinimumSectionSize(60);
}

// 从服务器拉取支付信息
void AdminWindow::queryPayInfoFromDB()
{
    DBHelper::getInstance()->sendRequest("getAllUserPayInfos", {},
                                         [this](const QJsonObject &resp){
                                             if (!resp["success"].toBool()) return;
                                             QJsonArray arr = resp["data"].toArray();
                                             ui->payInfoTable->setRowCount(0);
                                             for (int i = 0; i < arr.size(); ++i) {
                                                 QJsonObject o = arr[i].toObject();
                                                 ui->payInfoTable->insertRow(i);
                                                 ui->payInfoTable->setItem(i, 0, new QTableWidgetItem(o["user_phone"].toString()));
                                                 ui->payInfoTable->setItem(i, 1, new QTableWidgetItem(o["real_name"].toString()));
                                                 ui->payInfoTable->setItem(i, 2, new QTableWidgetItem(o["id_card"].toString()));
                                                 ui->payInfoTable->setItem(i, 3, new QTableWidgetItem(o["pay_method"].toString()));
                                                 ui->payInfoTable->setItem(i, 4, new QTableWidgetItem(o["pay_password"].toString())); // 已是****
                                                 ui->payInfoTable->setItem(i, 5, new QTableWidgetItem(o["create_time"].toString()));
                                             }
                                         });
}


void AdminWindow::on_btnRefreshrebook_clicked()
{
    // 从服务器获取所有退改申请
    DBHelper::getInstance()->getAllModifyApplications([this](bool success, const QJsonArray& applies) {
        onRebookApplyResult(success, applies);
    });
    filterRebookApplications();
}

void AdminWindow::onRebookApplyResult(bool success, const QJsonArray& applies)
{
    if (success) {
        loadRebookDataToTable(applies);
    } else {
        QMessageBox::critical(this, "错误", "获取退改申请失败！", QMessageBox::Ok);
    }
}

void AdminWindow::loadRebookDataToTable(const QJsonArray& applies)
{
    // 清空表格
    ui->rebookTable->setRowCount(0);

    // 填充数据
    for (int i = 0; i < applies.size(); ++i) {
        QJsonObject apply = applies[i].toObject();
        ui->rebookTable->insertRow(i);

        // 第0列：退改ID（新增列）
        ui->rebookTable->setItem(i, 0, new QTableWidgetItem(apply["apply_id"].toString()));
        // 第1列：乘客姓名（原0列后移）
        ui->rebookTable->setItem(i, 1, new QTableWidgetItem(apply["passenger_name"].toString()));
        // 第2列：身份证号（原1列后移）
        ui->rebookTable->setItem(i, 2, new QTableWidgetItem(apply["id_card"].toString()));
        // 第3列：退改类型（转换为中文显示，原2列后移）
        QString type = apply["apply_type"].toString() == "refund" ? "退票" : "改签";
        ui->rebookTable->setItem(i, 3, new QTableWidgetItem(type));
        // 第4列：原航班号（原3列后移）
        ui->rebookTable->setItem(i, 4, new QTableWidgetItem(apply["old_flight_num"].toString()));
        // 第5列：目标航班（退票时为空，原4列后移）
        ui->rebookTable->setItem(i, 5, new QTableWidgetItem(apply["new_flight_num"].toString()));
        // 第6列：申请时间（原5列后移）
        ui->rebookTable->setItem(i, 6, new QTableWidgetItem(apply["apply_time"].toString()));
        // 第7列：申请状态（转换为中文显示，原6列后移）
        QString statusText;
        if (apply["status"].toString() == "pending") statusText = "待处理";
        else if (apply["status"].toString() == "approved") statusText = "已通过";
        else if (apply["status"].toString() == "rejected") statusText = "已拒绝";
        ui->rebookTable->setItem(i, 7, new QTableWidgetItem(statusText));
    }

    // 调整列宽
    ui->rebookTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->rebookTable->setColumnWidth(0, 80);  // 退改ID
    ui->rebookTable->setColumnWidth(1, 150);  // 乘客姓名
    ui->rebookTable->setColumnWidth(2, 260);  // 身份证号
    ui->rebookTable->setColumnWidth(3, 100);   //退改类型
    ui->rebookTable->setColumnWidth(4, 140);  // 原航班号
    ui->rebookTable->setColumnWidth(5, 140);  // 目标航班
    ui->rebookTable->setColumnWidth(6, 240);   // 申请时间
    ui->rebookTable->setColumnWidth(7, 80);  // 申请状态
    ui->rebookTable->horizontalHeader()->setMinimumSectionSize(60);
}


void AdminWindow::on_btnApprove_clicked()
{
    // 获取选中的行
    int currentRow = ui->rebookTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请选择需要处理的申请！", QMessageBox::Ok);
        return;
    }

    // 从第0列获取退改ID（核心修改）
    QString applyId = ui->rebookTable->item(currentRow, 0)->text();
    if (applyId.isEmpty()) {
        QMessageBox::warning(this, "错误", "选中的申请ID为空！", QMessageBox::Ok);
        return;
    }

    // 后续审核逻辑不变（弹出对话框选择通过/拒绝 + 填写原因）
    QDialog dialog(this);
    dialog.setWindowTitle("处理申请");
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QRadioButton* rbApprove = new QRadioButton("通过", &dialog);
    QRadioButton* rbReject = new QRadioButton("拒绝", &dialog);
    rbApprove->setChecked(true);

    QLabel* lblReason = new QLabel("处理原因：", &dialog);
    QLineEdit* edtReason = new QLineEdit(&dialog);

    layout->addWidget(rbApprove);
    layout->addWidget(rbReject);
    layout->addWidget(lblReason);
    layout->addWidget(edtReason);

    QDialogButtonBox* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString status = rbApprove->isChecked() ? "approved" : "rejected";
    QString reason = edtReason->text().trimmed();

    // 调用处理接口
    DBHelper::getInstance()->processModifyApplication(applyId, status, reason, [this](bool success, const QString& message) {
        if (success) {
            QMessageBox::information(this, "成功", "申请处理成功！");
            on_btnRefreshrebook_clicked(); // 刷新表格
        } else {
            QMessageBox::critical(this, "失败", "处理失败：" + message);
        }
    });
}

void AdminWindow::onProcessApplyResult(bool success, const QString& message)
{
    if (success) {
        QMessageBox::information(this, "成功", "申请处理成功！", QMessageBox::Ok);
        on_btnRefreshrebook_clicked(); // 刷新表格
    } else {
        QMessageBox::critical(this, "失败", "处理失败：" + message, QMessageBox::Ok);
    }
}


void AdminWindow::on_btnReject_clicked()
{
    // 1. 获取选中行
    int currentRow = ui->rebookTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "操作提示", "请先选中需要拒绝的退改申请！", QMessageBox::Ok);
        return;
    }

    // 2. 获取选中行的退改ID（第0列）
    QTableWidgetItem* idItem = ui->rebookTable->item(currentRow, 0);
    if (!idItem || idItem->text().isEmpty()) {
        QMessageBox::critical(this, "错误", "选中的申请ID为空，请刷新后重试！", QMessageBox::Ok);
        return;
    }
    QString applyId = idItem->text();

    // 3. 弹出拒绝原因输入框（必填）
    bool isOk;
    QString rejectReason = QInputDialog::getText(this,
                                                 "拒绝申请",
                                                 "请输入拒绝原因（必填）：",
                                                 QLineEdit::Normal,
                                                 "",
                                                 &isOk);

    // 4. 判断用户是否取消输入
    if (!isOk || rejectReason.trimmed().isEmpty()) {
        if (!isOk) {
            QMessageBox::information(this, "提示", "已取消拒绝操作");
        } else {
            QMessageBox::warning(this, "提示", "拒绝原因不能为空！");
        }
        return;
    }

    // 5. 二次确认是否拒绝
    int confirm = QMessageBox::question(this,
                                        "确认拒绝",
                                        QString("是否确定拒绝退改申请【%1】？\n拒绝原因：%2").arg(applyId).arg(rejectReason),
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);
    if (confirm != QMessageBox::Yes) {
        return;
    }

    // 6. 调用DBHelper提交拒绝请求
    DBHelper::getInstance()->processModifyApplication(applyId, "rejected", rejectReason, [this](bool success, const QString& message) {
        if (success) {
            QMessageBox::information(this, "成功", "退改申请已成功拒绝！");
            // 刷新表格显示最新状态
            on_btnRefreshrebook_clicked();
        } else {
            QMessageBox::critical(this, "失败", "拒绝申请失败：" + message, QMessageBox::Ok);
        }
    });
}

// // 根据筛选条件查询退改申请
// void AdminWindow::filterRebookApplications()
// {
//     // 构造筛选参数
//     QJsonObject filters;
//     if (m_rebookFilterType != "all") {
//         filters["apply_type"] = m_rebookFilterType; // 筛选退票/改签
//     }
//     if (m_rebookFilterStatus != "all") {
//         filters["status"] = m_rebookFilterStatus; // 筛选未处理/已处理
//     }

//     // 调用DBHelper查询符合条件的退改申请
//     DBHelper::getInstance()->queryModifyApplications(filters, [this](bool success, const QJsonArray& applies) {
//         if (success) {
//             loadRebookDataToTable(applies); // 加载筛选后的结果到表格
//         } else {
//             QMessageBox::critical(this, "筛选失败", "获取退改申请列表失败！", QMessageBox::Ok);
//         }
//     });
// }

// 根据筛选条件查询退改申请（简化版：前端全量过滤）
void AdminWindow::filterRebookApplications()
{
    DBHelper::getInstance()->getAllModifyApplications([this](bool success, const QJsonArray& allApplies) {
        if (!success) {
            QMessageBox::critical(this, "筛选失败", "获取退改申请列表失败！", QMessageBox::Ok);
            return;
        }

        // 前端本地过滤（类型+状态，包括processed聚合）
        QJsonArray filteredApplies;
        for (const QJsonValue& val : allApplies) {
            QJsonObject apply = val.toObject();
            QString applyType = apply["apply_type"].toString(); // refund/rebook
            QString status = apply["status"].toString();       // pending/approved/rejected

            // 过滤1：申请类型（all则不过滤）
            if (m_rebookFilterType != "all" && applyType != m_rebookFilterType) {
                continue;
            }

            // 过滤2：状态（核心：处理processed聚合）
            if (m_rebookFilterStatus == "all") {
                filteredApplies.append(apply);
            } else if (m_rebookFilterStatus == "pending") {
                if (status == "pending") filteredApplies.append(apply);
            } else if (m_rebookFilterStatus == "processed") {
                if (status == "approved" || status == "rejected") {
                    filteredApplies.append(apply);
                }
            }
        }

        // 加载过滤后的结果到表格
        loadRebookDataToTable(filteredApplies);
    });
}

void AdminWindow::on_rbtnAllApply_clicked()
{
    m_rebookFilterType = "all";
    m_rebookFilterStatus = "all";
    filterRebookApplications();
}


void AdminWindow::on_rbtnRefund_clicked()
{
    m_rebookFilterType = "refund";
    filterRebookApplications();
}


void AdminWindow::on_rbtnRebook_clicked()
{
    m_rebookFilterType = "rebook";
    filterRebookApplications();
}


void AdminWindow::on_rbtnpending_clicked()
{
    m_rebookFilterStatus = "pending";
    filterRebookApplications();
}


void AdminWindow::on_rbtnProcessed_clicked()
{
    m_rebookFilterStatus = "processed";
    filterRebookApplications();
}



void AdminWindow::on_btnDeleteSeat_clicked()
{
    // 1. 检查是否有选中的座位行
    int currentRow = ui->tableSeat->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "操作提示", "请先选中需要删除的座位！", QMessageBox::Ok);
        return;
    }

    // 2. 获取选中座位的航班号和座位号
    QString flightNum = ui->tableSeat->item(currentRow, 0)->text();
    QString seatNum = ui->tableSeat->item(currentRow, 1)->text();

    if (flightNum.isEmpty() || seatNum.isEmpty()) {
        QMessageBox::critical(this, "数据错误", "选中的座位信息不完整，无法删除！", QMessageBox::Ok);
        return;
    }

    // 3. 二次确认删除操作
    int confirm = QMessageBox::question(this,
                                        "确认删除",
                                        QString("确定要删除航班 %1 的 %2 号座位吗？\n此操作不可恢复！")
                                            .arg(flightNum).arg(seatNum),
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);
    if (confirm != QMessageBox::Yes) {
        return; // 用户取消删除
    }

    // 4. 构造删除请求数据
    QJsonObject seatData;
    seatData["flight_num"] = flightNum;
    seatData["seat_num"] = seatNum;

    // 5. 调用DBHelper发送删除请求（假设后端提供deleteSeat接口）
    DBHelper::getInstance()->sendRequest("deleteSeat", seatData, [this, flightNum](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();

        if (success) {
            QMessageBox::information(this, "删除成功", "座位已成功删除！", QMessageBox::Ok);
            // 刷新当前航班的座位数据
            querySeatDataFromDB(flightNum);
            // 清空输入框
            on_btnClearInput_clicked();
        } else {
            QMessageBox::critical(this, "删除失败", "删除座位失败：" + message, QMessageBox::Ok);
        }
    });
}

