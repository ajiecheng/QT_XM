#include "server.h"
#include "operatedb.h"
#include <QApplication>
// 服务器程序入口

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);              // 创建Qt应用程序实例（服务器也需要事件循环）
    Server::getInstance();                     // 初始化单例服务器：加载配置并启动TCP监听
    //OperateDb::getInstance().connectDb();      // 连接MySQL数据库
    return a.exec();                           // 进入Qt事件循环，等待并处理客户端连接
}
