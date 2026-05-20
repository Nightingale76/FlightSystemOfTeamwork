#ifndef TICKETREBOOK_H
#define TICKETREBOOK_H

#include <QWidget>
class MainWindow;
namespace Ui {
class ticketrebook;
}

class ticketrebook : public QWidget
{
    Q_OBJECT

public:
    explicit ticketrebook(MainWindow* preWindow,QWidget *parent = nullptr);
    ~ticketrebook();
    MainWindow* prepage;
    void processRefund(const QString &name, const QString &idCard, const QString &flight);
    void processRebook(const QString &name, const QString &idCard, const QString &flight, const QString &newFlight);
    void loadAvailableFlights();

private slots:
    void on_back_clicked();

    void on_pushButton_clicked();

    void on_comfirmbutton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::ticketrebook *ui;
};

#endif // TICKETREBOOK_H
