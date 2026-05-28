#include "dbmanager.h"
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSettings>
#include <QSqlDatabase>
#include <QThread>

namespace {

struct DatabaseConfig
{
    QString driver = "MySQL ODBC 8.0 Unicode Driver";
    QString server = "127.0.0.1";
    int port = 3306;
    QString database = "flight_booking_system";
    QString user = "root";
    QString password;
    QString charset = "utf8mb4";
};

QString firstExistingConfigPath()
{
    const QString appDirPath = QCoreApplication::applicationDirPath() + "/db_config.ini";
    if (QFileInfo::exists(appDirPath))
        return appDirPath;

    const QString workDirPath = QDir::currentPath() + "/db_config.ini";
    if (QFileInfo::exists(workDirPath))
        return workDirPath;

    return appDirPath;
}

QString envOrSetting(const char* envName, QSettings& settings, const QString& key, const QString& defaultValue)
{
    const QByteArray envValue = qgetenv(envName);
    if (!envValue.isEmpty())
        return QString::fromLocal8Bit(envValue);

    return settings.value(key, defaultValue).toString();
}

DatabaseConfig loadDatabaseConfig()
{
    QSettings settings(firstExistingConfigPath(), QSettings::IniFormat);
    settings.beginGroup("database");

    DatabaseConfig config;
    config.driver = envOrSetting("FLIGHT_DB_DRIVER", settings, "driver", config.driver);
    config.server = envOrSetting("FLIGHT_DB_SERVER", settings, "server", config.server);
    config.port = envOrSetting("FLIGHT_DB_PORT", settings, "port", QString::number(config.port)).toInt();
    config.database = envOrSetting("FLIGHT_DB_NAME", settings, "name", config.database);
    config.user = envOrSetting("FLIGHT_DB_USER", settings, "user", config.user);
    config.password = envOrSetting("FLIGHT_DB_PASSWORD", settings, "password", config.password);
    config.charset = envOrSetting("FLIGHT_DB_CHARSET", settings, "charset", config.charset);

    settings.endGroup();
    return config;
}

QString buildOdbcConnectionString(const DatabaseConfig& config)
{
    return QString("DRIVER={%1};"
                   "SERVER=%2;PORT=%3;"
                   "DATABASE=%4;"
                   "USER=%5;PASSWORD=%6;"
                   "charset=%7;")
        .arg(config.driver,
             config.server,
             QString::number(config.port),
             config.database,
             config.user,
             config.password,
             config.charset);
}

}

DBManager::DBManager(QObject* parent) : QObject(parent)
{
    // 不再提前 open 任何连接
}

DBManager::~DBManager()
{
    // 连接名随线程销毁，无需手动 close
}

DBManager& DBManager::getInstance()
{
    static DBManager instance;
    return instance;
}

/* ---------- 线程级别连接 ---------- */
QSqlDatabase DBManager::threadDB()
{
    QString connName = QString("flight_%1").arg(quintptr(QThread::currentThreadId()));
    if (QSqlDatabase::contains(connName))
        return QSqlDatabase::database(connName);

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", connName);
    db.setDatabaseName(buildOdbcConnectionString(loadDatabaseConfig()));
    if (!db.open())
        qFatal("线程ODBC连接失败：%s", qPrintable(db.lastError().text()));
    return db;
}

/* ---------- 用户相关 ---------- */
bool DBManager::addUser(const QString& phone, const QString& password, const QString& nickname)
{
    QSqlQuery query(threadDB());
    query.prepare("INSERT INTO users (user_nickname, user_phone, user_password) "
                  "VALUES (:nickname, :phone, :password)");
    query.bindValue(":nickname", nickname.isEmpty() ? "用户" + phone : nickname);
    query.bindValue(":phone", phone);
    query.bindValue(":password", password);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "【addUser SQL 错误】" << m_lastError;
        return false;
    }
    return true;
}

