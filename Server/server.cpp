#include "myqtcpserver.h"
#include "server.h"
#include "connectionpool.h"
#include <QFile>
#include <QDebug>
// 服务器主类实现：配置加载、TCP监听启动

Server::Server(QWidget *parent): QWidget(parent)
{
    loadConfig();                                                          // 读取server.config获取IP和端口
    MyQTcpServer::getInstance().listen(QHostAddress(m_strIP),m_usPort);    // 启动TCP监听，等待客户端连接
    //ConnectionPool::getInstance().setMaxIdleMs(3000);
}

Server::~Server()
{

}

// 单例实现
Server &Server::getInstance()
{
    static Server instance;
    return instance;
}

// 加载server.config配置文件：IP地址，端口号，文件根路径
void Server::loadConfig()
{
    QFile file(":/server.config");                    // 从Qt资源系统读取配置文件
    if(file.open(QIODevice::ReadOnly)){                // 以只读方式打开文件
        QByteArray baData = file.readAll();            // 读取文件全部内容
        QString strData = QString(baData);             // 转为QString便于处理
        QStringList strList = strData.split("\r\n");   // 按Windows换行符分割各行

        m_strIP=strList.at(0);                         // 第一行：服务器IP地址
        m_usPort=strList.at(1).toUShort();             // 第二行：端口号，转为无符号短整型
        m_strRootPath = strList.at(2);                 // 第三行：用户文件在服务器上的存储根路径
        qDebug()<<"服务器启动，配置读取："<<"ip:"<< m_strIP <<"port:"<<m_usPort<<"RootPath"<<m_strRootPath;

        file.close();

    }
}



