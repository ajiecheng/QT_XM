#ifndef CLIENTTASK_H
#define CLIENTTASK_H
// 客户端任务类：将每个客户端连接封装为可在线程池中执行的任务
#include "mytcpsocket.h"

#include <QObject>
#include <QRunnable>

class ClientTask : public QObject,public QRunnable  // 双重继承：QObject用于信号槽，QRunnable用于线程池
{
    Q_OBJECT
public:
    ClientTask();
    ClientTask(MyTcpSocket* socket);   // 构造函数：绑定要处理的客户端socket
    MyTcpSocket* m_socket;             // 指向该任务所管理的客户端socket
    void run() override;               // 线程池入口：在新线程中绑定socket的信号槽连接


signals:

};

#endif // CLIENTTASK_H
