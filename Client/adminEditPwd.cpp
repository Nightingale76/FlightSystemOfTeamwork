#include "AdminEditPwd.h"
#include "ui_AdminEditPwd.h"

// 构造函数：接收当前管理员账号
AdminEditPwd::AdminEditPwd(QString currentAdminAccount, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AdminEditPwd),
    m_currentAdminAccount(currentAdminAccount)
{
    ui->setupUi(this);
    setWindowTitle("修改登录密码");

    // 初始化密码框为隐藏模式
    ui->adminpwdEdit->setEchoMode(QLineEdit::Password);
    ui->pwdConfirmEdit->setEchoMode(QLineEdit::Password);
}

AdminEditPwd::~AdminEditPwd()
{
    delete ui;
}

// 输入验证逻辑
bool AdminEditPwd::validateInput()
{
    QString oldPwd = ui->adminNameEdit->text().trimmed(); // 原密码
    QString newPwd = ui->adminpwdEdit->text().trimmed();  // 新密码
    QString confirmPwd = ui->pwdConfirmEdit->text().trimmed(); // 确认密码

    // 非空校验
    if (oldPwd.isEmpty() || newPwd.isEmpty() || confirmPwd.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "所有字段不能为空！");
        return false;
    }

    // 密码一致性校验
    if (newPwd != confirmPwd) {
        QMessageBox::warning(this, "输入错误", "两次输入的新密码不一致！");
        return false;
    }/*

    // 密码长度校验
    if (newPwd.length() < 6) {
        QMessageBox::warning(this, "输入错误", "新密码长度不能少于6位！");
        return false;
    }*/

    return true;
}

// 确认按钮点击事件：核心修改逻辑
void AdminEditPwd::on_confirmBtn_clicked()
{
    // 先验证输入合法性
    if (!validateInput()) {
        return;
    }

    // 注意：这里需要服务器支持修改密码API，但目前服务器没有提供
    // 暂时显示提示信息
    QMessageBox::information(this, "提示", "修改密码功能需要服务器支持，当前版本暂不可用。", QMessageBox::Ok);
    // 获取输入的密码
    QString oldPwd = ui->adminNameEdit->text().trimmed();
    QString newPwd = ui->adminpwdEdit->text().trimmed();

    // // 发送修改请求到服务器
    // QJsonObject data;
    // data["admin_account"] = m_currentAdminAccount; // 当前管理员账号
    // data["old_password"] = oldPwd;
    // data["new_password"] = newPwd;

    // // 调用网络工具类发送请求
    // DBHelper::getInstance()->sendRequest("updateAdminPassword", data,
    //                                      [this](const QJsonObject& response) {
    //                                          if (response["success"].toBool()) {
    //                                              QMessageBox::information(this, "成功", "密码修改成功，请重新登录！");
    //                                              this->accept(); // 关闭对话框并返回成功状态
    //                                          } else {
    //                                              QMessageBox::critical(this, "失败", response["message"].toString());
    //                                          }
    //                                      });
}

// 取消按钮点击事件
void AdminEditPwd::on_cancelBtn_clicked()
{
    this->reject(); // 关闭对话框并返回取消状态
}

// 显示密码复选框切换
void AdminEditPwd::on_cBoxPwdVisable_toggled(bool checked)
{
    // 根据复选框状态切换密码可见性
    QLineEdit::EchoMode mode = checked ? QLineEdit::Normal : QLineEdit::Password;
    ui->adminpwdEdit->setEchoMode(mode);
    ui->pwdConfirmEdit->setEchoMode(mode);
}
