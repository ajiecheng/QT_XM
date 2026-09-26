#ifndef SERVER_H
#define SERVER_H
// 服务器主类头文件：管理服务器配置和全局状态
#include <QWidget>

class Server : public QWidget
{
    Q_OBJECT

public:
    ~Server();
    static Server& getInstance();   // 单例模式获取服务器实例
    void loadConfig();              // 从配置文件加载IP、端口、根路径
    QString m_strIP;                // 服务器监听IP地址
    quint16 m_usPort;               // 服务器监听端口号
    QString m_strRootPath;          // 用户文件的根存储路径
private:
    Server(QWidget *parent = nullptr);
    Server(const Server& instance)=delete;           // 禁止拷贝构造
    Server& operator=(const Server&)=delete;         // 禁止拷贝赋值

};
#endif // SERVER_H
