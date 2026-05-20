#include "foodselection.h"
#include "ui_foodselection.h"

foodselection::foodselection(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::foodselection)
{
    ui->setupUi(this);
}

foodselection::~foodselection()
{
    delete ui;
}
