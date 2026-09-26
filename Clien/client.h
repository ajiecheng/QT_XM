#ifndef CLIENT_H
#define CLIENT_H
// 客户端主类头文件：管理TCP连接、配置加载、消息收发和路由/#include "protocol.h"
#include "reshandler.h"

#include <QTcpSocket>
#include <QWidget>
#include <QHostAddress>
#include <QPointer>

QT_BEGIN_NAMESPACE
namespace Ui { class Client;}//前置声明：告诉编译器有一个叫 Client 的类，定义在 Ui 命名空间中
class Uploader;
QT_END_NAMESPACE

class Client : public QWidget
{
    Q_OBJECT

public:
    ~Client();
    static Client& getInstance();   // 单例模式获取客户端实例
    void loadConfig();              // 从配置文件加载服务器IP、端口、用户根路径

    QTcpSocket m_socket;            // 与服务器通信的TCP socket
    ResHandler* m_prh;              // 响应处理器：处理服务器返回的各种响应

    QString m_strUserName;          // 当前登录用户名
    void sendMsg(PDU*pdu);          // 发送PDU消息到服务器并释放内存
    PDU* readMsg();                 // 从socket缓冲区读取一条完整PDU消息
    void handleMsg(PDU* pdu);       // 根据消息类型路由到ResHandler处理
    QString m_strRootPath;          // 用户在云盘上的根路径
    QByteArray buffer;              // 接收缓冲区：用于TCP粘包处理

    void startUpload();             // 启动文件上传流程（创建Uploader并开始上传）
    qint64 m_iUploadOffset;   // 当前上传的起始偏移（断点续传用）
    void showError(QString msg);    // 弹出错误/提示消息框

    void flushWriteBuffer();         // 尝试把发送缓冲里剩余的字节继续写入socket
    QByteArray m_writeBuffer;        // 发送缓冲：write()未写完时暂存剩余字节

    QPointer<Uploader> m_pUploader;   // 当前上传器（弱引用，deleteLater后自动置null）

public slots:
    void showConnect();             // 槽：连接服务器成功时打印日志
    void on_regist_PB_clicked();    // 槽：点击注册按钮，发送注册请求
    void recvMsg();                 // 槽：收到服务器数据时触发，粘包处理后分发
    void on_login_PB_clicked();     // 槽：点击登录按钮，发送登录请求
    void onDisconnected();//断开监听

private:
    Ui::Client *ui;                 // Qt Designer生成的UI对象

    Client(QWidget *parent = nullptr);
    Client(const Client& instance)=delete;           // 禁止拷贝构造（单例模式）
    Client& operator=(const Client&)=delete;         // 禁止拷贝赋值（单例模式）

    QString m_strIP;                // 服务器IP地址（从配置文件读取）
    quint16 m_usPort;               // 服务器端口号（从配置文件读取）

};
#endif // CLIENT_H
