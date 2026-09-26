#include "chat.h"
#include "client.h"
#include "ui_chat.h"
// 聊天窗口实现：消息发送

// 构造函数：初始化UI
Chat::Chat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Chat)
{
    ui->setupUi(this);
}

// 在聊天显示区域追加一条消息（纯文本）
void Chat::updageShow_TE(QString strMsg)
{
    ui->show_TE->append(strMsg);              // 在QTextEdit中追加文本
}

Chat::~Chat()
{
    delete ui;
}

// 发送按钮：构建聊天请求PDU（caData=发送者+接收者，caMsg=消息内容）
void Chat::on_send_PB_clicked()
{
    QString strCurName = Client::getInstance().m_strUserName; // 当前登录用户名
    QString strMsg = ui->msg_LE->text();                      // 获取输入框中的消息文本
    ui->msg_LE->clear();                                      // 清空输入框
    PDU* pdu = mkPDU(strMsg.toStdString().size()+1);          // 按消息长度+1(\\0)分配空间
    pdu->uiMsgType = ENUM_MSG_TYPE_CHAT_REQUEST;              // 消息类型：聊天请求
    //memcpy(pdu->caData,strCurName.toStdString().c_str(),32);        // caData前32字节：发送者用户名
    //memcpy(pdu->caData+32,m_strChatName.toStdString().c_str(),32); // caData后32字节：接收者用户名
    strncpySafe(pdu->caData, 32,strCurName.toStdString().c_str());
    strncpySafe(pdu->caData+32, 32,m_strChatName.toStdString().c_str());
    memcpy(pdu->caMsg,strMsg.toStdString().c_str(),strMsg.toStdString().size()); // caMsg：消息正文
    Client::getInstance().sendMsg(pdu);                        // 通过客户端单例发送给服务器

}
