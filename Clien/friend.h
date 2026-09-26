#ifndef FRIEND_H
#define FRIEND_H
// 好友管理页面头文件：好友列表管理、查找用户、在线用户、删除好友、发起聊天
#include "chat.h"
#include "onlineuser.h"

#include <QWidget>

namespace Ui {
class Friend;
}

class Friend : public QWidget
{
    Q_OBJECT

public:
    explicit Friend(QWidget *parent = nullptr);
    ~Friend();
    OnlineUser* m_pOnlineUser;           // 在线用户弹窗
    Chat* m_pChat;                       // 聊天窗口
    void flushFriend();                  // 向服务器请求刷新好友列表
    void updateFriend(QStringList userList); // 用服务器返回的好友列表更新UI
    QListWidget* getFirend_LW();         // 获取好友ListWidget（供分享文件时复制好友列表用）


private slots:
    void on_flushFriend_PB_clicked();    // 手动刷新好友列表
    void on_findUser_PB_clicked();       // 查找用户（输入用户名查询在线状态）
    void on_onlineUser_PB_clicked();     // 显示在线用户列表
    void on_delFriend_PB_clicked();      // 删除选中的好友
    void on_chat_PB_clicked();           // 与选中的好友开始聊天

private:
    Ui::Friend *ui;
};

#endif // FRIEND_H