bool DBManager::validateUser(const QString& phone, const QString& password, QJsonObject& userInfo)
{
    QSqlQuery query(threadDB());
    query.prepare("SELECT user_id, user_nickname, user_phone, create_time "
                  "FROM users WHERE user_phone = :phone AND user_password = :password");
    query.bindValue(":phone", phone);
    query.bindValue(":password", password);
    if (!query.exec() || !query.next())
        return false;
    userInfo["user_id"] = query.value(0).toInt();
    userInfo["user_nickname"] = query.value(1).toString();
    userInfo["user_phone"] = query.value(2).toString();
    userInfo["create_time"] = query.value(3).toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    return true;
}

bool DBManager::updateUserPassword(const QString& phone, const QString& newPassword)
{
    QSqlQuery query(threadDB());
    query.prepare("UPDATE users SET user_password = :password WHERE user_phone = :phone");
    query.bindValue(":password", newPassword);
    query.bindValue(":phone", phone);
    return query.exec() && query.numRowsAffected() > 0;
}

QJsonArray DBManager::getAllUsers()
{
    QJsonArray usersArray;
    QSqlQuery query("SELECT user_id, user_nickname, user_phone, create_time, update_time "
                    "FROM users ORDER BY create_time DESC", threadDB());
    while (query.next()) {
        QJsonObject user;
        user["user_id"] = query.value(0).toInt();
        user["user_nickname"] = query.value(1).toString();
        user["user_phone"] = query.value(2).toString();
        user["create_time"] = query.value(3).toDateTime().toString("yyyy-MM-dd HH:mm:ss");
        user["update_time"] = query.value(4).toDateTime().toString("yyyy-MM-dd HH:mm:ss");
        usersArray.append(user);
    }
    return usersArray;
}

/* ---------- 管理员 ---------- */
bool DBManager::validateAdmin(const QString& account, const QString& password, QJsonObject& adminInfo)
{
    QSqlQuery query(threadDB());
    query.prepare("SELECT admin_id, admin_account, admin_name, create_time "
                  "FROM admins WHERE admin_account = :account AND admin_password = :password");
    query.bindValue(":account", account);
    query.bindValue(":password", password);
    if (!query.exec() || !query.next())
        return false;
    adminInfo["admin_id"] = query.value(0).toInt();
    adminInfo["admin_account"] = query.value(1).toString();
    adminInfo["admin_name"] = query.value(2).toString();
    adminInfo["create_time"] = query.value(3).toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    return true;
}

/* ---------- 航班 ---------- */
bool DBManager::addFlight(const QJsonObject& flightData)
{
    QSqlQuery query(threadDB());
    query.prepare("INSERT INTO flights (flight_num, airline_company, departure_city, "
                  "arrival_city, departure_time, arrival_time, base_price, total_seats, "
                  "available_seats, gate) VALUES (:num, :airline, :departure, :arrival, "
                  ":departure_time, :arrival_time, :price, :total, :available, :gate)");
    query.bindValue(":num", flightData["flight_num"].toString());
    query.bindValue(":airline", flightData["airline_company"].toString());
    query.bindValue(":departure", flightData["departure_city"].toString());
    query.bindValue(":arrival", flightData["arrival_city"].toString());
    query.bindValue(":departure_time", flightData["departure_time"].toString());
    query.bindValue(":arrival_time", flightData["arrival_time"].toString());
    query.bindValue(":price", flightData["base_price"].toDouble());
    query.bindValue(":total", flightData["total_seats"].toInt());
    query.bindValue(":available", flightData["total_seats"].toInt());
    query.bindValue(":gate", flightData.contains("gate") ? flightData["gate"].toString() : "");
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "【addFlight SQL 错误】" << m_lastError;
        return false;
    }
    return true;
}

QJsonArray DBManager::getAllFlights()
{
    QJsonArray flightsArray;
    QSqlQuery query("SELECT * FROM flights ORDER BY departure_time", threadDB());
    while (query.next())
        flightsArray.append(flightRecordToJson(query.record()));
    return flightsArray;
}
bool DBManager::connectToDatabase()
{
    // 无参版本：使用默认连接串
    return threadDB().isOpen();
}
QJsonArray DBManager::queryFlights(const QString& departure, const QString& arrival, const QString& date)
{
    QJsonArray flightsArray;
    QString sql = "SELECT * FROM flights WHERE 1=1";
    if (!departure.isEmpty()) sql += " AND departure_city LIKE :departure";
    if (!arrival.isEmpty()) sql += " AND arrival_city LIKE :arrival";
    if (!date.isEmpty()) sql += " AND DATE(departure_time) = :date";
    sql += " ORDER BY departure_time";

    QSqlQuery query(threadDB());
    query.prepare(sql);
    if (!departure.isEmpty()) query.bindValue(":departure", "%" + departure + "%");
    if (!arrival.isEmpty()) query.bindValue(":arrival", "%" + arrival + "%");
    if (!date.isEmpty()) query.bindValue(":date", date);
    if (!query.exec()) {
        qDebug() << "查询航班失败:" << query.lastError();
        return flightsArray;
    }
    while (query.next())
        flightsArray.append(flightRecordToJson(query.record()));
    return flightsArray;
}

