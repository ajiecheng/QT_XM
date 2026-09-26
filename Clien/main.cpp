#include "client.h"

#include <QApplication>
// 客户端程序入口

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);          // 创建Qt应用程序实例
    Client::getInstance().show();         // 初始化单例客户端，显示登录/注册界面
    return a.exec();                      // 进入Qt事件循环
}
