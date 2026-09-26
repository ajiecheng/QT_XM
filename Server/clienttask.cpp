#include "clienttask.h"

#include <QThread>
// 客户端任务实现：将socket移到工作线程，建立该线程内的信号槽连接

ClientTask::ClientTask()
{

}

// 构造函数：保存要处理的客户端socket
ClientTask::ClientTask(MyTcpSocket *socket)
{
    m_socket = socket;
}

// 线程池入口函数：在新线程中绑定信号槽，使数据处理在该线程中执行
void ClientTask::run()
{
    // 在新线程中关联信号与槽（直接连接，不跨线程排队）
    connect(m_socket,&QTcpSocket::readyRead,m_socket,&MyTcpSocket::recvMsg);
    connect(m_socket,&QTcpSocket::disconnected,m_socket,&MyTcpSocket::clientoffline);
    m_socket->moveToThread(QThread::currentThread());  // 将socket对象移到当前工作线程
}