QString DBManager::getFlightGate(const QString& flightNum)
{
    QSqlQuery query(threadDB());
    query.prepare("SELECT gate FROM flights WHERE flight_num = :num");
    query.bindValue(":num", flightNum);
    if (query.exec() && query.next())
        return query.value(0).toString();
    return "";
}

bool DBManager::updateFlightGate(const QString& flightNum, const QString& gate)
{
    QSqlQuery query(threadDB());
    query.prepare("UPDATE flights SET gate = :gate WHERE flight_num = :num");
    query.bindValue(":gate", gate);
    query.bindValue(":num", flightNum);
    return query.exec() && query.numRowsAffected() > 0;
}

/* ---------- 座位 ---------- */
bool DBManager::addSeat(const QJsonObject& seatData)
{
    // 1. 先根据 flight_num 查出真实 flight_id
    QString flightNum = seatData["flight_num"].toString();
    QSqlQuery getId(threadDB());
    getId.prepare("SELECT flight_id FROM flights WHERE flight_num = :num");
    getId.bindValue(":num", flightNum);
    if (!getId.exec() || !getId.next()) {
        m_lastError = "航班号不存在";
        qDebug() << "【addSeat】航班号不存在：" << flightNum;
        return false;
    }
    int flightId = getId.value(0).toInt();

    // 2. 再插入座位
    QSqlQuery query(threadDB());
    query.prepare("INSERT INTO flight_seats (flight_id, flight_num, seat_type, seat_num, "
                  "seat_price, seat_status) VALUES (:flight_id, :flight_num, :seat_type, "
                  ":seat_num, :price, 'available')");
    query.bindValue(":flight_id", flightId);                       // 真实外键
    query.bindValue(":flight_num", flightNum);
    query.bindValue(":seat_type", seatData["seat_type"].toString());
    query.bindValue(":seat_num", seatData["seat_num"].toString());
    query.bindValue(":price", seatData["seat_price"].toDouble());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "【addSeat SQL 错误】" << m_lastError;
        return false;
    }
    return true;
}

QJsonArray DBManager::getSeatsByFlight(const QString& flightNum)
{
    QJsonArray seatsArray;
    QSqlQuery query(threadDB());
    if (flightNum.isEmpty()) {
        query.exec("SELECT * FROM flight_seats ORDER BY flight_num, seat_num");
    } else {
        query.prepare("SELECT * FROM flight_seats WHERE flight_num = :num ORDER BY seat_num");
        query.bindValue(":num", flightNum);
        query.exec();
    }
    while (query.next())
        seatsArray.append(seatRecordToJson(query.record()));
    return seatsArray;
}

bool DBManager::updateSeatStatus(const QString& flightNum, const QString& seatNum, const QString& status)
{
    QSqlQuery query(threadDB());
    query.prepare("UPDATE flight_seats SET seat_status = :status "
                  "WHERE flight_num = :flight_num AND seat_num = :seat_num");
    query.bindValue(":status", status);
    query.bindValue(":flight_num", flightNum);
    query.bindValue(":seat_num", seatNum);
    return query.exec() && query.numRowsAffected() > 0;
}

bool DBManager::batchUpdateSeats(const QJsonArray& seatList, const QString& status)
{
    QSqlDatabase::database().transaction();
    try {
        for (const QJsonValue& v : seatList) {
            QJsonObject seat = v.toObject();
            if (!updateSeatStatus(seat["flight_num"].toString(),
                                  seat["seat_num"].toString(), status))
                throw QString("批量更新失败");
        }
        QSqlDatabase::database().commit();
        return true;
    } catch (const QString& e) {
        QSqlDatabase::database().rollback();
        qDebug() << e;
        return false;
    }
}

