#ifndef CHAT_H
#define CHAT_H
// 聊天窗口头文件：与好友进行一对一聊天
#include <QWidget>

namespace Ui {
class Chat;
}

class Chat : public QWidget
{
    Q_OBJECT

public:
    explicit Chat(QWidget *parent = nullptr);
    QString m_strChatName;                   // 当前聊天对象用户名
    void updageShow_TE(QString strMsg);      // 在聊天显示区追加一条消息
    ~Chat();

private slots:
    void on_send_PB_clicked();               // 发送按钮：构建聊天PDU并发送给服务器

private:
    Ui::Chat *ui;
};

#endif // CHAT_H
