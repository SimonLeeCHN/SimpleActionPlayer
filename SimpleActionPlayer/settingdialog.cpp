#include "settingdialog.h"
#include "ui_settingdialog.h"
#include <QtSerialPort/QSerialPortInfo>
#include "QMessageBox"
#include "mainwindow.h"

SettingDialog::SettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingDialog)
{
    ui->setupUi(this);



    /*      TAB1        */
    this->fillAvaliablePorts();

}

SettingDialog::~SettingDialog()
{
    delete ui;
}

void SettingDialog::fillAvaliablePorts()
{
    ui->CB_PortName->clear();

    QList<QSerialPortInfo> _infos = QSerialPortInfo::availablePorts();
    for(const QSerialPortInfo &info : _infos)
    {
        ui->CB_PortName->addItem(info.portName());
    }
}

void SettingDialog::on_BTN_PortRefrush_clicked()
{
    this->fillAvaliablePorts();
}

/*
 *  TODO:通过检查ZIGBEE内部固定寄存器返回值检测是否链接成功
 */
void SettingDialog::on_BTN_ConnectDetect_clicked()
{

}

void SettingDialog::on_BTN_ConfirmSets_clicked()
{
    /*      TAB1        */
    //设置端口号
    QString _portName = ui->CB_PortName->currentText();
    if(!(_portName.isNull()))
    {
        emit RequestSetStationPort(_portName);

    }else
    {
        QMessageBox::warning(this,tr("端口设置错误"),tr("端口不得为空"));
        return;
    }



    //对话框隐藏
    this->setVisible(false);
}

void SettingDialog::on_BTN_CancelSets_clicked()
{

    //对话框隐藏
    this->setVisible(false);
}
