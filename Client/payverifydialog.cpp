#include "payverifydialog.h"
#include "ui_payverifydialog.h"

PayVerifyDialog::PayVerifyDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::PayVerifyDialog)
{
    ui->setupUi(this);
    setWindowTitle("支付验证");

    // 如果 designer 里已经填过，这里可省略
    ui->payMethodCombo->addItems({"微信", "支付宝"});
    ui->payPwdEdit->setEchoMode(QLineEdit::Password);
    ui->payPwdEdit->setMaxLength(6);

    // 信号自动连 OK/Cancel
    connect(ui->okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

PayVerifyDialog::~PayVerifyDialog() { delete ui; }

QString PayVerifyDialog::realName()    const { return ui->nameEdit->text().trimmed(); }
QString PayVerifyDialog::payMethod()   const { return ui->payMethodCombo->currentText(); }
QString PayVerifyDialog::payPassword() const { return ui->payPwdEdit->text(); }
