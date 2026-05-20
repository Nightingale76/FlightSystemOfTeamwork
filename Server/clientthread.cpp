#include "clientthread.h"
#include "flightserver.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDateTime>
#include <QDebug>

Clientthread::Clientthread(qintptr socketDescriptor, FlightServer* server, QObject* parent)
    : QObject(parent), m_socketDescriptor(socketDescriptor), m_server(server), m_socket(nullptr)
{}

void Clientthread::process()
{
    if (!DBManager::getInstance().connectToDatabase()) {
        sendJsonResponse(500, "Internal Server Error",
                         QJsonObject{{"message", "数据库线程连接失败"}});
        emit finished();
        return;
    }
    m_socket = new QTcpSocket(); // 无 parent，避免线程问题
    if (!m_socket->setSocketDescriptor(m_socketDescriptor)) {
        qDebug() << "设置 socket 描述符失败:" << m_socket->errorString();
        m_socket->deleteLater();
        emit finished();
        return;
    }

    connect(m_socket, &QTcpSocket::readyRead, this, &Clientthread::readyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &Clientthread::disconnected);
}

void Clientthread::readyRead()
{
    QByteArray req = m_socket->readAll();
    QString reqStr = QString::fromUtf8(req);

    if (!reqStr.contains("\r\n\r\n")) {
        sendJsonResponse(400, "Bad Request", QJsonObject{{"success", false}, {"message", "不完整请求"}});
        return;
    }

    QStringList lines = reqStr.split("\r\n");
    QStringList parts = lines.first().split(" ");
    if (parts.size() != 3) {
        sendJsonResponse(400, "Bad Request", QJsonObject{{"success", false}});
        return;
    }

    QString method = parts[0];
    QString path   = parts[1];

    int emptyIdx = lines.indexOf("");
    QJsonObject json;
    if (emptyIdx != -1 && emptyIdx + 1 < lines.size()) {
        QString jsonStr = lines.mid(emptyIdx + 1).join("\r\n");
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject())
            json = doc.object();
    }

    QJsonObject resp = processRequest(path, method, json);
    sendJsonResponse(200, "OK", resp);
}

void Clientthread::disconnected()
{
    m_socket->deleteLater();
    emit finished();
}

void Clientthread::sendJsonResponse(int statusCode,
                                     const QString &statusText,
                                     const QJsonObject &json)
{
    QJsonDocument doc(json);
    QByteArray body = doc.toJson(QJsonDocument::Compact);

    QByteArray header;
    header += QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8();
    header += "Content-Type: application/json\r\n";
    header += QString("Content-Length: %1\r\n").arg(body.size()).toUtf8();
    header += "Connection: close\r\n";
    header += "Access-Control-Allow-Origin: *\r\n";
    header += "\r\n";

    m_socket->write(header + body);
    m_socket->flush();          // 立即发出
    qDebug() << "[ClientHandler] 发送响应：" << header + body;
}

