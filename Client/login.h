#ifndef LOGIN_H
#define LOGIN_H

#include <QMainWindow>
#include "user1.h"
#include "adminwindow.h"
namespace Ui {
class login;
}

class login : public QMainWindow
{
    Q_OBJECT

public:
    explicit login(QWidget *parent = nullptr);
    ~login();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_4_clicked();
    void on_adminLoginBtn_clicked();

    void on_adminPwdVisCheck_checkStateChanged(const Qt::CheckState &arg1);

    void on_userPwdVisCheck_checkStateChanged(const Qt::CheckState &arg1);

private:
    Ui::login *ui;
};

#endif // LOGIN_H
