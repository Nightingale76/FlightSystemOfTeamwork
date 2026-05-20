#include "flightserver.h"
#include "clientthread.h"
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QThread>
#include <QTimer>
#include <QNetworkInterface>

FlightServer::FlightServer(QObject *parent)
    : QTcpServer(parent)
    , m_dbManager(DBManager::getInstance())
{

    qDebug() << "服务器启动，使用 MySQL(ODBC)";
}

FlightServer::~FlightServer()
{
    if (m_autoSaveTimer) {
        m_autoSaveTimer->stop();
    }
    qDebug() << "服务器关闭";
}

bool FlightServer::initDatabase()
{
    // 不再提前连接，让每条线程第一次请求时自己建连接
    qDebug() << "数据库连接改为线程级延迟建立";
    return true;
}bool FlightServer::startServer(quint16 port)
{
    if (!listen(QHostAddress::Any, port)) {
        qDebug() << "服务器启动失败:" << errorString();
        return false;
    }

    qDebug() << "服务器启动成功，端口:" << serverPort();
    qDebug() << "服务器IP地址: " << getLocalIP();
    return true;
}

QString FlightServer::getLocalIP()
{
    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

    for (const QHostAddress &address : ipAddressesList) {
        if (address != QHostAddress::LocalHost && address.toIPv4Address()) {
            if (address.toString().startsWith("192.168.") ||
                address.toString().startsWith("10.") ||
                address.toString().startsWith("172.")) {
                ipAddress = address.toString();
                break;
            }
        }
    }

    if (ipAddress.isEmpty()) {
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    }
    return ipAddress;
}

// 用户相关方法
bool FlightServer::userLogin(const QString &phone, const QString &password, QJsonObject &userInfo)
{
    // 密码需要在前端进行MD5加密，或在这里加密
    QString md5Pwd = QString(QCryptographicHash::hash(password.toUtf8(),
                                                      QCryptographicHash::Md5).toHex());
    return m_dbManager.validateUser(phone, md5Pwd, userInfo);
}

bool FlightServer::userRegister(const QString &phone, const QString &password, const QString &nickname)
{
    QString md5Pwd = QString(QCryptographicHash::hash(password.toUtf8(),
                                                      QCryptographicHash::Md5).toHex());
    return m_dbManager.addUser(phone, md5Pwd, nickname);
}

bool FlightServer::userResetPassword(const QString &phone, const QString &newPassword)
{
    QString md5Pwd = QString(QCryptographicHash::hash(newPassword.toUtf8(),
                                                      QCryptographicHash::Md5).toHex());
    return m_dbManager.updateUserPassword(phone, md5Pwd);
}

// 管理员登录
bool FlightServer::adminLogin(const QString &username, const QString &password, QJsonObject &adminInfo)
{
    QString md5Pwd = QString(QCryptographicHash::hash(password.toUtf8(),
                                                      QCryptographicHash::Md5).toHex());
    return m_dbManager.validateAdmin(username, md5Pwd, adminInfo);
}

// 航班查询
QJsonArray FlightServer::queryFlightsForUser(const QString &departure, const QString &arrival, const QString &date)
{
    return m_dbManager.queryFlights(departure, arrival, date);
}

QJsonArray FlightServer::getAllFlights()
{
    return m_dbManager.getAllFlights();
}

QJsonArray FlightServer::getAllUsers()
{
    return m_dbManager.getAllUsers();
}

QJsonArray FlightServer::getSeats(const QString &flightNum)
{
    return m_dbManager.getSeatsByFlight(flightNum);
}

QJsonArray FlightServer::getUserOrdersByPhone(const QString &phone)
{
    return m_dbManager.getOrdersByUserPhone(phone);
}

QJsonArray FlightServer::getOrders(const QJsonObject &filters)
{
    // 这里可以根据filters构建更复杂的查询
    // 简化处理：返回所有订单
    return m_dbManager.getAllOrders();
}

// 创建订单
bool FlightServer::createUserOrder(const QJsonObject &orderData)
{
    return m_dbManager.createOrder(orderData);
}

// 选座
bool FlightServer::selectSeat(const QString &flightNum, const QString &seatNum, const QString &passengerPhone)
{
    return m_dbManager.updateSeatStatus(flightNum, seatNum, "occupied");
}

// 投诉建议
bool FlightServer::submitComplaint(const QJsonObject& data)
{
    return m_dbManager.addComplaint(data);
}