QJsonObject Clientthread::processRequest(const QString& path, const QString& method, const QJsonObject& req)
{
    QJsonObject resp;
    QString action = path.mid(1);

    if (method == "OPTIONS") {
        resp["success"] = true;
        resp["message"] = "CORS preflight";
        return resp;
    }
    // 提交投诉/建议
    if (action == "submitComplaint" && method == "POST") {
        bool ok = m_server->submitComplaint(req);
        resp["success"] = ok;
        resp["message"] = ok ? QString("提交成功") : QString("提交失败");
        return resp;
    }

    // 获取所有投诉/建议
    if (action == "getAllComplaints" && method == "POST") {
        resp["success"] = true;
        resp["data"] = m_server->getAllComplaints();
        resp["message"] = "获取成功";
        return resp;
    }

    // 提交餐食订单
    if (action == "submitMealOrder" && method == "POST") {
        bool ok = m_server->submitMealOrder(req);
        resp["success"] = ok;
        resp["message"] = ok ? QString("订餐成功") : QString("订餐失败");
        return resp;
    }

    // 获取所有餐食订单
    if (action == "getAllMealOrders" && method == "POST") {
        resp["success"] = true;
        resp["data"] = m_server->getAllMealOrders();
        resp["message"] = "获取成功";
        return resp;
    }
    // 用户登录
    if (action == "userLogin" && method == "POST") {
        QString phone = req["phone"].toString();
        QString password = req["password"].toString();
        QJsonObject info;
        bool ok = m_server->userLogin(phone, password, info);
        resp["success"] = ok;
        resp["data"] = info;
        resp["message"] = ok ? "登录成功" : "手机号或密码错误";
        return resp;
    }

    // 用户注册
    // 用户注册（修复后）
    if (action == "userRegister" && method == "POST") {
        QString phone = req["phone"].toString();
        QString password = req["password"].toString();
        // 新增：从请求中读取nickname参数
        QString nickname = req["nickname"].toString();
        // 传递3个参数给userRegister
        bool ok = m_server->userRegister(phone, password, nickname);
        resp["success"] = ok;
        resp["message"] = ok ? "注册成功" : "手机号已存在";
        return resp;
    }

    // 重置密码
    if (action == "userResetPassword" && method == "POST") {
        QString phone = req["phone"].toString();
        QString newPassword = req["newPassword"].toString();
        bool ok = m_server->userResetPassword(phone, newPassword);
        resp["success"] = ok;
        resp["message"] = ok ? "密码重置成功" : "手机号不存在";
        return resp;
    }

    // 用户查询航班
    if (action == "queryFlightsForUser" && method == "POST") {
        QString departure = req["departure"].toString();
        QString arrival = req["arrival"].toString();
        QString date = req["date"].toString();
        resp["success"] = true;
        resp["data"] = m_server->queryFlightsForUser(departure, arrival, date);
        resp["message"] = "查询成功";
        return resp;
    }

    // 获取用户订单
    if (action == "getUserOrdersByPhone" && method == "POST") {
        QString phone = req["phone"].toString();
        resp["success"] = true;
        resp["data"] = m_server->getUserOrdersByPhone(phone);
        resp["message"] = "获取成功";
        return resp;
    }

    // 创建用户订单
    if (action == "createUserOrder" && method == "POST") {
        bool ok = m_server->createUserOrder(req);
        resp["success"] = ok;
        resp["message"] = ok ? "订单创建成功" : "订单创建失败";
        return resp;
    }

    // 选座
    if (action == "selectSeat" && method == "POST") {
        QString flightNum = req["flight_num"].toString();
        QString seatNum = req["seat_num"].toString();
        QString passengerPhone = req["passenger_phone"].toString();
        bool ok = m_server->selectSeat(flightNum, seatNum,passengerPhone);
        resp["success"] = ok;
        resp["message"] = ok ? "选座成功" : "选座失败";
        return resp;
    }

    // 管理员登录
    if (action == "adminLogin" && method == "POST") {
        QString u = req["username"].toString();
        QString p = req["password"].toString();
        QJsonObject info;
        bool ok = m_server->adminLogin(u, p, info);
        resp["success"] = ok;
        resp["data"] = info;
        resp["message"] = ok ? "登录成功" : "用户名或密码错误";
        return resp;
    }

    // 其他接口统一转发到 FlightServer 原函数
    if (action == "getAllFlights" && method == "POST") {
        resp["success"] = true;
        resp["data"] = m_server->getAllFlights();
        resp["message"] = "获取成功";
        return resp;
    }

    if (action == "getAllUsers" && method == "POST") {
        resp["success"] = true;
        resp["data"] = m_server->getAllUsers();
        resp["message"] = "获取成功";
        return resp;
    }

    if (action == "getAllSeats" && method == "POST") {
        QString fn = req["flight_num"].toString();
        resp["success"] = true;
        resp["data"] = m_server->getSeats(fn);
        resp["message"] = "获取成功";
        return resp;
    }

    if (action == "getAllOrders" && method == "POST") {
        resp["success"] = true;
        resp["data"] = m_server->getOrders(req);
        resp["message"] = "获取成功";
        return resp;
    }

    if (action == "addFlight" && method == "POST") {
        bool ok = m_server->addFlight(req);
        resp["success"] = ok;
        resp["message"] = ok ? "添加航班成功" : "添加航班失败";
        return resp;
    }

    if (action == "updateFlight" && method == "POST") {
        bool ok = m_server->updateFlight(req);
        resp["success"] = ok;
        resp["message"] = ok ? "更新航班成功" : "更新航班失败";
        return resp;
    }

    if (action == "deleteFlight" && method == "POST") {
        QString fn = req["flight_num"].toString();
        bool ok = m_server->deleteFlight(fn);
        resp["success"] = ok;
        resp["message"] = ok ? "删除航班成功" : "删除航班失败";
        return resp;
    }

    if (action == "addSeat" && method == "POST") {
        bool ok = m_server->addSeat(req);
        resp["success"] = ok;
        resp["message"] = ok ? "添加座位成功" : "添加座位失败";
        return resp;
    }

    if (action == "updateSeat" && method == "POST") {
        bool ok = m_server->updateSeat(req);
        resp["success"] = ok;
        resp["message"] = ok ? "更新座位成功" : "更新座位失败";
        return resp;
    }

    if (action == "batchUpdateSeats" && method == "POST") {
        QJsonArray list = req["seat_list"].toArray();
        QString st = req["status"].toString();
        bool ok = m_server->batchUpdateSeats(list, st);
        resp["success"] = ok;
        resp["message"] = ok ? "批量更新成功" : "批量更新失败";
        return resp;
    }
    // 新增：获取航班登机口信息
    if (action == "getFlightGate" && method == "POST") {
        QString flight_num = req["flight_num"].toString();
        QString gate = m_server->getFlightGate(flight_num);
        QString status = m_server->getFlightDelayStatus(flight_num);

        QJsonObject data;
        data["gate"] = gate.isEmpty() ? "未分配" : gate;
        data["delay_status"] = status;

        resp["success"] = true;
        resp["data"] = data;
        resp["message"] = "获取成功";
        return resp;
    }

    // 新增：修改航班登机口信息
    if (action == "updateFlightGate" && method == "POST") {
        QString flight_num = req["flight_num"].toString(); // 接收航班号参数
        QString newGate = req["newGate"].toString(); // 接收新的登机口参数
        bool ok = m_server->updateFlightGate(flight_num, newGate); // 调用服务器的修改登机口方法
        resp["success"] = ok;
        resp["message"] = ok ? "修改登机口成功" : "修改登机口失败（航班不存在）";
        return resp;
    }

    resp["success"] = false;
    resp["message"] = "未知接口";
    return resp;
}
