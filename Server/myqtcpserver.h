#ifndef MYQTCPSERVER_H
#define MYQTCPSERVER_H
#include <QObject>
#include <QTcpServer>
#include <QMutex>
#include <QThread>                
#include "mytcpsocket.h"

class MyQTcpServer : public QTcpServer
{
public:
    void incomingConnection(qintptr handle) override;
    static MyQTcpServer& getInstance();
    void deleteSocket(MyTcpSocket* mysocket);   // 移除断开连接（不改socket生命周期）
    void resend(char* caTarName,PDU* pdu);

    ~MyQTcpServer();                // ← 新增

private:
    MyQTcpServer();
    MyQTcpServer(const MyQTcpServer& instance)=delete;
    MyQTcpServer& operator=(const MyQTcpServer&)=delete;

    static const int kThreadCount = 8;          // 固定 8 个工作线程
    QThread m_workerThreads[kThreadCount];     
    int     m_nextIndex = 0;                    // 轮询分配用的下标

    QMutex m_socketListMutex;        // 保护m_socketList的并发访问
    QList <MyTcpSocket*>m_socketList;
};
#endif
