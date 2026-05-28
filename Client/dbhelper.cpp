#include "dbhelper.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
QString DBHelper::userPhone = "";
// 定义回调包装器结构
struct CallbackWrapper {
    std::function<void(const QJsonObject&)> callback;
};

DBHelper::DBHelper(QObject* parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);

    // 从配置文件或环境变量读取服务器地址，默认为localhost
    m_serverUrl = qEnvironmentVariable("FLIGHT_SERVER_URL", "http://192.168.137.1:8080");
    //m_serverUrl = qEnvironmentVariable("FLIGHT_SERVER_URL", "http://localhost:8080");

    // 或者使用配置文件
    // QSettings settings;
    // m_serverUrl = settings.value("server/url", "http://localhost:8080").toString();

    qDebug() << "连接服务器:" << m_serverUrl;

    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &DBHelper::onReplyFinished);
}

DBHelper::~DBHelper()
{
    if (m_networkManager) {
        m_networkManager->deleteLater();
    }
}
// 实现adminLogin接口：向服务器发送管理员登录请求（姓名+电话+密码）
void DBHelper::adminLogin(const QString& adminName, const QString& adminPhone, const QString& password,
                          std::function<void(bool, const QString&, const QJsonObject&)> callback)
{
    // 1. 构造请求参数
    QJsonObject requestData;
    requestData["username"] = adminName;   // 管理员姓名
    requestData["admin_phone"] = adminPhone; // 管理员电话
    requestData["password"] = password;      // 管理员密码

    // 2. 调用sendRequest（假设你已有这个辅助函数，负责发送HTTP请求）
    sendRequest("adminLogin", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        QJsonObject adminInfo = response["data"].toObject();
        // 3. 回调返回结果
        if (callback) {
            callback(success, message, adminInfo);
        }
    });
}
// 1. 获取航班登机口
void DBHelper::getFlightGate(const QString& flight_num, std::function<void(QString)> callback) {
    QJsonObject json;
    json["flight_num"] = flight_num; // 适配你的航班号字段名

    // 替换为你DBHelper中实际的请求发送函数（比如sendRequest）
    sendRequest("/api/getFlightGate", json, [callback](const QJsonObject& response) {
        if (response["code"].toInt() == 200) {
            callback(response["data"].toString());
        } else {
            callback("");
        }
    });
}

// 2. 修改航班登机口
void DBHelper::updateFlightGate(const QString& flight_num,
                                const QString& newGate,
                                std::function<void(bool success)> callback)
{
    QJsonObject req;
    req["flight_num"] = flight_num;
    req["newGate"]    = newGate;

    sendRequest("updateFlightGate", req,
                [callback](const QJsonObject& resp) {
                    bool ok = resp["success"].toBool();
                    callback(ok);
                });
}

void DBHelper::userLogin(const QString& phone, const QString& password,
                         std::function<void(bool, const QString&, const QJsonObject&)> callback)
{
    QJsonObject requestData;
    requestData["phone"] = phone;
    requestData["password"] = password;

    sendRequest("userLogin", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        QJsonObject userInfo = response["data"].toObject();
        if (callback) {
            callback(success, message, userInfo);
        }
    });
}

void DBHelper::userRegister(const QString& phone, const QString& password, const QString& nickname,
                            std::function<void(bool, const QString&)> callback)
{
    QJsonObject requestData;
    requestData["phone"] = phone;
    requestData["password"] = password;
    requestData["nickname"] = nickname; // 传递昵称参数

    sendRequest("userRegister", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        if (callback) {
            callback(success, message);
        }
    });
}


void DBHelper::submitComplaint(const QJsonObject& data,
                               std::function<void(bool, const QString&)> callback)
{
    sendRequest("submitComplaint", data, [callback](const QJsonObject& resp) {
        if (callback) callback(resp["success"].toBool(), resp["message"].toString());
    });
}

void DBHelper::getAllComplaints(std::function<void(bool, const QJsonArray&)> callback)
{
    sendRequest("getAllComplaints", QJsonObject(), [callback](const QJsonObject& resp) {
        if (callback) callback(resp["success"].toBool(), resp["data"].toArray());
    });
}

