#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>

class FlightServer;

class Clientthread : public QObject
{
    Q_OBJECT
public:
    explicit Clientthread(qintptr socketDescriptor, FlightServer* server, QObject* parent = nullptr);

public slots:
    void process(); // 线程入口
    void readyRead();
    void disconnected();

signals:
    void finished();

private:
    qintptr m_socketDescriptor;
    FlightServer* m_server;
    QTcpSocket* m_socket;

    void sendJsonResponse(int statusCode, const QString& statusText, const QJsonObject& json);
    QJsonObject processRequest(const QString& path, const QString& method, const QJsonObject& requestData);
};
#endif // CLIENTHANDLER_H