/* ---------- 订单 ---------- */
bool DBManager::createOrder(const QJsonObject& orderData)
{
    QSqlDatabase db = threadDB();
    db.transaction();
    try {
        QSqlQuery checkSeat(db);
        checkSeat.prepare("SELECT seat_id, seat_status FROM flight_seats "
                          "WHERE flight_num = :flight_num AND seat_num = :seat_num");
        checkSeat.bindValue(":flight_num", orderData["flight_num"].toString());
        checkSeat.bindValue(":seat_num", orderData["seat_num"].toString());
        if (!checkSeat.exec() || !checkSeat.next())
            throw QString("座位不存在");
        if (checkSeat.value(1).toString() != "available")
            throw QString("座位已被占用");
        int seatId = checkSeat.value(0).toInt();

        QSqlQuery getUser(db);
        getUser.prepare("SELECT user_id FROM users WHERE user_phone = :phone");
        getUser.bindValue(":phone", orderData["user_phone"].toString());
        if (!getUser.exec() || !getUser.next())
            throw QString("用户不存在");
        int userId = getUser.value(0).toInt();

        QSqlQuery getFlight(db);
        getFlight.prepare("SELECT flight_id FROM flights WHERE flight_num = :flight_num");
        getFlight.bindValue(":flight_num", orderData["flight_num"].toString());
        if (!getFlight.exec() || !getFlight.next())
            throw QString("航班不存在");
        int flightId = getFlight.value(0).toInt();

        QString orderId = QString("ORD%1%2").arg(QDateTime::currentSecsSinceEpoch())
                              .arg(QRandomGenerator::global()->bounded(10000), 4, 10, QChar('0'));

        QSqlQuery ins(db);
        ins.prepare("INSERT INTO booking_orders (order_id, user_id, flight_id, seat_id, "
                    "user_phone, flight_num, seat_num, passenger_name, passenger_idcard, "
                    "contact_phone, order_status, order_amount, departure_city, departure_time) "
                    "VALUES (:order_id, :user_id, :flight_id, :seat_id, :user_phone, "
                    ":flight_num, :seat_num, :passenger_name, :passenger_idcard, "
                    ":contact_phone, 'unpaid', :amount, :departure_city, :departure_time)");
        ins.bindValue(":order_id", orderId);
        ins.bindValue(":user_id", userId);
        ins.bindValue(":flight_id", flightId);
        ins.bindValue(":seat_id", seatId);
        ins.bindValue(":user_phone", orderData["user_phone"].toString());
        ins.bindValue(":flight_num", orderData["flight_num"].toString());
        ins.bindValue(":seat_num", orderData["seat_num"].toString());
        ins.bindValue(":passenger_name", orderData["passenger_name"].toString());
        ins.bindValue(":passenger_idcard", orderData["passenger_idcard"].toString());
        ins.bindValue(":contact_phone", orderData["contact_phone"].toString());
        ins.bindValue(":amount", orderData["order_amount"].toDouble());
        ins.bindValue(":departure_city", orderData["departure_city"].toString());
        ins.bindValue(":departure_time", orderData["departure_time"].toString());
        if (!ins.exec())
            throw QString("创建订单失败: " + ins.lastError().text());

        if (!updateSeatStatus(orderData["flight_num"].toString(),
                              orderData["seat_num"].toString(), "locked"))
            throw QString("锁定座位失败");

        db.commit();
        return true;
    } catch (const QString& e) {
        db.rollback();
        qDebug() << "创建订单失败:" << e;
        return false;
    }
}

QJsonArray DBManager::getOrdersByUserPhone(const QString& phone)
{
    QJsonArray ordersArray;
    QSqlQuery query(threadDB());
    query.prepare("SELECT o.*, f.departure_city, f.departure_time "
                  "FROM booking_orders o "
                  "JOIN flights f ON o.flight_id = f.flight_id "
                  "WHERE o.user_phone = :phone "
                  "ORDER BY o.create_time DESC");
    query.bindValue(":phone", phone);
    if (!query.exec()) return ordersArray;
    while (query.next())
        ordersArray.append(orderRecordToJson(query.record()));
    return ordersArray;
}

