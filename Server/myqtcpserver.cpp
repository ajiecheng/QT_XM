#include "myqtcpserver.h"

MyQTcpServer::MyQTcpServer()
{
    // 一次性启动固定 8 个工作线程。
    // QThread 默认的 run() 会进入事件循环(exec)，
    // socket 的 readyRead/disconnected 信号靠它在这些线程里派发。
    for (int i = 0; i < kThreadCount; ++i) {
        m_workerThreads[i].start();
    }
}

MyQTcpServer::~MyQTcpServer()
{
    // 优雅关闭：先让每个线程退出事件循环，再等它结束
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

    // 轮询挑一个已存在的线程，负载大致均匀（不再每客户端 new QThread）
    //QThread* thread = &m_workerThreads[m_nextIndex % kThreadCount];
   //++m_nextIndex;
    // 轮询挑一个已存在的线程，负载大致均匀（不再每客户端 new QThread）
    int index = m_nextIndex % kThreadCount;     // 先算出下标
    ++m_nextIndex;
    QThread* thread = &m_workerThreads[index];
    qDebug() << "客户端分配到线程" << index << "（下标 0~7）";   // ← 新增打印


    socket->m_pThread = thread;                     // 记录所属线程
    socket->moveToThread(thread);                   // 主线程调用，socket在主线程，符合规则

    QMutexLocker locker(&m_socketListMutex);   // 加锁，作用域结束自动解锁
    m_socketList.append(socket);                    // 主线程操作，安全

    // 删掉原来的三行：
    // connect(thread,&QThread::finished,thread,&QObject::deleteLater);
    // thread->start();
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
/*void MyQTcpServer::resend(char *caTarName, PDU *pdu)
{
    if(caTarName == NULL || pdu == NULL){
        return;
    }
    for(int i = 0; i<m_socketList.size();i++){
        if(caTarName == m_socketList.at(i)->m_strUserName){
            m_socketList.at(i)->write((char*)pdu,pdu->uiPDULen);
        }
    }
}
*/
/*




#include "clienttask.h"
#include "myqtcpserver.h"
// TCP服务器实现：客户端连接管理、消息转发

// 构造函数：设置线程池最大线程数为8（同时处理8个客户端连接）
MyQTcpServer::MyQTcpServer()
{
    threadPool.setMaxThreadCount(8);
}

// 重写入站连接处理：每个新客户端连接创建一个MyTcpSocket并提交到线程池
void MyQTcpServer::incomingConnection(qintptr handle)
{
    qDebug()<<"客户端链接成功";
    MyTcpSocket* pSocket =new MyTcpSocket;          // 为新连接创建自定义TCP socket对象
    pSocket->setSocketDescriptor(handle);            // 将底层socket描述符绑定到MyTcpSocket
    m_socketList.append(pSocket);                     // 加入到在线用户列表，用于后续消息转发
    ClientTask* task = new ClientTask(pSocket);       // 创建任务对象（封装socket）
    threadPool.start(task);                            // 将任务提交到线程池，在新线程中运行
}

// 单例实现：返回全局唯一的TCP服务器实例
MyQTcpServer &MyQTcpServer::getInstance()
{
    static MyQTcpServer instance;
    return instance;
}

// 移除断开的客户端连接：从列表中删除并延迟释放socket对象
void MyQTcpServer::deleteSocket(MyTcpSocket *mysocket)
{
    m_socketList.removeOne(mysocket);                 // 从在线客户端列表中移除
    mysocket->deleteLater();                           // 通过Qt事件循环延迟释放堆空间
    mysocket = NULL;                                   // 指针置空，防止悬空指针
}

// 消息转发：在在线客户端列表中查找目标用户并发送PDU数据
void MyQTcpServer::resend(char *caTarName, PDU *pdu)
{
    if(caTarName == NULL || pdu == NULL){              // 参数校验：目标和消息不能为空
        return;
    }
    for(int i = 0; i<m_socketList.size();i++){         // 遍历所有已连接的客户端
        if(caTarName == m_socketList.at(i)->m_strUserName){  // 按用户名匹配目标socket
            m_socketList.at(i)->write((char*)pdu,pdu->uiPDULen); // 将消息写入目标socket发送
        }
    }
}
*/