QJsonArray FlightServer::getAllComplaints()
{
    return m_dbManager.getAllComplaints();
}

// 餐食订单
bool FlightServer::submitMealOrder(const QJsonObject& data)
{
    return m_dbManager.addMealOrder(data);
}

QJsonArray FlightServer::getAllMealOrders()
{
    return m_dbManager.getAllMealOrders();
}

// 登机口管理
QString FlightServer::getFlightGate(const QString& flight_num)
{
    return m_dbManager.getFlightGate(flight_num);
}

bool FlightServer::updateFlightGate(const QString& flight_num, const QString& newGate)
{
    return m_dbManager.updateFlightGate(flight_num, newGate);
}

QString FlightServer::getFlightDelayStatus(const QString& flight_num)
{
    // 这里可以从数据库查询实际延误状态
    // 示例：模拟逻辑
    if (flight_num.contains("CA", Qt::CaseInsensitive)) {
        return "延误";
    }
    return "正常";
}

// 航班管理
bool FlightServer::addFlight(const QJsonObject &flightData)
{
    return m_dbManager.addFlight(flightData);
}

bool FlightServer::updateFlight(const QJsonObject &flightData)
{
    return m_dbManager.updateFlight(flightData);
}

bool FlightServer::deleteFlight(const QString &flightNum)
{
    // 需要先删除相关座位
    return m_dbManager.deleteFlight(flightNum);
}

// 座位管理
bool FlightServer::addSeat(const QJsonObject &seatData)
{
    return m_dbManager.addSeat(seatData);
}

bool FlightServer::updateSeat(const QJsonObject &seatData)
{
    return m_dbManager.updateSeat(seatData);
}

bool FlightServer::batchUpdateSeats(const QJsonArray &seatList, const QString &status)
{
    // 批量更新座位状态
    bool success = true;
    for (const QJsonValue& value : seatList) {
        QJsonObject seat = value.toObject();
        if (!m_dbManager.updateSeatStatus(seat["flight_num"].toString(),
                                          seat["seat_num"].toString(),
                                          status)) {
            success = false;
        }
    }
    return success;
}

// 其他方法（保持原逻辑或简化）
bool FlightServer::createOrder(int userId, int flightId, const QString &passengerName,
                               const QString &passengerId, const QString &seatNum)
{
    // 简化实现
    QJsonObject orderData;
    orderData["user_id"] = userId;
    orderData["flight_id"] = flightId;
    orderData["passenger_name"] = passengerName;
    orderData["passenger_idcard"] = passengerId;
    orderData["seat_num"] = seatNum;

    // 需要补充其他必要字段
    return false; // 需要完整实现
}

bool FlightServer::cancelOrder(int orderId)
{
    return m_dbManager.updateOrderStatus(QString::number(orderId), "cancelled");
}

QJsonObject FlightServer::createPayment(int orderId, const QString &paymentMethod)
{
    QJsonObject response;
    response["success"] = true;
    response["payment_id"] = orderId;
    response["amount"] = 100.0;
    response["qr_code"] = "https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=payment";
    response["order_id"] = orderId;
    response["payment_method"] = paymentMethod;
    response["message"] = "支付创建成功";
    return response;
}

QJsonObject FlightServer::getWeather(const QString &city)
{
    QJsonObject weatherInfo;
    QMap<QString, QPair<QString, int>> cityWeather;
    cityWeather["北京"] = qMakePair(QString("晴朗"), 25);
    cityWeather["上海"] = qMakePair(QString("多云"), 23);
    cityWeather["广州"] = qMakePair(QString("阵雨"), 28);

    if (cityWeather.contains(city)) {
        auto weatherData = cityWeather[city];
        weatherInfo["city"] = city;
        weatherInfo["temperature"] = weatherData.second;
        weatherInfo["description"] = weatherData.first;
        weatherInfo["humidity"] = 60;
        weatherInfo["wind_speed"] = 10;
        weatherInfo["success"] = true;
    } else {
        weatherInfo["success"] = false;
        weatherInfo["error"] = "暂不支持该城市天气查询";
    }

    return weatherInfo;
}

void FlightServer::incomingConnection(qintptr socketDescriptor)
{
    QThread* thread = new QThread(this);
    Clientthread* client = new Clientthread(socketDescriptor, this);
    client->moveToThread(thread);

    connect(thread, &QThread::started, client, &Clientthread::process);
    connect(client, &Clientthread::finished, thread, &QThread::quit);
    connect(client, &Clientthread::finished, client, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}
