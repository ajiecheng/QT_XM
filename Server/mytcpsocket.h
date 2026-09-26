#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H
// 自定义TCP Socket类：网络通信的端点，封装消息收发、粘包处理、消息路由
#include "msghandler.h"
#include "protocol.h"

#include <QObject>
#include <QTcpSocket>
#include <QThread>

class MyTcpSocket : public QTcpSocket
{
    Q_OBJECT            // ← 加这一行
public:
    MyTcpSocket();
    QString m_strUserName;         // 该socket对应的登录用户名（未登录时为空）
    void sendMsg(PDU*pdu);         // 发送PDU消息并释放内存
    PDU* readMsg();                // 从socket缓冲区读取一条完整的PDU消息
    PDU* handleMsg(PDU* pdu);      // 根据消息类型路由到对应的MsgHandler处理函数
    MsgHandler* m_pmh;             // 消息处理器：执行业务逻辑（注册、登录、文件操作等）
    QByteArray buffer;             // 接收缓冲区：用于粘包处理（TCP流式传输需拼接数据）
    QThread* m_pThread;          // 该socket所属的工作线程


    ~MyTcpSocket();

public slots:
    void recvMsg();                // 槽函数：收到数据时触发，处理粘包并逐个派发消息
    void clientoffline();          // 槽函数：客户端断开时触发，更新数据库在线状态
    void forwardData(QByteArray data);   // 跨线程安全写：由resend投递到本线程执行

};

#endif // MYTCPSOCKET_H
