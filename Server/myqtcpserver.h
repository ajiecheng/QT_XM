
/* #ifndef MYQTCPSERVER_H
#define MYQTCPSERVER_H

// TCP服务器类：监听客户端连接，管理所有在线客户端socket，转发消息
#include <QObject>
#include <QTcpServer>
#include <QThreadPool>
#include "mytcpsocket.h"
class MyQTcpServer : public QTcpServer
{
public:
    static MyQTcpServer& getInstance();  // 单例模式获取实例
    QThreadPool threadPool;                              // 线程池：为每个客户端分配独立线程处理消息
    void incomingConnection(qintptr handle) override;    // 重写：客户端连接到达时创建socket并分配线程

    void deleteSocket(MyTcpSocket* mysocket);            // 移除并销毁断开的客户端socket
    void resend(char* caTarName,PDU* pdu);               // 将消息转发给指定目标用户
private:
    MyQTcpServer();
    MyQTcpServer(const MyQTcpServer& instance)=delete;
    MyQTcpServer& operator=(const MyQTcpServer&)=delete;
    QList <MyTcpSocket*>m_socketList;                    // 维护所有在线客户端的socket列表
};

*/
#ifndef MYQTCPSERVER_H
#define MYQTCPSERVER_H
#include <QObject>
#include <QTcpServer>
#include <QMutex>
#include <QThread>                 // ← 新增
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

    static const int kThreadCount = 8;          // ← 固定 8 个工作线程
    QThread m_workerThreads[kThreadCount];      // ← 线程池本体
    int     m_nextIndex = 0;                    // ← 轮询分配用的下标

    QMutex m_socketListMutex;        // 保护m_socketList的并发访问
    QList <MyTcpSocket*>m_socketList;
};
#endif
