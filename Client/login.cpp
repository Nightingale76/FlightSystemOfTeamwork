#include "login.h"
#include "ui_login.h"
#include "user1.h"
#include "dbhelper.h"  // 添加这行
#include <QMessageBox>
#include <QInputDialog>
#include "adminwindow.h"   // 注意：这是管理员主界面
#include <QKeyEvent>
/*
 include "loginwindow.h"  // 新增这行，或者创建新的管理员登录逻辑
*/
login::login(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::login)
{
    ui->setupUi(this);
    connect(ui->adminLoginBtn, &QPushButton::clicked,
            this, &login::on_adminLoginBtn_clicked);
}

login::~login()
{
    delete ui;
}
void login::on_adminLoginBtn_clicked()
{
    QString name=ui->admin_name->text();
    QString phone=ui->admin_phone->text();
    QString passward=ui->admin_passward->text();
    if(name=="admin"&&passward=="123456"&&phone=="123456789")
    {
    AdminWindow *adminWin = new AdminWindow(name);
    adminWin->show();
    this->close();
    }    // 关闭普通用户登录窗口
    else
        QMessageBox::warning(this,"错误提示","用户名/密码/手机号错误");
}
void login::on_pushButton_clicked()
{
    QString phone = ui->phone->text().trimmed();
    QString password = ui->password->text().trimmed();
    QString nickname=ui->name->text();
    if (phone.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "手机号和密码不能为空！");
        return;
    }
    DBHelper::getInstance()->userLogin(phone, password,
                                       [this, phone, nickname](bool success, const QString& message, const QJsonObject& userInfo) {
                                           if (success) {
                                               // 检查用户是否被封禁
                                               if (userInfo["is_banned"].toBool()) {
                                                   QMessageBox::critical(this, "登录失败", "您的账号已被封禁，请联系管理员");
                                                   return;
                                               }
                                               // ✅ 把这行加上：缓存当前登录手机号
                                               DBHelper::userPhone = phone;

                                               // 跳转界面
                                               MainWindow* fpage = new MainWindow();
                                               fpage->setwelcome(phone, nickname);
                                               this->close();
                                               fpage->show();
                                           } else {
                                               QMessageBox::warning(this, "登录失败", message);
                                           }
                                       });

}

void login::on_pushButton_2_clicked()
{
    QString phone = ui->phone->text().trimmed();
    QString password = ui->password->text().trimmed();
    // 新增：获取昵称输入框内容（去除前后空格）
    QString nickname = ui->name->text().trimmed();

    // 校验手机号和密码不为空（昵称可选，无需校验）
    if(phone.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "手机号/密码不能为空！");
        return;
    }

    // 使用HTTP请求注册（新增传递昵称参数）
    DBHelper::getInstance()->userRegister(phone, password, nickname,
                                          [this](bool success, const QString& message) {
                                              if (success) {
                                                  QMessageBox::information(this, "注册成功", message);

                                              } else {
                                                  QMessageBox::warning(this, "注册失败", message);
                                              }
                                          });
}

void login::on_pushButton_4_clicked()
{
    QString phone = ui->phone->text().trimmed();

    if(phone.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入手机号!");
        return;
    }

    bool ok;
    QString newPassword = QInputDialog::getText(this, "重置密码", "请输入新密码:",
                                                QLineEdit::Password, "", &ok);

    if(ok && !newPassword.isEmpty()) {
        QString confirmPassword = QInputDialog::getText(this, "确认密码", "请再次输入新密码:",
                                                        QLineEdit::Password, "", &ok);

        if(ok) {
            if(newPassword == confirmPassword) {
                // 使用HTTP请求重置密码
                DBHelper::getInstance()->userResetPassword(phone, newPassword,
                                                           [this](bool success, const QString& message) {
                                                               if (success) {
                                                                   QMessageBox::information(this, "提示", message);
                                                                   ui->password->clear();
                                                               } else {
                                                                   QMessageBox::warning(this, "错误", message);
                                                               }
                                                           });
            } else {
                QMessageBox::warning(this, "错误", "两次输入的密码不一致!");
            }
        }
    }
}

// 在login.cpp中实现：
void login::keyPressEvent(QKeyEvent *event)
{
    // 1. Enter键触发登录（区分用户/管理员登录）
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        // 判断当前焦点在用户登录区域还是管理员区域，触发对应登录按钮
        QWidget *focusedWidget = focusWidget();
        if (focusedWidget == ui->name || focusedWidget == ui->phone || focusedWidget == ui->password) {
            on_pushButton_clicked(); // 触发用户登录按钮
        } else if (focusedWidget == ui->admin_name || focusedWidget == ui->admin_phone || focusedWidget == ui->admin_passward) {
            on_adminLoginBtn_clicked(); // 触发管理员登录按钮
        }
    }

    // 2. 处理上箭头：精准切换输入框焦点（只在输入框之间）
    else if (event->key() == Qt::Key_Up) {
        QWidget *focused = focusWidget();
        // ---- 用户登录区输入框上箭头切换 ----
        if (focused == ui->phone) {
            ui->name->setFocus(); // 手机号 → 用户名
        } else if (focused == ui->password) {
            ui->phone->setFocus(); // 密码 → 手机号
        }
        // ---- 管理员登录区输入框上箭头切换 ----
        else if (focused == ui->admin_phone) {
            ui->admin_name->setFocus(); // 管理员手机号 → 管理员用户名
        } else if (focused == ui->admin_passward) {
            ui->admin_phone->setFocus(); // 管理员密码 → 管理员手机号
        }
        // 可选：循环切换（比如用户名按上箭头切到密码）
        // else if (focused == ui->user_name) {
        //     ui->user_password->setFocus();
        // } else if (focused == ui->admin_name) {
        //     ui->admin_passward->setFocus();
        // }
    }
    // 3. 处理下箭头：精准切换输入框焦点（只在输入框之间）
    else if (event->key() == Qt::Key_Down) {
        QWidget *focused = focusWidget();
        // ---- 用户登录区输入框下箭头切换 ----
        if (focused == ui->name) {
            ui->phone->setFocus(); // 用户名 → 手机号
        } else if (focused == ui->phone) {
            ui->password->setFocus(); // 手机号 → 密码
        }
        // ---- 管理员登录区输入框下箭头切换 ----
        else if (focused == ui->admin_name) {
            ui->admin_phone->setFocus(); // 管理员用户名 → 管理员手机号
        } else if (focused == ui->admin_phone) {
            ui->admin_passward->setFocus(); // 管理员手机号 → 管理员密码
        }
        // 可选：循环切换（比如密码按下箭头切到用户名）
        // else if (focused == ui->user_password) {
        //     ui->user_name->setFocus();
        // } else if (focused == ui->admin_passward) {
        //     ui->admin_name->setFocus();
        // }
    }

    // 保留Qt默认的键盘事件处理（非上下箭头/Enter键的操作正常）
    else {
        QMainWindow::keyPressEvent(event);
    }
}

// 密码可见性切换（checkStateChanged信号的槽函数)
void login::on_adminPwdVisCheck_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        ui->admin_passward->setEchoMode(QLineEdit::Normal);
    } else {
        ui->admin_passward->setEchoMode(QLineEdit::Password);
    }
}


void login::on_userPwdVisCheck_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        ui->password->setEchoMode(QLineEdit::Normal);
    } else {
        ui->password->setEchoMode(QLineEdit::Password);
    }
}

