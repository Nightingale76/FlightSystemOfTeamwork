#ifndef USER2_H
#define USER2_H

#include <QMainWindow>
#include <QSqlQueryModel>
#include <QJsonArray>

class MainWindow;

namespace Ui {
class myMainWindow;
}

class myMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit myMainWindow(MainWindow* preWindow, QWidget *parent = nullptr);
    ~myMainWindow();
    void showFlightResults(QSqlQueryModel *model);
    void showFlightResultsFromJson(const QJsonArray& flights);  // 添加这行
    QString getWeekdayString(const QDate &date);

private slots:
    void on_pushButton_clicked();

private:
    Ui::myMainWindow *ui;
    MainWindow *m_prevWindow;
    QString m_selectedFlightNo;
    QString m_selectedDepartureTime;
};

#endif // USER2_H