QJsonArray DBManager::getOrdersByFilters(const QJsonObject& filters)
{
    QJsonArray ordersArray;
    QString sql = "SELECT o.*, f.departure_city, f.departure_time "
                  "FROM booking_orders o "
                  "JOIN flights f ON o.flight_id = f.flight_id "
                  "WHERE 1=1";
    if (filters.contains("passenger_name") && !filters["passenger_name"].toString().isEmpty())
        sql += " AND o.passenger_name LIKE :name";
    if (filters.contains("passenger_idcard") && !filters["passenger_idcard"].toString().isEmpty())
        sql += " AND o.passenger_idcard LIKE :idcard";
    if (filters.contains("flight_num") && !filters["flight_num"].toString().isEmpty())
        sql += " AND o.flight_num = :flight_num";
    if (filters.contains("status_filter") && !filters["status_filter"].toString().isEmpty())
        sql += " AND o.order_status = :status";
    sql += " ORDER BY o.create_time DESC";

    QSqlQuery query(threadDB());
    query.prepare(sql);
    if (filters.contains("passenger_name") && !filters["passenger_name"].toString().isEmpty())
        query.bindValue(":name", "%" + filters["passenger_name"].toString() + "%");
    if (filters.contains("passenger_idcard") && !filters["passenger_idcard"].toString().isEmpty())
        query.bindValue(":idcard", "%" + filters["passenger_idcard"].toString() + "%");
    if (filters.contains("flight_num") && !filters["flight_num"].toString().isEmpty())
        query.bindValue(":flight_num", filters["flight_num"].toString());
    if (filters.contains("status_filter") && !filters["status_filter"].toString().isEmpty())
        query.bindValue(":status", filters["status_filter"].toString());
    if (!query.exec()) return ordersArray;
    while (query.next())
        ordersArray.append(orderRecordToJson(query.record()));
    return ordersArray;
}

QJsonArray DBManager::getAllOrders()
{
    QJsonArray ordersArray;
    QSqlQuery query("SELECT o.*, f.departure_city, f.departure_time "
                    "FROM booking_orders o "
                    "JOIN flights f ON o.flight_id = f.flight_id "
                    "ORDER BY o.create_time DESC", threadDB());
    while (query.next())
        ordersArray.append(orderRecordToJson(query.record()));
    return ordersArray;
}

bool DBManager::updateOrderStatus(const QString& orderId, const QString& status)
{
    QSqlQuery query(threadDB());
    query.prepare("UPDATE booking_orders SET order_status = :status WHERE order_id = :order_id");
    query.bindValue(":status", status);
    query.bindValue(":order_id", orderId);
    return query.exec() && query.numRowsAffected() > 0;
}

/* ---------- 投诉/餐食 ---------- */
bool DBManager::addComplaint(const QJsonObject& complaintData)
{
    QSqlQuery query(threadDB());
    QString id = QString("CMP%1").arg(QDateTime::currentSecsSinceEpoch());
    query.prepare("INSERT INTO complaints (id, user_phone, user_name, flight_num, "
                  "type, content, submit_time) VALUES (:id, :phone, :name, :flight, "
                  ":type, :content, NOW())");
    query.bindValue(":id", id);
    query.bindValue(":phone", complaintData["user_phone"].toString());
    query.bindValue(":name", complaintData["user_name"].toString());
    query.bindValue(":flight", complaintData["flight_num"].toString());
    query.bindValue(":type", complaintData["type"].toString());
    query.bindValue(":content", complaintData["content"].toString());
    return query.exec();
}

