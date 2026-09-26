#ifndef ONLINEUSER_H
#define ONLINEUSER_H
// 在线用户弹窗头文件：显示所有在线用户，双击可添加好友
#include <QListWidget>
#include <QWidget>

namespace Ui {
class OnlineUser;
}

class OnlineUser : public QWidget
{
    Q_OBJECT

public:
    explicit OnlineUser(QWidget *parent = nullptr);
    ~OnlineUser();
    void updateOnlineUser(QStringList userList);        // 用服务器返回的在线用户列表更新UI

private slots:
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item); // 双击发送添加好友请求

private:
    Ui::OnlineUser *ui;
};

#endif // ONLINEUSER_H