void DBHelper::submitMealOrder(const QJsonObject& data,
                               std::function<void(bool, const QString&)> callback)
{
    sendRequest("submitMealOrder", data, [callback](const QJsonObject& resp) {
        if (callback) callback(resp["success"].toBool(), resp["message"].toString());
    });
}

void DBHelper::getAllMealOrders(std::function<void(bool, const QJsonArray&)> callback)
{
    sendRequest("getAllMealOrders", QJsonObject(), [callback](const QJsonObject& resp) {
        if (callback) callback(resp["success"].toBool(), resp["data"].toArray());
    });
}

void DBHelper::userResetPassword(const QString& phone, const QString& newPassword,
                                 std::function<void(bool, const QString&)> callback)
{
    QJsonObject requestData;
    requestData["phone"] = phone;
    requestData["newPassword"] = newPassword;

    sendRequest("userResetPassword", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        if (callback) {
            callback(success, message);
        }
    });
}

void DBHelper::queryFlightsForUser(const QString& departure, const QString& arrival, const QString& date,
                                   std::function<void(bool, const QJsonArray&)> callback)
{
    QJsonObject requestData;
    if (!departure.isEmpty()) requestData["departure"] = departure;
    if (!arrival.isEmpty()) requestData["arrival"] = arrival;
    if (!date.isEmpty()) requestData["date"] = date;

    sendRequest("queryFlightsForUser", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QJsonArray flights = response["data"].toArray();
        if (callback) {
            callback(success, flights);
        }
    });
}

void DBHelper::sendRequest(const QString& action, const QJsonObject& data,
                           std::function<void(const QJsonObject&)> callback)
{
    QUrl url(m_serverUrl + "/" + action);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument doc(data);
    QByteArray postData = doc.toJson();

    qDebug() << "发送请求到:" << url.toString() << "数据:" << data;

    // 发送请求
    QNetworkReply* reply = m_networkManager->post(request, postData);

    // 使用动态分配的回调包装器
    if (callback) {
        CallbackWrapper* wrapper = new CallbackWrapper{callback};
        reply->setProperty("callbackWrapper", QVariant::fromValue<void*>(static_cast<void*>(wrapper)));
    }
    reply->setProperty("action", action);
}

void DBHelper::login(const QString& username, const QString& password,
                     std::function<void(bool, const QString&, const QJsonObject&)> callback)
{
    QJsonObject requestData;
    requestData["username"] = username;
    requestData["password"] = password;

    sendRequest("adminLogin", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        QJsonObject userInfo = response["data"].toObject();
        if (callback) {
            callback(success, message, userInfo);
        }
    });
}

void DBHelper::queryFlights(std::function<void(bool, const QJsonArray&)> callback)
{
    QJsonObject requestData;
    sendRequest("getAllFlights", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QJsonArray flights = response["data"].toArray();
        if (callback) {
            callback(success, flights);
        }
    });
}

// 实现：本地过滤+排序
void DBHelper::queryFlightsWithFilterAndSort(const QString& flightNum, const QString& sortField,
                                             std::function<void(bool, const QJsonArray&)> callback)
{
    // 第一步：先调用原有接口获取所有航班
    this->queryFlights([=](bool success, QJsonArray flights) {
        if (!success) {
            callback(false, QJsonArray());
            return;
        }

        // 第二步：前端过滤（航班编号模糊匹配）
        if (!flightNum.isEmpty()) {
            QJsonArray filteredFlights;
            for (const QJsonValue& val : flights) {
                QJsonObject flight = val.toObject();
                QString num = flight["flight_num"].toString();
                if (num.contains(flightNum, Qt::CaseInsensitive)) { // 不区分大小写
                    filteredFlights.append(flight);
                }
            }
            flights = filteredFlights;
        }

        // 第三步：前端排序（按选择的字段）- 兼容 Qt 6.9.3 版本
        if (!sortField.isEmpty()) {
            // 1. 将QJsonArray转换为QVector（兼容所有Qt版本）
            QVector<QJsonValue> flightVector;
            for (int i = 0; i < flights.size(); ++i) {
                flightVector.append(flights[i]); // 遍历方式兼容所有Qt版本
            }

            // 2. 对QVector进行排序（核心逻辑不变）
            std::sort(flightVector.begin(), flightVector.end(), [&](const QJsonValue& a, const QJsonValue& b) {
                QJsonObject objA = a.toObject();
                QJsonObject objB = b.toObject();

                // 根据字段类型选择排序方式
                if (sortField == "base_price") { // 价格（数值）
                    return objA[sortField].toDouble() < objB[sortField].toDouble();
                } else if (sortField == "create_time" || sortField == "departure_time" || sortField == "arrival_time") { // 时间
                    QDateTime timeA = QDateTime::fromString(objA[sortField].toString(), Qt::ISODate);
                    QDateTime timeB = QDateTime::fromString(objB[sortField].toString(), Qt::ISODate);
                    return timeA < timeB;
                } else { // 字符串（出发地、目的地、航空公司等）
                    return objA[sortField].toString() < objB[sortField].toString();
                }
            });

            // 3. 手动将排序后的QVector转回QJsonArray（替代fromVector，兼容Qt 6.9.3）
            flights = QJsonArray(); // 清空原数组
            for (const QJsonValue& val : flightVector) {
                flights.append(val); // 逐个添加排序后的元素
            }
        }

        // 第四步：返回处理后的结果
        callback(true, flights);
    });
}

