#ifndef USER1_H
#define USER1_H

#include <QMainWindow>
#include "user2.h"
#include "seatselection.h"
#include "flightsearch.h"
#include "ticketrebook.h"
#include "login.h"
#include <QStandardItemModel>
#include <QTableView>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct FlightInfo {
    int id;
    QString flightNumber;
    QString airline;
    QString departureCity;
    QString arrivalCity;
    QDateTime departureTime;
    QDateTime arrivalTime;
    double price;
    int seatsAvailable;
    QDateTime createdAt;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setwelcome(const QString &phone,const QString &nickname);
    QString currentname;//实则是手机号
    QString passengername;
    QString flight_num;
    QString passengerIdCard;

private slots:
    void on_searchButton_clicked();
    void on_seatselection_clicked();  // 确保拼写正确
    void on_foodselection_clicked();
    void on_changeticket_2_clicked();
    void on_idcertify_clicked();
    void on_changeticket_clicked();
    void on_advice_clicked();

    void on_pushButton_clicked();

    void on_my_order_clicked();

private:
    // 添加 loadUserFlightsForSeatSelection 函数的声明
    void loadUserFlightsForSeatSelection();
    QString m_currentFlightNo;      // 当前选中的航班号
    QString m_currentDepartureTime; // 当前选中的起飞时间
    QStandardItemModel *flightModel;
    QString m_selectedFlightNo;      // 用户点中的航班号
    QString m_selectedDepartureTime; // 对应的起飞时间
    // 查询相关函数
    QList<FlightInfo> searchFlights(const QString &departure,
                                    const QString &arrival,
                                    const QDateTime &queryDateTime);
    void displayFlightResults(const QList<FlightInfo> &flights);
    void verifyDatabaseData();
    bool connectToDatabase();

    Ui::MainWindow *ui;
};

#endif // USER1_H
