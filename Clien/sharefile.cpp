#include "client.h"
#include "index.h"
#include "sharefile.h"
#include "ui_sharefile.h"

#include <QMessageBox>
// 分享文件对话框实现：选择好友、确认分享

// 构造函数：初始化UI
ShareFile::ShareFile(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ShareFile)
{
    ui->setupUi(this);
}

ShareFile::~ShareFile()
{
    delete ui;
}

// 从好友管理页面的好友列表中复制所有好友名到分享对话框
void ShareFile::updateFirend_LW()
{
    ui->listWidget->clear();                                            // 清空当前列表
    QListWidget* friend_LW = Index::getInstance().getFriend()->getFirend_LW(); // 获取好友页面的ListWidget
    for (int i = 0; i < friend_LW->count() ; i++) {                     // 遍历好友列表
        QListWidgetItem* friendItem = friend_LW->item(i);                // 获取原好友项
        QListWidgetItem* newItem = new QListWidgetItem(*friendItem);     // 拷贝构造新项
        ui->listWidget->addItem(newItem);                                // 添加到分享对话框
    }
}

// 全选按钮：选中所有好友
void ShareFile::on_allSelect_PB_clicked()
{
    for(int i = 0; i < ui->listWidget->count(); i++){
        ui->listWidget->item(i)->setSelected(true);                      // 逐项设为选中
    }
}

// 取消全选按钮：取消所有好友的选中状态
void ShareFile::on_cancelSelect_PB_clicked()
{
    for(int i = 0; i < ui->listWidget->count(); i++){
        ui->listWidget->item(i)->setSelected(false);                     // 逐项取消选中
    }
}

// 确认分享按钮：构建分享PDU，caMsg前半存好友名列表，后半存文件路径
void ShareFile::on_ok_PB_clicked()
{
    QString strCurName = Client::getInstance().m_strUserName;            // 分享者用户名
    QString strCurPath = Index::getInstance().getFile()->m_strCurPath;   // 文件所在目录
    QString strShaerFileName = Index::getInstance().getFile()->m_strShareFileName; // 要分享的文件名
    QString strPath = QString("%1/%2").arg(strCurPath).arg(strShaerFileName); // 完整文件路径
    QList<QListWidgetItem*> pItems = ui->listWidget->selectedItems();    // 获取所有选中的好友

    int iFriendNum = pItems.size();                                      // 选中的好友数量
    PDU* pdu = mkPDU(iFriendNum*32+strPath.toStdString().size()+1);     // caMsg = 好友名列表(iFriendNum*32字节) + 文件路径
    pdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_REQUEST;
   // memcpy(pdu->caData, strCurName.toStdString().c_str(),32);            // caData前32字节：分享者名
    strncpySafe(pdu->caData, 32,strCurName.toStdString().c_str());
    memcpy(pdu->caData+32, &iFriendNum,sizeof(int));                     // caData后4字节：好友数量（用于接收端偏移计算）

    for (int i = 0;i < iFriendNum; i++) {                                // 逐个写入好友名（每个32字节）
        //memcpy(pdu->caMsg+i*32,pItems.at(i)->text().toStdString().c_str(),32);
        strncpySafe(pdu->caMsg+i*32, 32,pItems.at(i)->text().toStdString().c_str());

    }
    memcpy(pdu->caMsg+iFriendNum*32,strPath.toStdString().c_str(),strPath.toStdString().size()); // 写入文件路径
    Client::getInstance().sendMsg(pdu);                                  // 发送给服务器处理分享
}