void DBHelper::queryUsers(std::function<void(bool, const QJsonArray&)> callback)
{
    QJsonObject requestData;
    sendRequest("getAllUsers", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QJsonArray users = response["data"].toArray();
        if (callback) {
            callback(success, users);
        }
    });
}

void DBHelper::querySeats(const QString& flightNum, std::function<void(bool, const QJsonArray&)> callback)
{
    QJsonObject requestData;
    if (!flightNum.isEmpty()) {
        requestData["flight_num"] = flightNum;
    }

    sendRequest("getAllSeats", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QJsonArray seats = response["data"].toArray();
        if (callback) {
            callback(success, seats);
        }
    });
}

void DBHelper::queryOrders(const QJsonObject& filters, std::function<void(bool, const QJsonArray&)> callback)
{
    sendRequest("getAllOrders", filters, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QJsonArray orders = response["data"].toArray();
        if (callback) {
            callback(success, orders);
        }
    });
}

void DBHelper::getUserOrdersByPhone(const QString& phone, std::function<void(bool, const QJsonArray&)> callback)
{
    QJsonObject requestData;
    requestData["phone"] = phone;

    sendRequest("getUserOrdersByPhone", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QJsonArray orders = response["data"].toArray();
        if (callback) {
            callback(success, orders);
        }
    });
}

void DBHelper::addFlight(const QJsonObject& flightData, std::function<void(bool, const QString&)> callback)
{
    sendRequest("addFlight", flightData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        if (callback) {
            callback(success, message);
        }
    });
}

void DBHelper::updateFlight(const QJsonObject& flightData, std::function<void(bool, const QString&)> callback)
{
    sendRequest("updateFlight", flightData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        if (callback) {
            callback(success, message);
        }
    });
}

void DBHelper::deleteFlight(const QString& flightNum, std::function<void(bool, const QString&)> callback)
{
    QJsonObject requestData;
    requestData["flight_num"] = flightNum;

    sendRequest("deleteFlight", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        if (callback) {
            callback(success, message);
        }
    });
}

void DBHelper::addSeat(const QJsonObject& seatData, std::function<void(bool, const QString&)> callback)
{
    sendRequest("addSeat", seatData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        if (callback) {
            callback(success, message);
        }
    });
}

void DBHelper::updateSeat(const QJsonObject& seatData, std::function<void(bool, const QString&)> callback)
{
    sendRequest("updateSeat", seatData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        if (callback) {
            callback(success, message);
        }
    });
}

void DBHelper::batchUpdateSeats(const QJsonArray& seatList, const QString& status,
                                std::function<void(bool, const QString&)> callback)
{
    QJsonObject requestData;
    requestData["seat_list"] = seatList;
    requestData["status"] = status;

    sendRequest("batchUpdateSeats", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        if (callback) {
            callback(success, message);
        }
    });
}
void DBHelper::createUserOrder(const QJsonObject& orderData,
                               std::function<void(bool, const QString&, const QString&)> callback)
{
    sendRequest("createUserOrder", orderData,
                [callback](const QJsonObject& resp) {
                    bool success = resp["success"].toBool();
                    QString msg  = resp["message"].toString();
                    QString oid  = success ? resp["order_id"].toString() : "";
                    if (callback) callback(success, msg, oid);
                });
}

