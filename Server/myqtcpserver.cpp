#include "myqtcpserver.h"

MyQTcpServer::MyQTcpServer()
{
    for (int i = 0; i < kThreadCount; ++i) {
        m_workerThreads[i].start();
    }
}

MyQTcpServer::~MyQTcpServer()
{
    for (int i = 0; i < kThreadCount; ++i) {
        m_workerThreads[i].quit();
        m_workerThreads[i].wait();
    }
}

void MyQTcpServer::incomingConnection(qintptr handle)
{
    qDebug()<<"客户端链接成功";
    MyTcpSocket* socket = new MyTcpSocket;         // 主线程new，affinity=主线程
    socket->setSocketDescriptor(handle);            // 主线程绑定描述符

    int index = m_nextIndex % kThreadCount;     // 先算出下标
    ++m_nextIndex;
    QThread* thread = &m_workerThreads[index];
    qDebug() << "客户端分配到线程" << index << "（下标 0~7）";   // ← 新增打印


    socket->m_pThread = thread;                     // 记录所属线程
    socket->moveToThread(thread);                   // 主线程调用，socket在主线程，符合规则

    QMutexLocker locker(&m_socketListMutex);   // 加锁，作用域结束自动解锁
    m_socketList.append(socket);                    // 主线程操作，安全
}

MyQTcpServer &MyQTcpServer::getInstance()
{
    static MyQTcpServer instance;
    return instance;
}

void MyQTcpServer::deleteSocket(MyTcpSocket *mysocket)
{
    QMutexLocker locker(&m_socketListMutex);   // 加锁，函数结束自动解锁
    m_socketList.removeOne(mysocket);
}
void MyQTcpServer::resend(char *caTarName, PDU *pdu)
{
    if(caTarName == NULL || pdu == NULL){
        return;
    }

    // 加锁遍历找目标socket；找到就退出作用域释放锁，不持锁做write
    MyTcpSocket* target = NULL;
    {
        QMutexLocker locker(&m_socketListMutex);
        for(int i = 0; i < m_socketList.size(); i++){
            if(m_socketList.at(i)->m_strUserName == QString(caTarName)){
                target = m_socketList.at(i);
                break;
            }
        }
    }

    // resend 里
    if(target == NULL){
        qDebug() << "resend: 目标" << caTarName << "不在线";
        return;
    }
    qDebug() << "resend: 找到目标" << caTarName;
    QByteArray data((const char*)pdu, pdu->uiPDULen);
    bool ok = QMetaObject::invokeMethod(target, "forwardData", Qt::QueuedConnection,
                                        Q_ARG(QByteArray, data));
    qDebug() << "resend: invokeMethod返回" << ok;

}
