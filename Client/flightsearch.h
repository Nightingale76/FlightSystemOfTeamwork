#ifndef FLIGHTSEARCH_H
#define FLIGHTSEARCH_H

#include <QWidget>
class MainWindow;
namespace Ui {
class flightsearch;
}

class flightsearch : public QWidget
{
    Q_OBJECT

public:
    explicit flightsearch(MainWindow* preWindow,QWidget *parent = nullptr);
    ~flightsearch();
    MainWindow* prepage;

private slots:
    void on_back_clicked();
    void on_queryBtn_clicked();

    void on_pushButton_clicked();

private:
    Ui::flightsearch *ui;
};

#endif // FLIGHTSEARCH_H