QJsonArray DBManager::getAllComplaints()
{
    QJsonArray complaintsArray;
    QSqlQuery query("SELECT * FROM complaints ORDER BY submit_time DESC", threadDB());
    while (query.next()) {
        QJsonObject c;
        c["id"] = query.value("id").toString();
        c["user_phone"] = query.value("user_phone").toString();
        c["user_name"] = query.value("user_name").toString();
        c["flight_num"] = query.value("flight_num").toString();
        c["type"] = query.value("type").toString();
        c["content"] = query.value("content").toString();
        c["submit_time"] = query.value("submit_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
        c["status"] = query.value("status").toString();
        complaintsArray.append(c);
    }
    return complaintsArray;
}

bool DBManager::addMealOrder(const QJsonObject& mealData)
{
    QSqlQuery query(threadDB());
    QString id = QString("MEAL%1").arg(QDateTime::currentSecsSinceEpoch());
    query.prepare("INSERT INTO meal_orders (id, user_phone, user_name, flight_num, "
                  "flight_time, meal_name, price, order_time) VALUES (:id, :phone, "
                  ":name, :flight, :flight_time, :meal, :price, NOW())");
    query.bindValue(":id", id);
    query.bindValue(":phone", mealData["user_phone"].toString());
    query.bindValue(":name", mealData["user_name"].toString());
    query.bindValue(":flight", mealData["flight_num"].toString());
    query.bindValue(":flight_time", mealData.contains("flight_time") ?
                                        mealData["flight_time"].toString() : "");
    query.bindValue(":meal", mealData["meal_name"].toString());
    query.bindValue(":price", mealData["price"].toInt());
    return query.exec();
}

QJsonArray DBManager::getAllMealOrders()
{
    QJsonArray mealOrdersArray;
    QSqlQuery query("SELECT * FROM meal_orders ORDER BY order_time DESC", threadDB());
    while (query.next()) {
        QJsonObject m;
        m["id"] = query.value("id").toString();
        m["user_phone"] = query.value("user_phone").toString();
        m["user_name"] = query.value("user_name").toString();
        m["flight_num"] = query.value("flight_num").toString();
        m["flight_time"] = query.value("flight_time").toString();
        m["meal_name"] = query.value("meal_name").toString();
        m["price"] = query.value("price").toInt();
        m["order_time"] = query.value("order_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
        m["status"] = query.value("status").toString();
        mealOrdersArray.append(m);
    }
    return mealOrdersArray;
}

/* ---------- 辅助：记录转 JSON ---------- */
QJsonObject DBManager::flightRecordToJson(const QSqlRecord& rec)
{
    QJsonObject f;
    f["flight_id"] = rec.value("flight_id").toInt();
    f["flight_num"] = rec.value("flight_num").toString();
    f["airline_company"] = rec.value("airline_company").toString();
    f["departure_city"] = rec.value("departure_city").toString();
    f["arrival_city"] = rec.value("arrival_city").toString();
    f["departure_time"] = rec.value("departure_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    f["arrival_time"] = rec.value("arrival_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    f["base_price"] = rec.value("base_price").toDouble();
    f["total_seats"] = rec.value("total_seats").toInt();
    f["available_seats"] = rec.value("available_seats").toInt();
    f["gate"] = rec.value("gate").toString();
    f["create_time"] = rec.value("create_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    return f;
}

QJsonObject DBManager::userRecordToJson(const QSqlRecord& rec)
{
    QJsonObject u;
    u["user_id"] = rec.value("user_id").toInt();
    u["user_nickname"] = rec.value("user_nickname").toString();
    u["user_phone"] = rec.value("user_phone").toString();
    u["create_time"] = rec.value("create_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    if (rec.contains("update_time"))
        u["update_time"] = rec.value("update_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    return u;
}

QJsonObject DBManager::seatRecordToJson(const QSqlRecord& rec)
{
    QJsonObject s;
    s["seat_id"] = rec.value("seat_id").toInt();
    s["flight_id"] = rec.value("flight_id").toInt();
    s["flight_num"] = rec.value("flight_num").toString();
    s["seat_num"] = rec.value("seat_num").toString();
    s["seat_type"] = rec.value("seat_type").toString();
    s["seat_price"] = rec.value("seat_price").toDouble();
    s["seat_status"] = rec.value("seat_status").toString();
    s["create_time"] = rec.value("create_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    return s;
}

QJsonObject DBManager::orderRecordToJson(const QSqlRecord& rec)
{
    QJsonObject o;
    o["order_id"] = rec.value("order_id").toString();
    o["user_id"] = rec.value("user_id").toInt();
    o["flight_id"] = rec.value("flight_id").toInt();
    o["seat_id"] = rec.value("seat_id").toInt();
    o["user_phone"] = rec.value("user_phone").toString();
    o["flight_num"] = rec.value("flight_num").toString();
    o["seat_num"] = rec.value("seat_num").toString();
    o["passenger_name"] = rec.value("passenger_name").toString();
    o["passenger_idcard"] = rec.value("passenger_idcard").toString();
    o["contact_phone"] = rec.value("contact_phone").toString();
    o["order_status"] = rec.value("order_status").toString();
    o["order_amount"] = rec.value("order_amount").toDouble();
    o["create_time"] = rec.value("create_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    if (!rec.value("pay_time").isNull())
        o["pay_time"] = rec.value("pay_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    if (rec.contains("departure_city"))
        o["departure_city"] = rec.value("departure_city").toString();
    if (rec.contains("departure_time"))
        o["departure_time"] = rec.value("departure_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    return o;
}

bool DBManager::updateFlight(const QJsonObject& flightData)
{
    QSqlQuery query(threadDB());
    query.prepare("UPDATE flights SET "
                  "airline_company = :airline, "
                  "departure_city  = :departure, "
                  "arrival_city    = :arrival, "
                  "departure_time  = :departure_time, "
                  "arrival_time    = :arrival_time, "
                  "base_price      = :price, "
                  "total_seats     = :total, "
                  "available_seats = :available, "
                  "gate            = :gate "
                  "WHERE flight_num = :num");

    /* 显式指定类型，防止 ODBC 猜错 */
    query.bindValue(":airline",       QVariant(flightData["airline_company"].toString()));
    query.bindValue(":departure",     QVariant(flightData["departure_city"].toString()));
    query.bindValue(":arrival",       QVariant(flightData["arrival_city"].toString()));
    query.bindValue(":departure_time",QVariant(flightData["departure_time"].toString()));
    query.bindValue(":arrival_time",  QVariant(flightData["arrival_time"].toString()));
    query.bindValue(":price",         QVariant(flightData["base_price"].toDouble()));
    query.bindValue(":total",         QVariant(flightData["total_seats"].toInt()));
    query.bindValue(":available",     QVariant(flightData["available_seats"].toInt()));
    query.bindValue(":gate",          QVariant(flightData["gate"].toString()));
    query.bindValue(":num",           QVariant(flightData["old_flight_num"].toString())); // 关键！

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "【updateFlight SQL 错误】" << m_lastError;
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool DBManager::deleteFlight(const QString& flightNum)
{
    QSqlDatabase db = threadDB();
    db.transaction();
    try {
        QSqlQuery delSeats(db);
        delSeats.prepare("DELETE FROM flight_seats WHERE flight_num = :num");
        delSeats.bindValue(":num", flightNum);
        if (!delSeats.exec()) throw QString("删除座位失败");

        QSqlQuery delFlight(db);
        delFlight.prepare("DELETE FROM flights WHERE flight_num = :num");
        delFlight.bindValue(":num", flightNum);
        if (!delFlight.exec()) throw QString("删除航班失败");
        db.commit();
        return true;
    } catch (const QString& e) {
        db.rollback();
        qDebug() << e;
        return false;
    }
}

    bool DBManager::updateSeat(const QJsonObject& seatData)
{
    QSqlQuery query(threadDB());
    query.prepare("UPDATE flight_seats SET "
                  "seat_type   = :type, "
                  "seat_price  = :price, "
                  "seat_status = :status "
                  "WHERE flight_num = :flight_num AND seat_num = :seat_num");

    /* 只改非键字段 */
    query.bindValue(":type",   QVariant(seatData["seat_type"].toString()));
    query.bindValue(":price",  QVariant(seatData["seat_price"].toDouble()));
    query.bindValue(":status", QVariant(seatData["seat_status"].toString()));

    /* 键只用于定位 */
    query.bindValue(":flight_num", QVariant(seatData["flight_num"].toString()));
    query.bindValue(":seat_num",   QVariant(seatData["seat_num"].toString()));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "【updateSeat SQL 错误】" << m_lastError;
        return false;
    }
    return query.numRowsAffected() > 0;
}
