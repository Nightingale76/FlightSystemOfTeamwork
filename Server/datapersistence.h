#ifndef DATAPERSISTENCE_H
#define DATAPERSISTENCE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// 前向声明
struct UserInfo;
struct AdminInfo;
struct FlightInfo;
struct FlightSeat;
struct BookingOrder;

class DataPersistence : public QObject
{
    Q_OBJECT

public:
    static DataPersistence& getInstance();

    // 保存数据
    bool saveAllData(const QList<UserInfo>& users,
                     const QList<AdminInfo>& admins,
                     const QList<FlightInfo>& flights,
                     const QList<FlightSeat>& seats,
                     const QList<BookingOrder>& orders);

    // 加载数据
    bool loadAllData(QList<UserInfo>& users,
                     QList<AdminInfo>& admins,
                     QList<FlightInfo>& flights,
                     QList<FlightSeat>& seats,
                     QList<BookingOrder>& orders,
                     int& nextUserId, int& nextAdminId,
                     int& nextFlightId, int& nextSeatId, int& nextOrderId);

    // 获取数据文件路径
    QString getDataFilePath() const;

private:
    DataPersistence(QObject* parent = nullptr);
    ~DataPersistence();

    QString m_dataDir;
    QString m_dataFile;

    // JSON转换方法
    QJsonObject userToJson(const UserInfo& user);
    QJsonObject adminToJson(const AdminInfo& admin);
    QJsonObject flightToJson(const FlightInfo& flight);
    QJsonObject seatToJson(const FlightSeat& seat);
    QJsonObject orderToJson(const BookingOrder& order);

    UserInfo jsonToUser(const QJsonObject& json);
    AdminInfo jsonToAdmin(const QJsonObject& json);
    FlightInfo jsonToFlight(const QJsonObject& json);
    FlightSeat jsonToSeat(const QJsonObject& json);
    BookingOrder jsonToOrder(const QJsonObject& json);
};

#endif // DATAPERSISTENCE_H
