#include "client.h"
#include "onlineuser.h"
#include "ui_onlineuser.h"
// 在线用户弹窗实现：显示在线用户并支持双击添加好友

// 构造函数：初始化UI
OnlineUser::OnlineUser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OnlineUser)
{
    ui->setupUi(this);
}

OnlineUser::~OnlineUser()
{
    delete ui;
}

// 更新在线用户列表：清空并填充新的在线用户名
void OnlineUser::updateOnlineUser(QStringList userList)
{
    ui->listWidget->clear();                 // 清空旧列表
    ui->listWidget->addItems(userList);      // 填充新数据
}

// 双击在线用户：发送添加好友请求
void OnlineUser::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString strTarName = item->text();                               // 目标用户名（被双击的在线用户）
    QString strCurName = Client::getInstance().m_strUserName;        // 当前登录用户名
    PDU* pdu = mkPDU();                                              // 创建空消息体PDU
    pdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;               // 消息类型：添加好友请求
    //memcpy(pdu->caData,strCurName.toStdString().c_str(),32);         // caData前32字节：发起者
    //memcpy(pdu->caData+32,strTarName.toStdString().c_str(),32);      // caData后32字节：被添加者
    strncpySafe(pdu->caData, 32,strCurName.toStdString().c_str());
    strncpySafe(pdu->caData+32, 32,strTarName.toStdString().c_str());
    Client::getInstance().sendMsg(pdu);                               // 发送给服务器处理

}
