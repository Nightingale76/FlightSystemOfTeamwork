#ifndef SEATSELECTION_H
#define SEATSELECTION_H

#include <QWidget>
#include <QMap>
#include <QString>
#include <QMessageBox>
#include <QSqlQuery>

class MainWindow;  // 前向声明

namespace Ui {
class SeatSelection;
}

class SeatSelection : public QWidget
{
    Q_OBJECT

public:
    explicit SeatSelection(MainWindow* preWindow, QWidget *parent = nullptr);
    ~SeatSelection();

    void setPassengerName(const QString &name);
    void loadSeatDataFromDB(const QString &flightNum);
    void setPassengerByPhone(const QString &phone);

    // 新增：刷新座位图
    void refreshSeatMap();

private slots:
    void onSeatSelected(int row, char column);
    void onConfirmSelection();
    void onBackButtonClicked();
void onSearchFlight();   // 搜索航班

    //void on_searchBtn_clicked();

private:
    void setupSeatMap();
    void createSeatButtons();
    void updateSelectedSeatDisplay();

    // 成员变量（只保留一份，删除重复的定义）
    Ui::SeatSelection *ui;
    MainWindow* prepage;

    // 乘客信息
    QString currentUsername;  // 用户名
    QString currentPassenger; // 乘客姓名
    QString Phone;

    // 航班信息
    QString currentFlightNo;
    QString currentDepartureTime;
    // seatselection.h  private:
    QPushButton *lastSelectedBtn = nullptr;
    // 座位信息
    QString selectedSeat;
    QHash<QString, bool> seatOccupancy;
    QStringList occupiedSeats;
};

#endif // SEATSELECTION_H
