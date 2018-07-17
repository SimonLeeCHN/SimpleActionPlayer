#include "Carrier.h"
#include "QMessageBox"

#define INFORM_NUM              4
#define INFORM_CARMODEL_COLUMN  0           //车辆模型号
#define INFORM_CARPOS_COLUMN    1           //车辆位置
#define INFORM_CARSPEED_COLUMN  2           //车辆速度
#define INFORM_CARSTATUS_COLUMN 3           //车辆状态

#define CARSTATUS_ERROR         0x01
#define CARSTATUS_STANDBY       0x02
#define CARSTATUS_RUNNING        0x03

/*  Translate Map   */
QMap<QString,int> map_StatusCmd =
{
    {"错误",0x01},{"待机",0x02},{"运动中",0x03}
};

QMap<QString,int> map_SpeedCmd =
{
    {"低速",0x01},{"中低速",0x02},{"中速",0x03},{"中高速",0x04},{"高速",0x05}
};

QMap<QString,int> map_SpeedcurveCmd =
{
    {"S曲线",0x01},{"定常曲线",0x02}
};

QMap<QString,int> map_CarenableCmd =
{
    {"启用",0x01},{"停用",0x02}
};

QMap<QString,int> map_ControlCmd =
{
    {"急停",0x01},{"复位",0x02}
};

int ConvertStringToCmd(QMap<QString, int> &map, QString str)
{
    return map.value(str);
}

QString ConvertCmdToString(QMap<QString, int> &map, int val)
{
    return map.key(val);
}



Carrier::Carrier(int carrierNum, QObject *parent) : QStandardItemModel(carrierNum,INFORM_NUM,parent)
{
    //设置表头
    QStringList modelHeader={tr("Model"),tr("Position"),tr("Speed"),tr("Status")};
    this->setHorizontalHeaderLabels(modelHeader);

    m_pHeartbeatTimer = new QTimer();
    m_iCarrierNum = carrierNum;

    //初始化心跳包记录表
    for (int i = 0;i < m_iCarrierNum;i++)
    {
        m_HeartbeatRecordList << true;
    }

    //初始化数据
    InitModelData();

    //开始心跳包
    //StartHeartbeatTimer();

}

Carrier::~Carrier()
{
    if(m_pHeartbeatTimer->isActive())
        m_pHeartbeatTimer->stop();

    delete m_pHeartbeatTimer;
}

void Carrier::BandViewer(QTableView *viewerpoint)
{
    //绑定模型与视图
    viewerpoint->setModel(this);
}

void Carrier::InitModelData()
{
    //从文件读或者默认值

    //默认值
    for(int column = 0;column < INFORM_NUM;column++)
    {
        for(int row = 0;row < m_iCarrierNum;row++)
        {
            QModelIndex index = this->index(row,column);
            switch (column) 
            {
                case INFORM_CARMODEL_COLUMN:        //型号
                    this->setData(index,QVariant(tr("M-80")));
                    break;
                case INFORM_CARPOS_COLUMN:          //位置
                    this->setData(index,QVariant(0));
                    break;
                case INFORM_CARSPEED_COLUMN:        //速度
                    this->setData(index,QVariant(tr("高速")));
                    break;
                case INFORM_CARSTATUS_COLUMN:       //状态
                    this->setData(index,QVariant(tr("待机")));
                    break;
                default:
                    break;
            }
        }
    }

    //设置图标
    for(int row = 0;row < m_iCarrierNum;row++)
    {
        QStandardItem* tempItem = this->item(row);
        tempItem->setIcon(QIcon(":/img/carrier_standby"));
    }
}

void Carrier::StartHeartbeatTimer()
{
    connect(m_pHeartbeatTimer,&QTimer::timeout,this,&Carrier::OnHeartbeatTimeup);
    m_pHeartbeatTimer->start(HEARTBEAT_TIME);
}

void Carrier::StopHearbeatTimer()
{
    disconnect(m_pHeartbeatTimer,&QTimer::timeout,this,&Carrier::OnHeartbeatTimeup);
    m_pHeartbeatTimer->stop();
}

int Carrier::IsAllCarrierStatusSame(QString status)
{
    //检测是否每台载体车都是处于同样的status
    //正常返回0 否则返回顺序第一个不正常车辆编号

    int index =0;
    for(int i = 0;i < m_iCarrierNum;i++)
    {
        QString temp = this->data(this->index(i,INFORM_CARSTATUS_COLUMN)).toString();
        if(temp != status)
        {
            index = i+1;
            break;
        }
    }
    return index;
}

void Carrier::GetCarrierConfig()
{
    //TODO:获取载体车配置

    QList<QByteArray> requestList;
    for(int i = 0;i < m_iCarrierNum;i++)
    {
        //先发送一个车辆号给stationport用于辨识打包
        //再按照协议添加一个车辆号

        QByteArray temp;
        temp.append(i + 1);
        temp.append(i+1);
        requestList << temp;
    }
    emit RequestSendPackageData(requestList,PORT_CONFIG_REQUEST);

}

