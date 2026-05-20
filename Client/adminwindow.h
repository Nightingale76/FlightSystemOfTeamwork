#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>
#include <QVariantList>
#include <QTableWidgetItem>
#include <QVariantMap>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>

// #include <QLineEdit>   // 登机口输入框
// #include <QPushButton> // 登机口修改按钮
// #include <QLabel>      // 标签控件
// #include <QHBoxLayout> // 布局控件
// #include <QVBoxLayout> // 布局控件

QT_BEGIN_NAMESPACE
namespace Ui {
class AdminWindow;
}
QT_END_NAMESPACE

class AdminWindow : public QMainWindow
{
    Q_OBJECT

public:
    AdminWindow(const QString& adminAccount, QWidget *parent = nullptr);
    ~AdminWindow();

private slots:
    void on_addFlightBtn_clicked();
    void on_updateFlightBtn_clicked();
    void on_deleteFlightBtn_clicked();
    void on_refreshFlightsBtn_clicked();

    void on_refreshUsersBtn_clicked();

    void on_flightTable_itemClicked(QTableWidgetItem *item);

    void on_btnClearFlight_clicked();

    void on_tableSeat_itemSelectionChanged();

    void on_btnClearInput_clicked();

    void on_btnRefreshSeat_clicked();

    void on_btnAddSeat_clicked();

    void on_cboFlightNum_currentIndexChanged(int index);

    void on_btnUpdateSeat_clicked();

    void on_btnBatchSet_clicked();

    void on_btnSelectOrder_clicked();

    void on_btnClearOrder_clicked();

    void on_btnRefreshOrder_clicked();

    void on_rbtnAllOrder_clicked();

    void on_rbtnUnpaidOrder_clicked();

    void on_rbtnPaidOrder_clicked();

    void on_rbtnCancelledOrder_clicked();

    void on_actionInfo_triggered();

    void on_actionPwdChange_triggered();

    void on_actionLoginExit_triggered();

    void on_actionSystemExit_triggered();

    void on_actionAbout_triggered();

    void on_actionHelpDoc_triggered();

    /* ---------- private slots: 里追加 ---------- */
    void queryComplaintsFromDB();
    void queryMealOrdersFromDB();

    void on_refreshmealBtn_clicked();

    void on_refreshComplainsBtn_clicked();

    void on_sortComboBox_currentIndexChanged(int index);

    void on_searchBtn_clicked();

    void on_rstUserPwdBtn_clicked();

    void on_banUserBtn_clicked();

    void on_btnRefreshrebook_clicked();

    void on_btnApprove_clicked();

    void on_btnReject_clicked();

    void on_rbtnAllApply_clicked();

    void on_rbtnRefund_clicked();

    void on_rbtnRebook_clicked();

    void on_rbtnpending_clicked();

    void on_rbtnProcessed_clicked();

    void filterRebookApplications();


    void on_btnDeleteSeat_clicked();

private:
    Ui::AdminWindow *ui;

    void initPayInfoTable();
    void queryPayInfoFromDB();

    //---退改信息---
    void loadRebookDataToTable(const QJsonArray& applies);
    void onRebookApplyResult(bool success, const QJsonArray& applies);
    void onProcessApplyResult(bool success, const QString& message);



    QString m_currentAdminAccount; // 存储当前登录的管理员账号
    // 新增：排序/搜索相关变量
    QString m_flightSearchNum;  // 航班编号搜索内容
    QString m_sortField;        // 排序字段（默认create_time）

    // 退改申请筛选条件
    QString m_rebookFilterType;  // 退改类型筛选（refund/rebook/all）
    QString m_rebookFilterStatus; // 状态筛选（pending/processed/all）

    /* ---------- private: 里追加 ---------- */
    void loadComplaintsToTable(const QJsonArray& complaints);
    void loadMealOrdersToTable(const QJsonArray& orders);


    // ========== 航班信息页==========
    void queryFlightDataFromDB(); // 查询航班数据
    void loadFlightDataToTable(const QJsonArray& flights);         // 填充航班表格
    // ========== 座位信息页 ==========
    void querySeatDataFromDB(const QString& flightNum = "");    // 查询座位数据（支持按航班号筛选）
    void loadSeatDataToTable(const QJsonArray& seats); // 填充座位表格
    // ========== 用户信息页 ==========
    void queryUserDataFromDB();  // 查询用户数据
    void loadUserDataToTable(const QJsonArray& users);          // 填充用户表格
    // ========== 订单信息页==========
    void queryOrderDataFromDB(
        const QString& name = "",
        const QString& idCard = "",
        const QString& flightNum = "",
        QDate startDate = QDate(),  // 默认无效日期（表示无范围）
        QDate endDate = QDate(),    // 默认无效日期（表示无范围）
        const QString& statusFilter = ""  // 状态筛选（默认空：不筛选）
        );
    void loadOrderDataToTable(const QJsonArray& orders); // 填充订单表格

    void initSeatManagement();
    void loadFlightNumbersToCombo();

    void updateOrderCount(int count);
    void initOrderManagement();
    void filterOrderByStatus(const QString& status);
    void updateFlightData(const QString& oldFlightNum,
                          const QString& newFlightNum,
                          const QString& fromCity,
                          const QString& toCity,
                          const QString& departTime,
                          const QString& arriveTime,
                          const QString& price,
                          const QString& airline,
                          const QString& gate);

    // HTTP请求回调函数
    void onFlightsQueryResult(bool success, const QJsonArray& flights);
    void onUsersQueryResult(bool success, const QJsonArray& users);
    void onSeatsQueryResult(bool success, const QJsonArray& seats);
    void onOrdersQueryResult(bool success, const QJsonArray& orders);
    void onAddFlightResult(bool success, const QString& message);
    void onUpdateFlightResult(bool success, const QString& message);
    void onDeleteFlightResult(bool success, const QString& message);
    void onAddSeatResult(bool success, const QString& message);
    void onUpdateSeatResult(bool success, const QString& message);
    void onBatchUpdateSeatsResult(bool success, const QString& message);

};
#endif // ADMINWINDOW_H
