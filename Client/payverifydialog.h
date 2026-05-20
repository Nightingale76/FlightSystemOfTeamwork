#ifndef PAYVERIFYDIALOG_H
#define PAYVERIFYDIALOG_H

#include <QDialog>

namespace Ui {
class PayVerifyDialog;
}

class PayVerifyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PayVerifyDialog(QWidget *parent = nullptr);
    ~PayVerifyDialog();

    QString realName()    const;
    QString payMethod()   const;
    QString payPassword() const;

private:
    Ui::PayVerifyDialog *ui;
};

#endif // PAYVERIFYDIALOG_H