void Carrier::SetCarrierConfig()
{
    //TODO:设置载体车配置

    //要求每台载体车处于待机状态
    int erroCarrierNum = IsAllCarrierStatusSame("待机");
    if(erroCarrierNum != 0)
    {
        emit RequestPrintDebugMessage("Not all carrier stoped!");
        return;
    }

    //将每个载体车的配置转义后放入list内发送出去
    QList<QByteArray> configList;

    for(int i = 0;i < m_iCarrierNum;i++)
    {
        QByteArray temp;

        //for stationport
        temp.append(i+1);

        //carrier number
        temp.append(i+1);

        //speed
        temp.append(ConvertStringToCmd(map_SpeedCmd,this->data(this->index(i,INFORM_CARSPEED_COLUMN)).toString()));

        //speedcurve
        temp.append(ConvertStringToCmd(map_SpeedcurveCmd,"S曲线"));

        //enable
        temp.append(ConvertStringToCmd(map_CarenableCmd,"启用"));

        configList << temp;
    }
    emit RequestSendPackageData(configList,PORT_CONFIG_SET);
}

void Carrier::CarrierEntiretyControl(QString strCmd)
{
    //TODO 载体车整体控制命令   急停  复位

    QList<QByteArray> controlList;
    for(int i = 0; i< m_iCarrierNum;i++)
    {
        //为stationport添加车辆号用于辨识
        //再添加命令

        QByteArray tempCmd;
        tempCmd.append(i+1);
        tempCmd.append(ConvertStringToCmd(map_ControlCmd,strCmd));
        controlList << tempCmd;
    }
    emit RequestSendPackageData(controlList,PORT_CONTROL_SEND);
}


/*      SLOT    */

void Carrier::OnHeartbeatTimeup()
{
    //TODO: 查询上一轮是否所有载体车都有心跳回应记录
    for(int i = 0; i < m_iCarrierNum;i++)
    {
        if(m_HeartbeatRecordList.at(i) == false)
        {
            //发现 i+1 号车未回馈心跳包

            //debug print
            QString str = "Carrier " + QString::number(i+1) + " no heartbeat back!";
            emit RequestPrintDebugMessage(str);

#if ENABLE_HEARTBEAT_ERROR_OPTION

            //紧急停车
            CarrierEntiretyControl("急停");
#endif

        }
    }



    //心跳查询
    QList<QByteArray> tempList;
    for(int i = 0;i < m_iCarrierNum;i++)
    {
        //先为stationport加入车辆号辨识，再加数据体

        QByteArray tempArray;
        tempArray.append(i+1);
        tempArray.append(i+1);
        tempList << tempArray;
    }

    emit RequestSendPackageData(tempList,PORT_HEARTBEAT_SEND);

    //清除回应记录
    for (int i = 0;i < m_iCarrierNum;i++)
    {
        m_HeartbeatRecordList[i] = false;
    }

}

void Carrier::OnSetCarrierStatus(int carNum, int stu, int pos)
{
    //根据心跳包的回馈进行载体车状态设置

    //判断车辆号
    if((carNum < 0) || (m_iCarrierNum < carNum))
    {
        emit RequestPrintDebugMessage(QString(tr("载体车号错误 %1")).arg(QString::number(carNum)));
        return;
    }

    //设置状态
    if(stu > 0)
    {
        QModelIndex tempIndex = this->index(carNum - 1,INFORM_CARSTATUS_COLUMN);
        this->setData(tempIndex,QVariant(ConvertCmdToString(map_StatusCmd,stu)));

        //修改状态图标
        QStandardItem* tempItem = this->item(carNum - 1);
        switch (stu)
        {
            case CARSTATUS_ERROR:
            {
                tempItem->setIcon(QIcon(":/img/carrier_error"));
                break;
            }
            case CARSTATUS_STANDBY:
            {
                tempItem->setIcon(QIcon(":/img/carrier_standby"));
                break;
            }
            case CARSTATUS_RUNNING:
            {
                tempItem->setIcon(QIcon(":/img/carrier_running"));
                break;
            }
        }
    }
    else
    {
        //有效性判断错误
        emit RequestPrintDebugMessage(QString(tr("载体车 %1 未定义的状态")).arg(QString::number(carNum)));
    }

    //设置位置
    if(pos >= 0)
    {
        QModelIndex tempIndex = this->index(carNum - 1,INFORM_CARPOS_COLUMN);
        this->setData(tempIndex,QVariant((unsigned int)(pos)));
    }
    else
    {
        //有效性判断错误
        emit RequestPrintDebugMessage(QString(tr("载体车 %1 错误的路径点 %2").arg(QString::number(carNum)).arg(QString(QString::number(pos)))));
    }

#if  ENABLE_HEARTBEAT_ERROR_OPTION

    //若任何车发回了错误状态，则发送紧急停车
    bool compStatu = false;
    compStatu = (QString("错误") == ConvertCmdToString(map_StatusCmd,stu));
    if(compStatu == true)
    {
        //若状态为 错误 则进行停车操作 其他状态不进行操作
        CarrierEntiretyControl("急停");
    }

#endif

    //记录心跳包
    m_HeartbeatRecordList[carNum - 1] = true;

    //TODO:检查是否所有载体车都处于待机状态，若是发送信号给mainwindow,用以判断是否触发doNextStep


}

void Carrier::OnSetCarrierProfile(QByteArray config)
{
    //TODO:根据回馈进行载体车配置项的设置
    int carNum = config.at(0);

    //运行速度
    this->setData(this->index(carNum - 1,INFORM_CARSPEED_COLUMN),ConvertCmdToString(map_SpeedCmd,config.at(1)));
}