void DBHelper::selectSeat(const QString& flightNum, const QString& seatNum, const QString& passengerphone,
                          std::function<void(bool, const QString&)> callback)
{
    QJsonObject requestData;
    requestData["flight_num"] = flightNum;
    requestData["seat_num"] = seatNum;
    requestData["passenger_phone"] = passengerphone;

    sendRequest("selectSeat", requestData, [callback](const QJsonObject& response) {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        if (callback) {
            callback(success, message);
        }
    });
}
void DBHelper::onReplyFinished(QNetworkReply* reply)
{
    qDebug() << "收到网络响应";

    // 处理HTTP响应
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject response = doc.object();

        QString action = reply->property("action").toString();
        qDebug() << "收到响应:" << action << response;

        // 获取并调用回调函数
        QVariant callbackVar = reply->property("callbackWrapper");
        if (callbackVar.isValid()) {
            void* wrapperPtr = callbackVar.value<void*>();
            CallbackWrapper* wrapper = static_cast<CallbackWrapper*>(wrapperPtr);
            if (wrapper && wrapper->callback) {
                wrapper->callback(response);
            }
            // 清理包装器
            delete wrapper;
        }
    } else {
        qDebug() << "请求错误:" << reply->errorString();

        // 构建错误响应
        QJsonObject errorResponse;
        errorResponse["success"] = false;
        errorResponse["message"] = "网络请求失败: " + reply->errorString();

        // 调用回调函数通知错误
        QVariant callbackVar = reply->property("callbackWrapper");
        if (callbackVar.isValid()) {
            void* wrapperPtr = callbackVar.value<void*>();
            CallbackWrapper* wrapper = static_cast<CallbackWrapper*>(wrapperPtr);
            if (wrapper && wrapper->callback) {
                wrapper->callback(errorResponse);
            }
            // 清理包装器
            delete wrapper;
        }
    }

    reply->deleteLater();
}

void DBHelper::submitModifyRequest(const QJsonObject& data,
                                   std::function<void(bool, const QString&)> callback)
{
    sendRequest("submitModifyApplication", data, [callback](const QJsonObject& resp) {
        bool success = resp["success"].toBool();
        QString msg = resp["message"].toString();
        if(callback) callback(success, msg);
    });
}

// 获取所有退改申请（管理员接口）
void DBHelper::getAllModifyApplications(std::function<void(bool success, const QJsonArray& applies)> callback)
{
    // 调用服务器 getAllModifyApplications 接口，参数为空对象
    sendRequest("getAllModifyApplications", QJsonObject(), [callback](const QJsonObject& resp) {
        bool success = resp["success"].toBool();
        QJsonArray applies = resp["data"].toArray(); // 服务器返回的退改申请列表
        if (callback) callback(success, applies);
    });
}

// 处理退改申请（审核，管理员接口）
void DBHelper::processModifyApplication(const QString& applyId, const QString& status, const QString& reason,
                                        std::function<void(bool success, const QString& message)> callback)
{
    // 构造请求参数
    QJsonObject data;
    data["apply_id"] = applyId;
    data["status"] = status;
    data["reason"] = reason;

    // 调用服务器 processModifyApplication 接口
    sendRequest("processModifyApplication", data, [callback](const QJsonObject& resp) {
        bool success = resp["success"].toBool();
        QString msg = resp["message"].toString();
        if (callback) callback(success, msg);
    });
}

// void DBHelper::queryModifyApplications(const QJsonObject& filters,
//                                        std::function<void(bool success, const QJsonArray& applies)> callback)
// {
//     sendRequest("queryModifyApplications", filters, [callback](const QJsonObject& resp) {
//         bool success = resp["success"].toBool();
//         QJsonArray applies = resp["data"].toArray();
//         if (callback) callback(success, applies);
//     });
// }
void DBHelper::getUserPayInfo(const QString &userPhone,
                              std::function<void(bool,const QJsonObject&)> callback)
{
    QJsonObject req;
    req["user_phone"] = userPhone;
    sendRequest("getUserPayInfo", req,
                [callback](const QJsonObject &resp)
                {
                    bool ok = resp["success"].toBool();
                    QJsonObject data = resp["data"].toObject();
                    callback(ok, data);
                });
}
