#include "friend.h"
#include "ui_friend.h"
#include "QDebug"
#include "protocol.h"
#include "client.h"
#include <QInputDialog>
#include <QMessageBox>
// 好友管理页面实现：好友列表、查找用户、在线用户、删除好友、发起聊天

// 构造函数：初始化子窗口，加载好友列表
Friend::Friend(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Friend)
{
    ui->setupUi(this);
    m_pOnlineUser = new OnlineUser;      // 创建在线用户弹窗
    m_pChat = new Chat;                  // 创建聊天窗口
    flushFriend();                       // 初始化时请求好友列表
}

// 析构：释放子窗口和UI
Friend::~Friend()
{
    delete m_pOnlineUser;
    delete m_pChat;
    delete ui;
}

// 刷新好友列表：向服务器发送请求
void Friend::flushFriend()
{
    QString strCurName = Client::getInstance().m_strUserName; // 获取当前用户名
    PDU* pdu = mkPDU();                                       // 空消息体，名称放caData
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST;
    //memcpy(pdu->caData,strCurName.toStdString().c_str(),32);  // caData存当前用户名
    strncpySafe(pdu->caData, 32,strCurName.toStdString().c_str());
    Client::getInstance().sendMsg(pdu);                        // 发送请求
}

// 更新好友列表UI：清空后重新填充
void Friend::updateFriend(QStringList userList)
{
    ui->listWidget->clear();
    ui->listWidget->addItems(userList);                        // 直接添加所有好友名
}

// 获取好友列表控件指针（供ShareFile复制好友列表用）
QListWidget *Friend::getFirend_LW()
{
    return ui->listWidget;
}

// 查找用户按钮：弹出输入框获取用户名，发送查找请求
void Friend::on_findUser_PB_clicked()
{
    QString strName =  QInputDialog::getText(this,"查找用户","用户名"); // 弹出输入对话框
    if(strName.isEmpty()){
        return;
    }
    qDebug()<<"查找用户 strName" << strName;
    PDU*pdu = mkPDU();                                       // 将用户名通过caData发送给服务器
    //memcpy(pdu->caData,strName.toStdString().c_str(),32);
    strncpySafe(pdu->caData, 32,strName.toStdString().c_str());
    pdu->uiMsgType = ENUM_MSG_TYPE_FIND_USER_REQUEST;
    Client::getInstance(). sendMsg(pdu);
}

// 在线用户按钮：显示在线用户弹窗，并请求最新在线列表
void Friend::on_onlineUser_PB_clicked()
{
    if(m_pOnlineUser->isHidden()){
        m_pOnlineUser->show();                               // 显示在线用户窗口
    }
    PDU*pdu = mkPDU();
    pdu->uiMsgType =ENUM_MSG_TYPE_ONLINE_USER_REQUEST;       // 请求在线用户列表
    Client::getInstance().sendMsg(pdu);
}

// 手动刷新好友按钮
void Friend::on_flushFriend_PB_clicked()
{
    flushFriend();
}

// 删除好友按钮：选中好友后发送删除请求
void Friend::on_delFriend_PB_clicked()
{
    QListWidgetItem* pItem = ui->listWidget->currentItem();  // 获取选中的好友
    if(!pItem){
        QMessageBox::information(this,"删除好友","请选择要删除的好友");
        return;
    }
    QString strTarName = pItem->text();                      // 目标好友用户名
    QString strCurName = Client::getInstance().m_strUserName; // 当前用户名
    PDU* pdu = mkPDU();
    pdu->uiMsgType = ENUM_MSG_TYPE_DEL_FRIEND_REQUEST;
    //memcpy(pdu->caData,strCurName.toStdString().c_str(),32);      // caData前32字节：当前用户
    //memcpy(pdu->caData+32,strTarName.toStdString().c_str(),32);   // caData后32字节：目标用户
    strncpySafe(pdu->caData, 32,strCurName.toStdString().c_str());
    strncpySafe(pdu->caData+32, 32,strTarName.toStdString().c_str());
    Client::getInstance().sendMsg(pdu);
}

// 聊天按钮：选中好友后打开聊天窗口
void Friend::on_chat_PB_clicked()
{
    QListWidgetItem* pItem = ui->listWidget->currentItem();  // 获取选中的好友
    if(!pItem){
        QMessageBox::information(this,"开始聊天","请选择聊天好友");
        return;
    }
    if(m_pChat->isHidden()){
        m_pChat->show();                                     // 显示聊天窗口
    }
    m_pChat->m_strChatName = pItem->text();                  // 设置聊天对象名
}
