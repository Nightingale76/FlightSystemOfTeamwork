#ifndef ADMINEDITPWD_H
#define ADMINEDITPWD_H

#include <QDialog>
#include <QMessageBox>
#include "dbhelper.h" // 假设你的网络请求工具类

namespace Ui {
class AdminEditPwd;
}

class AdminEditPwd : public QDialog
{
    Q_OBJECT

public:
    // 构造函数：接收当前管理员账号
    explicit AdminEditPwd(QString currentAdminAccount, QWidget *parent = nullptr);
    ~AdminEditPwd();

private slots:
    // 确认按钮点击事件
    void on_confirmBtn_clicked();
    // 取消按钮点击事件
    void on_cancelBtn_clicked();
    // 显示密码复选框状态变化
    void on_cBoxPwdVisable_toggled(bool checked);

private:
    Ui::AdminEditPwd *ui;
    QString m_currentAdminAccount; // 保存当前管理员账号
    // 输入验证函数
    bool validateInput();
};

#endif // ADMINEDITPWD_H
