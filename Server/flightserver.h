#ifndef FLIGHTSERVER_H
#define FLIGHTSERVER_H

#include <QTcpServer>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QList>
#include <QDateTime>
#include <QTimer>
#include "dbmanager.h"  // 替换原有的datapersistence.h

class Clientthread;

class FlightServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit FlightServer(QObject *parent = nullptr);
    ~FlightServer();

    bool startServer(quint16 port);
    QString getLocalIP();

    // 连接数据库
    bool initDatabase();

    // 投诉建议
    bool submitComplaint(const QJsonObject& data);
    bool submitMealOrder(const QJsonObject& data);
    QJsonArray getAllComplaints();
    QJsonArray getAllMealOrders();

    // 管理员相关
    bool adminLogin(const QString &username, const QString &password, QJsonObject &adminInfo);

    // 用户相关
    bool userLogin(const QString &phone, const QString &password, QJsonObject &userInfo);
    bool userRegister(const QString &phone, const QString &password, const QString &nickname = "");
    bool userResetPassword(const QString &phone, const QString &newPassword);

    // 航班查询
    QJsonArray queryFlightsForUser(const QString &departure, const QString &arrival, const QString &date);
    QJsonArray queryFlights(const QString &from, const QString &to, const QString &date);
    QJsonArray getAllFlights();

    // 数据查询
    QJsonArray getAllUsers();
    QJsonArray getSeats(const QString &flightNum = "");
    QJsonArray getOrders(const QJsonObject &filters);
    QJsonArray getUserOrders(int userId);
    QJsonArray getUserOrdersByPhone(const QString &phone);

    // 登机口管理
    QString getFlightGate(const QString& flight_num);
    bool updateFlightGate(const QString& flight_num, const QString& newGate);
    QString getFlightDelayStatus(const QString& flight_num);

    // 数据操作
    bool addFlight(const QJsonObject &flightData);
    bool updateFlight(const QJsonObject &flightData);
    bool deleteFlight(const QString &flightNum);
    bool addSeat(const QJsonObject &seatData);
    bool updateSeat(const QJsonObject &seatData);
    bool batchUpdateSeats(const QJsonArray &seatList, const QString &status);

    // 订单相关
    bool createUserOrder(const QJsonObject &orderData);
    bool createOrder(int userId, int flightId, const QString &passengerName,
                     const QString &passengerId, const QString &seatNum = "");
    bool cancelOrder(int orderId);
    bool selectSeat(const QString &flightNum, const QString &seatNum, const QString &passengerPhone);

    // 支付和天气
    QJsonObject createPayment(int orderId, const QString &paymentMethod);
    bool processPaymentCallback(const QString &transactionId, const QString &paymentMethod);
    QJsonObject getWeather(const QString &city);

private:
    // 初始化示例数据（如果需要）
    void initializeSampleData();

    // 数据库管理器
    DBManager& m_dbManager;

    QList<QThread*> m_clients;

    QTimer* m_autoSaveTimer;

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};

#endif // FLIGHTSERVER_H
