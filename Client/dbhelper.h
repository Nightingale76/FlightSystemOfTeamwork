#ifndef DBHELPER_H
#define DBHELPER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <functional>

// 前置声明回调包装器
struct CallbackWrapper;

class DBHelper : public QObject
{
    Q_OBJECT

public:
    static DBHelper* getInstance() {
        static DBHelper instance;
        return &instance;
    }
    void getUserPayInfo(const QString &userPhone,
                        std::function<void(bool,const QJsonObject&)> callback);
    static QString userPhone;   // 当前登录用户手机号
    // 更新航班登机口（真正发 HTTP）
    void updateFlightGate(const QString& flight_num,
                          const QString& newGate,
                          std::function<void(bool success)> callback);
    // 必须确保有这个声明！
    void adminLogin(const QString& adminName, const QString& adminPhone, const QString& password,
                    std::function<void(bool, const QString&, const QJsonObject&)> callback);
    // 发送HTTP请求到服务器
    void sendRequest(const QString& action, const QJsonObject& data,
                     std::function<void(const QJsonObject&)> callback = nullptr);

    // 管理员登录方法
    void login(const QString& username, const QString& password,
               std::function<void(bool, const QString&, const QJsonObject&)> callback);

    // 用户登录方法
    void userLogin(const QString& phone, const QString& password,
                   std::function<void(bool, const QString&, const QJsonObject&)> callback);
    /* 加到 public 区域，不要写 DBHelper:: */
    void submitComplaint(const QJsonObject& data, std::function<void(bool, const QString&)> callback);
    void getAllComplaints(std::function<void(bool, const QJsonArray&)> callback);
    void submitMealOrder(const QJsonObject& data, std::function<void(bool, const QString&)> callback);
    void getAllMealOrders(std::function<void(bool, const QJsonArray&)> callback);
    // 用户注册
    // 新增 nickname 参数，传递给后端
    void userRegister(const QString& phone, const QString& password, const QString& nickname,
                      std::function<void(bool success, const QString& message)> callback);

    // 重置密码
    void userResetPassword(const QString& phone, const QString& newPassword,
                           std::function<void(bool, const QString&)> callback);

    // 查询航班数据
    void queryFlights(std::function<void(bool, const QJsonArray&)> callback = nullptr);
    // 新增：兼容原有逻辑的航班查询（带搜索+排序）
    void queryFlightsWithFilterAndSort(const QString& flightNum, const QString& sortField,
                                       std::function<void(bool, const QJsonArray&)> callback);

    // 用户查询航班
    void queryFlightsForUser(const QString& departure, const QString& arrival, const QString& date,
                             std::function<void(bool, const QJsonArray&)> callback = nullptr);

    // 查询用户数据
    void queryUsers(std::function<void(bool, const QJsonArray&)> callback = nullptr);

    // 查询座位数据
    void querySeats(const QString& flightNum = "", std::function<void(bool, const QJsonArray&)> callback = nullptr);

    // 查询订单数据
    void queryOrders(const QJsonObject& filters = QJsonObject(), std::function<void(bool, const QJsonArray&)> callback = nullptr);

    // 获取用户订单
    void getUserOrdersByPhone(const QString& phone, std::function<void(bool, const QJsonArray&)> callback = nullptr);

    // 添加航班
    void addFlight(const QJsonObject& flightData, std::function<void(bool, const QString&)> callback = nullptr);

    // 更新航班
    void updateFlight(const QJsonObject& flightData, std::function<void(bool, const QString&)> callback = nullptr);

    // 删除航班
    void deleteFlight(const QString& flightNum, std::function<void(bool, const QString&)> callback = nullptr);

    // 添加座位
    void addSeat(const QJsonObject& seatData, std::function<void(bool, const QString&)> callback = nullptr);

    // 更新座位
    void updateSeat(const QJsonObject& seatData, std::function<void(bool, const QString&)> callback = nullptr);

    // 批量更新座位状态
    void batchUpdateSeats(const QJsonArray& seatList, const QString& status,
                          std::function<void(bool, const QString&)> callback = nullptr);

    // 创建用户订单
    void createUserOrder(const QJsonObject& orderData,
                         std::function<void(bool, const QString&, const QString&)> callback);
    //                                         新增 orderId ↑
    // 选座
    void selectSeat(const QString& flightNum, const QString& seatNum, const QString& passengerPhone,
                    std::function<void(bool, const QString&)> callback = nullptr);
    // dbhelper.h 中 DBHelper 类
    void getFlightGate(const QString& flight_num, std::function<void(QString)> callback);

    // 提交改签/退票申请
    void submitModifyRequest(const QJsonObject& data,
                             std::function<void(bool, const QString&)> callback);
    void getAllModifyApplications(std::function<void(bool success, const QJsonArray& applies)> callback);
    void processModifyApplication(const QString& applyId, const QString& status, const QString& reason,
                                  std::function<void(bool success, const QString& message)> callback);
    void queryModifyApplications(const QJsonObject& filters,
                                 std::function<void(bool success, const QJsonArray& applies)> callback);


private:
    DBHelper(QObject* parent = nullptr);
    ~DBHelper();

    QNetworkAccessManager* m_networkManager;
    QString m_serverUrl;

private slots:
    void onReplyFinished(QNetworkReply* reply);
};

#endif // DBHELPER_H
