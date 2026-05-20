#ifndef FOODSELECTION_H
#define FOODSELECTION_H

#include <QWidget>

namespace Ui {
class foodselection;
}

class foodselection : public QWidget
{
    Q_OBJECT

public:
    explicit foodselection(QWidget *parent = nullptr);
    ~foodselection();

private:
    Ui::foodselection *ui;
};

#endif // FOODSELECTION_H
