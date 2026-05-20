#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>  // 添加这行以解决QSqlRecord不完整类型问题
#include <QDateTime>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>

class DBManager : public QObject
{
    Q_OBJECT

public:
    static DBManager& getInstance();
    QSqlDatabase threadDB();

    // 数据库连接
    bool connectToDatabase();
    void closeDatabase();

    // 用户相关操作
    bool addUser(const QString& phone, const QString& password, const QString& nickname);
    bool getUserByPhone(const QString& phone, QJsonObject& userInfo);
    bool validateUser(const QString& phone, const QString& password, QJsonObject& userInfo);
    bool updateUserPassword(const QString& phone, const QString& newPassword);
    QJsonArray getAllUsers();

    // 管理员相关操作
    bool validateAdmin(const QString& account, const QString& password, QJsonObject& adminInfo);

    // 航班相关操作
    bool addFlight(const QJsonObject& flightData);
    bool updateFlight(const QJsonObject& flightData);
    bool deleteFlight(const QString& flightNum);
    QJsonArray getFlightByNumber(const QString& flightNum);
    QJsonArray queryFlights(const QString& departure, const QString& arrival, const QString& date);
    QJsonArray getAllFlights();
    QString getFlightGate(const QString& flightNum);
    bool updateFlightGate(const QString& flightNum, const QString& gate);

    // 座位相关操作
    bool addSeat(const QJsonObject& seatData);
    bool updateSeat(const QJsonObject& seatData);
    QJsonArray getSeatsByFlight(const QString& flightNum);
    QJsonArray getAllSeats();
    bool updateSeatStatus(const QString& flightNum, const QString& seatNum, const QString& status);
    bool batchUpdateSeats(const QJsonArray& seatList, const QString& status);

    // 订单相关操作
    bool createOrder(const QJsonObject& orderData);
    QJsonArray getOrdersByUserPhone(const QString& phone);
    QJsonArray getOrdersByFilters(const QJsonObject& filters);
    QJsonArray getAllOrders();
    bool updateOrderStatus(const QString& orderId, const QString& status);

    // 投诉建议
    bool addComplaint(const QJsonObject& complaintData);
    QJsonArray getAllComplaints();

    // 餐食订单
    bool addMealOrder(const QJsonObject& mealData);
    QJsonArray getAllMealOrders();

private:
    DBManager(QObject* parent = nullptr);
    ~DBManager();


    QString m_lastError;

    // 辅助方法
    QJsonObject flightRecordToJson(const QSqlRecord& record);
    QJsonObject userRecordToJson(const QSqlRecord& record);
    QJsonObject seatRecordToJson(const QSqlRecord& record);
    QJsonObject orderRecordToJson(const QSqlRecord& record);
};

#endif // DBMANAGER_H
