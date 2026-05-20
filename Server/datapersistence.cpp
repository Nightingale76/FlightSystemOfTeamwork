#include "datapersistence.h"
#include "flightserver.h" // 包含数据结构定义
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>

DataPersistence& DataPersistence::getInstance()
{
    static DataPersistence instance;
    return instance;
}

DataPersistence::DataPersistence(QObject* parent) : QObject(parent)
{
    // 确定数据存储路径
    m_dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (m_dataDir.isEmpty()) {
        m_dataDir = QDir::currentPath() + "/data";
    }

    // 确保目录存在
    QDir dir;
    if (!dir.exists(m_dataDir)) {
        dir.mkpath(m_dataDir);
    }

    m_dataFile = m_dataDir + "/flight_booking_data.json";
    qDebug() << "数据文件路径:" << m_dataFile;
}

DataPersistence::~DataPersistence()
{
}

QString DataPersistence::getDataFilePath() const
{
    return m_dataFile;
}

bool DataPersistence::saveAllData(const QList<UserInfo>& users,
                                  const QList<AdminInfo>& admins,
                                  const QList<FlightInfo>& flights,
                                  const QList<FlightSeat>& seats,
                                  const QList<BookingOrder>& orders)
{
    QJsonObject root;

    // 保存ID计数器
    root["nextUserId"] = users.isEmpty() ? 1 : (users.last().user_id + 1);
    root["nextAdminId"] = admins.isEmpty() ? 1 : (admins.last().admin_id + 1);
    root["nextFlightId"] = flights.isEmpty() ? 1 : (flights.last().flight_id + 1);
    root["nextSeatId"] = seats.isEmpty() ? 1 : (seats.last().seat_id + 1);
    root["nextOrderId"] = orders.isEmpty() ? 1 : (orders.last().order_id.toInt() + 1);

    // 保存数据数组
    QJsonArray usersArray;
    for (const UserInfo& user : users) {
        usersArray.append(userToJson(user));
    }
    root["users"] = usersArray;

    QJsonArray adminsArray;
    for (const AdminInfo& admin : admins) {
        adminsArray.append(adminToJson(admin));
    }
    root["admins"] = adminsArray;

    QJsonArray flightsArray;
    for (const FlightInfo& flight : flights) {
        flightsArray.append(flightToJson(flight));
    }
    root["flights"] = flightsArray;

    QJsonArray seatsArray;
    for (const FlightSeat& seat : seats) {
        seatsArray.append(seatToJson(seat));
    }
    root["seats"] = seatsArray;

    QJsonArray ordersArray;
    for (const BookingOrder& order : orders) {
        ordersArray.append(orderToJson(order));
    }
    root["orders"] = ordersArray;

    // 保存到文件
    QFile file(m_dataFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "无法打开数据文件进行写入:" << file.errorString();
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();

    qDebug() << "数据保存成功，文件:" << m_dataFile;
    qDebug() << "保存数据统计 - 用户:" << users.size()
             << "管理员:" << admins.size()
             << "航班:" << flights.size()
             << "座位:" << seats.size()
             << "订单:" << orders.size();

    return true;
}

bool DataPersistence::loadAllData(QList<UserInfo>& users,
                                  QList<AdminInfo>& admins,
                                  QList<FlightInfo>& flights,
                                  QList<FlightSeat>& seats,
                                  QList<BookingOrder>& orders,
                                  int& nextUserId, int& nextAdminId,
                                  int& nextFlightId, int& nextSeatId, int& nextOrderId)
{
    QFile file(m_dataFile);
    if (!file.exists()) {
        qDebug() << "数据文件不存在，将使用默认数据";
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开数据文件进行读取:" << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "数据文件格式错误";
        return false;
    }

    QJsonObject root = doc.object();

    // 加载ID计数器
    nextUserId = root["nextUserId"].toInt(1);
    nextAdminId = root["nextAdminId"].toInt(1);
    nextFlightId = root["nextFlightId"].toInt(1);
    nextSeatId = root["nextSeatId"].toInt(1);
    nextOrderId = root["nextOrderId"].toInt(1);

    // 清空现有数据
    users.clear();
    admins.clear();
    flights.clear();
    seats.clear();
    orders.clear();

    // 加载数据数组
    QJsonArray usersArray = root["users"].toArray();
    for (const QJsonValue& value : usersArray) {
        users.append(jsonToUser(value.toObject()));
    }

    QJsonArray adminsArray = root["admins"].toArray();
    for (const QJsonValue& value : adminsArray) {
        admins.append(jsonToAdmin(value.toObject()));
    }

    QJsonArray flightsArray = root["flights"].toArray();
    for (const QJsonValue& value : flightsArray) {
        flights.append(jsonToFlight(value.toObject()));
    }

    QJsonArray seatsArray = root["seats"].toArray();
    for (const QJsonValue& value : seatsArray) {
        seats.append(jsonToSeat(value.toObject()));
    }

    QJsonArray ordersArray = root["orders"].toArray();
    for (const QJsonValue& value : ordersArray) {
        orders.append(jsonToOrder(value.toObject()));
    }

    qDebug() << "数据加载成功，文件:" << m_dataFile;
    qDebug() << "加载数据统计 - 用户:" << users.size()
             << "管理员:" << admins.size()
             << "航班:" << flights.size()
             << "座位:" << seats.size()
             << "订单:" << orders.size();

    return true;
}

// JSON转换实现
QJsonObject DataPersistence::userToJson(const UserInfo& user)
{
    QJsonObject obj;
    obj["user_id"] = user.user_id;
    obj["user_nickname"] = user.user_nickname;
    obj["user_phone"] = user.user_phone;
    obj["user_password"] = user.user_password;
    obj["create_time"] = user.create_time.toString(Qt::ISODate);
    obj["update_time"] = user.update_time.toString(Qt::ISODate);
    return obj;
}

QJsonObject DataPersistence::adminToJson(const AdminInfo& admin)
{
    QJsonObject obj;
    obj["admin_id"] = admin.admin_id;
    obj["admin_account"] = admin.admin_account;
    obj["admin_password"] = admin.admin_password;
    obj["admin_name"] = admin.admin_name;
    obj["create_time"] = admin.create_time.toString(Qt::ISODate);
    return obj;
}

QJsonObject DataPersistence::flightToJson(const FlightInfo& flight)
{
    QJsonObject obj;
    obj["flight_id"] = flight.flight_id;
    obj["flight_num"] = flight.flight_num;
    obj["airline_company"] = flight.airline_company;
    obj["departure_city"] = flight.departure_city;
    obj["arrival_city"] = flight.arrival_city;
    obj["departure_time"] = flight.departure_time.toString(Qt::ISODate);
    obj["arrival_time"] = flight.arrival_time.toString(Qt::ISODate);
    obj["base_price"] = flight.base_price;
    obj["total_seats"] = flight.total_seats;
    obj["available_seats"] = flight.available_seats;
    obj["create_time"] = flight.create_time.toString(Qt::ISODate);
    // 新增：保存登机口数据到JSON
    obj["gate"] = flight.gate;
    return obj;
}

QJsonObject DataPersistence::seatToJson(const FlightSeat& seat)
{
    QJsonObject obj;
    obj["seat_id"] = seat.seat_id;
    obj["flight_id"] = seat.flight_id;
    obj["flight_num"] = seat.flight_num;
    obj["seat_type"] = seat.seat_type;
    obj["seat_num"] = seat.seat_num;
    obj["seat_price"] = seat.seat_price;
    obj["seat_status"] = seat.seat_status;
    obj["create_time"] = seat.create_time.toString(Qt::ISODate);
    return obj;
}

QJsonObject DataPersistence::orderToJson(const BookingOrder& order)
{
    QJsonObject obj;
    obj["order_id"] = order.order_id;
    obj["user_id"] = order.user_id;
    obj["flight_id"] = order.flight_id;
    obj["seat_id"] = order.seat_id;
    obj["user_phone"] = order.user_phone;
    obj["flight_num"] = order.flight_num;
    obj["seat_num"] = order.seat_num;
    obj["passenger_name"] = order.passenger_name;
    obj["passenger_idcard"] = order.passenger_idcard;
    obj["contact_phone"] = order.contact_phone;
    obj["order_status"] = order.order_status;
    obj["order_amount"] = order.order_amount;
    obj["create_time"] = order.create_time.toString(Qt::ISODate);
    if (order.pay_time.isValid()) {
        obj["pay_time"] = order.pay_time.toString(Qt::ISODate);
    }
    return obj;
}

UserInfo DataPersistence::jsonToUser(const QJsonObject& json)
{
    UserInfo user;
    user.user_id = json["user_id"].toInt();
    user.user_nickname = json["user_nickname"].toString();
    user.user_phone = json["user_phone"].toString();
    user.user_password = json["user_password"].toString();
    user.create_time = QDateTime::fromString(json["create_time"].toString(), Qt::ISODate);
    user.update_time = QDateTime::fromString(json["update_time"].toString(), Qt::ISODate);
    return user;
}

AdminInfo DataPersistence::jsonToAdmin(const QJsonObject& json)
{
    AdminInfo admin;
    admin.admin_id = json["admin_id"].toInt();
    admin.admin_account = json["admin_account"].toString();
    admin.admin_password = json["admin_password"].toString();
    admin.admin_name = json["admin_name"].toString();
    admin.create_time = QDateTime::fromString(json["create_time"].toString(), Qt::ISODate);
    return admin;
}

FlightInfo DataPersistence::jsonToFlight(const QJsonObject& json)
{
    FlightInfo flight;
    flight.flight_id = json["flight_id"].toInt();
    flight.flight_num = json["flight_num"].toString();
    flight.airline_company = json["airline_company"].toString();
    flight.departure_city = json["departure_city"].toString();
    flight.arrival_city = json["arrival_city"].toString();
    flight.departure_time = QDateTime::fromString(json["departure_time"].toString(), Qt::ISODate);
    flight.arrival_time = QDateTime::fromString(json["arrival_time"].toString(), Qt::ISODate);
    flight.base_price = json["base_price"].toDouble();
    flight.total_seats = json["total_seats"].toInt();
    flight.available_seats = json["available_seats"].toInt();
    flight.create_time = QDateTime::fromString(json["create_time"].toString(), Qt::ISODate);
    // 新增：从JSON读取登机口数据（兼容旧数据：如果没有gate字段，默认空字符串）
    flight.gate = json["gate"].toString();
    return flight;
}

FlightSeat DataPersistence::jsonToSeat(const QJsonObject& json)
{
    FlightSeat seat;
    seat.seat_id = json["seat_id"].toInt();
    seat.flight_id = json["flight_id"].toInt();
    seat.flight_num = json["flight_num"].toString();
    seat.seat_type = json["seat_type"].toString();
    seat.seat_num = json["seat_num"].toString();
    seat.seat_price = json["seat_price"].toDouble();
    seat.seat_status = json["seat_status"].toString();
    seat.create_time = QDateTime::fromString(json["create_time"].toString(), Qt::ISODate);
    return seat;
}

BookingOrder DataPersistence::jsonToOrder(const QJsonObject& json)
{
    BookingOrder order;
    order.order_id = json["order_id"].toString();
    order.user_id = json["user_id"].toInt();
    order.flight_id = json["flight_id"].toInt();
    order.seat_id = json["seat_id"].toInt();
    order.user_phone = json["user_phone"].toString();
    order.flight_num = json["flight_num"].toString();
    order.seat_num = json["seat_num"].toString();
    order.passenger_name = json["passenger_name"].toString();
    order.passenger_idcard = json["passenger_idcard"].toString();
    order.contact_phone = json["contact_phone"].toString();
    order.order_status = json["order_status"].toString();
    order.order_amount = json["order_amount"].toDouble();
    order.create_time = QDateTime::fromString(json["create_time"].toString(), Qt::ISODate);

    if (json.contains("pay_time") && !json["pay_time"].isNull()) {
        order.pay_time = QDateTime::fromString(json["pay_time"].toString(), Qt::ISODate);
    }

    return order;
}
