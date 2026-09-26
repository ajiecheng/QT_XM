#include "client.h"
#include "index.h"
#include "protocol.h"
#include "ui_client.h"
#include "uploader.h"

#include <QFile>
#include <QMessageBox>
#include <QDebug>
// 客户端主类实现：配置加载、服务器连接、消息收发、UI事件处理

// 构造函数：初始化UI，加载配置，连接服务器，绑定信号槽
Client::Client(QWidget *parent) : QWidget(parent), ui(new Ui::Client)
{
    ui->setupUi(this);                                               // 初始化UI界面
    loadConfig();                                                    // 从配置文件读取服务器IP和端口
    m_socket.connectToHost(QHostAddress(m_strIP),m_usPort);          // 连接服务器（先连信号再连服务器防信号丢失）
    connect(&m_socket,&QTcpSocket::connected,this,&Client::showConnect); // 连接成功通知
    connect(&m_socket,&QTcpSocket::readyRead,this,&Client::recvMsg);     // 收到数据通知
    //1.谁发的信号2.发的什么信号 3.谁处理信号4.槽函数

    connect(&m_socket,&QTcpSocket::bytesWritten,this,&Client::flushWriteBuffer); // 数据被内核接受后，继续写剩余字节
    connect(&m_socket,&QTcpSocket::disconnected,this,&Client::onDisconnected);//断开监听信号连接


    m_prh = new ResHandler;                                          // 创建响应处理器
}

// 析构函数：释放UI和响应处理器
Client::~Client()
{
    delete ui;
    delete m_prh;
    m_prh = NULL;
}

// 单例实现：返回全局唯一的客户端实例
Client &Client::getInstance()
{
    static Client instance;
    return instance;
}


// 加载client.config配置文件：第1行服务器IP，第2行端口号，第3行用户根路径
void Client::loadConfig()
{
    QFile file(":/client.config");                     // 从Qt资源系统读取配置文件
    if(file.open(QIODevice::ReadOnly)){
        QByteArray baData =file.readAll();             // 读取全部内容
        QString strData = QString(baData);
        QStringList strList = strData.split("\r\n");   // 按行分割

        m_strIP=strList.at(0);                         // 第一行：服务器IP地址
        m_usPort=strList.at(1).toUShort();             // 第二行：端口号
        m_strRootPath = strList.at(2);                 // 第三行：用户云盘根路径
        qDebug()<<"项目启动，加载配置成功："<<"ip:"<< m_strIP <<"port:"<<m_usPort<<"RootPath"<<m_strRootPath;

        file.close();
    }else{
        QMessageBox::information(this,"打开配置文件","打开失败"); // 弹出配置打开失败提示
    }
}

// 注册按钮槽函数：从UI获取用户名密码，构建注册请求PDU并发送
void Client::on_regist_PB_clicked()
{
    QString strName = ui->name_LE->text();               // 从输入框获取用户名
    QString strPwd = ui->pwd_LE->text();                 // 从输入框获取密码
    if(strName.isEmpty()){                               // 用户名为空则不发送
        return;
    }
    PDU*pdu = mkPDU();                                   // 创建空消息体PDU
    //memcpy(pdu->caData,strName.toStdString().c_str(),32);      // caData前32字节：用户名
    //memcpy(pdu->caData+32,strPwd.toStdString().c_str(),32);   // caData后32字节：密码
    strncpySafe(pdu->caData, 32,strName.toStdString().c_str());
    strncpySafe(pdu->caData+32, 32,strPwd.toStdString().c_str());
    pdu->uiMsgType = ENUM_MSG_TYPE_REGIST_REQUEST;             // 消息类型：注册请求
    sendMsg(pdu);                                              // 发送到服务器
}

// 登录按钮槽函数：从UI获取用户名密码，构建登录请求PDU并发送
void Client::on_login_PB_clicked()
{
    QString strName = ui->name_LE->text();               // 从输入框获取用户名
    QString strPwd = ui->pwd_LE->text();                 // 从输入框获取密码
    m_strUserName = strName;                              // 保存当前用户名
    PDU*pdu = mkPDU();                                   // 创建空消息体PDU
    //memcpy(pdu->caData,strName.toStdString().c_str(),32);      // caData前32字节：用户名
    //memcpy(pdu->caData+32,strPwd.toStdString().c_str(),32);   // caData后32字节：密码
    strncpySafe(pdu->caData, 32,strName.toStdString().c_str());
    strncpySafe(pdu->caData+32, 32,strPwd.toStdString().c_str());
    pdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_REQUEST;              // 消息类型：登录请求
    sendMsg(pdu);                                              // 发送到服务器
}


// 发送PDU消息：通过TCP写入数据后释放内存
/*
void Client::sendMsg(PDU*pdu)
{
    if(pdu==NULL){
        return;
    }

    m_socket.write((char*)pdu,pdu->uiPDULen);          // 将PDU原始字节写入TCP socket发送
    qDebug()<< "send uiPDULen" << pdu->uiPDULen
            << "uiMsgLen" << pdu->uiMsgLen
            << "uiMsgType" << pdu->uiMsgType
            << "caData" << pdu->caData
            << "caData+32" << pdu->caData+32
            << "caMsg" <<pdu->caMsg;
    free(pdu);                                          // 发送后释放内存
    pdu=NULL;
}

*/
void Client::sendMsg(PDU*pdu)
{
    if(pdu==NULL){
        return;
    }

    qDebug()<< "send uiPDULen" << pdu->uiPDULen
            << "uiMsgLen" << pdu->uiMsgLen
            << "uiMsgType" << pdu->uiMsgType
            << "caData" << pdu->caData
            << "caData+32" << pdu->caData+32;
            //<< "caMsg" <<QByteArray(pdu->caMsg, int(pdu->uiMsgLen));

    // 把整块PDU的原始字节追加到发送缓冲（可能已有之前未写完的字节）
    m_writeBuffer.append((const char*)pdu, int(pdu->uiPDULen));
    free(pdu);                                           // 数据已拷进缓冲，可以释放PDU
    pdu=NULL;

    flushWriteBuffer();                                  // 尝试把缓冲里的数据写出去
}

// 把发送缓冲里的剩余字节继续写入socket；没写完就等bytesWritten信号再触发本函数
void Client::flushWriteBuffer()
{//新增缓冲
    if(m_writeBuffer.isEmpty()){
        return;
    }
    qint64 n = m_socket.write(m_writeBuffer);            // 返回实际被接受的字节数
    if(n < 0){                                           // 写失败（如socket已断开）
        m_writeBuffer.clear();                           // 丢弃积压数据，避免无限堆积
        return;
    }
    if(n > 0){
        m_writeBuffer.remove(0, int(n));                 // 移除已写出的部分
    }
    // n==0（缓冲满，一字节都没写进去）或仍有剩余：等bytesWritten信号再次触发
}


// 从socket缓冲区读取一条完整PDU消息：先读总长度，再读剩余数据
PDU *Client::readMsg()
{
    qDebug()<<"readMsg 读消息的长度"<<m_socket.bytesAvailable();
    unsigned int uiPDULen = 0;
    m_socket.read((char* ) &uiPDULen,sizeof(unsigned int)); // 读取4字节的PDU总长度

    PDU* pdu = mkPDU(uiPDULen-sizeof(PDU));                // 根据消息体长度分配空间
    m_socket.read((char*)pdu+sizeof(unsigned int),uiPDULen-sizeof(unsigned int)); // 读取剩余完整数据
    qDebug()<< "readMsg(接收回应) uiPDULen" << pdu->uiPDULen
            << "uiMsgLen" << pdu->uiMsgLen
            << "uiMsgType" << pdu->uiMsgType
            << "caData" << pdu->caData
            << "caData+32" << pdu->caData+32;
            //<< "caMsg" <<QByteArray(pdu->caMsg, int(pdu->uiMsgLen));
    return pdu;
}

// 根据消息类型路由到ResHandler对应的响应处理函数
void Client::handleMsg(PDU *pdu)
{
    qDebug()<< "handleMsg(接收回应) uiPDULen" << pdu->uiPDULen
            << "uiMsgLen" << pdu->uiMsgLen
            << "uiMsgType" << pdu->uiMsgType
            << "caData" << pdu->caData
            << "caData+32" << pdu->caData+32;
            //<< "caMsg" <<QByteArray(pdu->caMsg, int(pdu->uiMsgLen));
    switch (pdu->uiMsgType) {                             // 根据消息类型分派
    case ENUM_MSG_TYPE_REGIST_RESPOND:{                   // 注册响应：显示成功或失败
        m_prh ->regist(pdu);
        break;
    }
    case ENUM_MSG_TYPE_LOGIN_RESPOND:{                    // 登录响应：成功则跳转到主界面
        m_prh ->login(pdu);
        break;
    }
    case ENUM_MSG_TYPE_FIND_USER_RESPOND:{                // 查找用户响应：显示用户在线状态
        m_prh ->findUser(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ONLINE_USER_RESPOND:{              // 在线用户列表响应：更新在线用户窗口
        m_prh ->onlineUser(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND:{               // 添加好友响应：显示添加结果
        m_prh ->addFriend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:{               // 收到好友申请：弹出询问是否同意
        m_prh ->addFriendResend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_ARGEE_RESPOND:{         // 同意好友申请响应：刷新好友列表
        m_prh ->addFriendAgree(pdu);
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND:{             // 刷新好友列表响应：更新好友列表UI
        m_prh ->flushFriend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_DEL_FRIEND_RESPOND:{               // 删除好友响应：刷新好友列表
        m_prh ->delFriend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_CHAT_REQUEST:{                     // 收到聊天消息：显示在聊天窗口
        m_prh ->chat(pdu);
        break;
    }
    case ENUM_MSG_TYPE_MKDIR_RESPOND:{                    // 新建文件夹响应：刷新文件列表
        m_prh ->mkdir(pdu);
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FILE_RESPOND:{               // 刷新文件列表响应：更新文件列表UI
        m_prh ->flushFile(pdu);
        break;
    }
    case ENUM_MSG_TYPE_DEL_DIR_RESPOND:{                  // 删除文件夹响应：刷新文件列表
        m_prh ->deldir(pdu);
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_INIT_RESPOND:{         // 上传初始化响应：开始发送文件数据
        m_prh ->uploadFileInit(pdu);
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_DATA_RESPOND:{         // 上传完成响应：提示成功并刷新
        m_prh ->uploadFileData(pdu);
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_RESPOND:{               // 分享文件响应：提示分享成功
        m_prh ->shareFile(pdu);
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_ARGEE_RESPOND:{         // 接收分享文件通知：询问是否接收
        m_prh ->shareFileResend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_DATA_ACK:{         // 上传块确认：唤醒Uploader
        m_prh ->uploadFileAck(pdu);
        break;
    }



    default:
        break;

    }
}

// 启动文件上传：创建Uploader对象，连接信号槽，在子线程中执行上传
void Client::startUpload()
{
    //File* f = Index::getInstance().getFile();              // 获取文件页面实例
    //Uploader* uploader = new Uploader(f->m_strUploadFilePath); // 创建上传器，传入本地文件路径
    //m_pUploader = new Uploader(f->m_strUploadFilePath);   // 保存指针，供ACK唤醒
    //m_pUploader = new Uploader(f->m_strUploadFilePath, m_iUploadOffset);   // 传入起始偏移
    //connect(uploader,&Uploader::errorMsg,this,&Client::showError); // 错误消息→弹窗显示
    //connect(uploader,&Uploader::sendPDU,this,&Client::sendMsg);    // 发送PDU→通过Client发送
    //connect(uploader,&Uploader::progress,f,&File::updateProgress); // 进度信号→进度条
    //connect(uploader,&Uploader::finished,uploader,&QObject::deleteLater);
    //uploader->start();                                     // 启动上传线程
    File* f = Index::getInstance().getFile();
       m_pUploader = new Uploader(f->m_strUploadFilePath, m_iUploadOffset);   // 只创建一个
       connect(m_pUploader,&Uploader::errorMsg,this,&Client::showError);
       connect(m_pUploader,&Uploader::sendPDU,this,&Client::sendMsg);
       connect(m_pUploader,&Uploader::progress,f,&File::updateProgress);
       connect(m_pUploader,&Uploader::finished,m_pUploader,&QObject::deleteLater);
       m_pUploader->start();
}


// 弹出提示消息框
void Client::showError(QString msg)
{
    QMessageBox::information(this,"提示",msg);
}

// 服务器连接成功通知
void Client::showConnect()
{
    qDebug()<<"服务器链接成功";
}

// 连接断开：中止正在进行的上传，避免Uploader永久停等
void Client::onDisconnected()
{
    if(m_pUploader){
        m_pUploader->abort();
    }
}

// 接收消息槽函数：处理TCP粘包问题，逐个解析并处理完整PDU
void Client::recvMsg()
{
    qDebug()<<"recvMsg 读消息的长度"<<m_socket.bytesAvailable();
    // 处理粘包：TCP是流式协议，一条消息可能分多次到达
    QByteArray data = m_socket.readAll();                // 读取本次到达的数据
    buffer.append(data);                                  // 追加到缓冲区
    while(buffer.size() >= int(sizeof(PDU))){             // 至少有一个PDU头部长度
        PDU* pdu = (PDU*)buffer.data();                   // 读取PDU头
        if(buffer.size() < int(pdu->uiPDULen)){           // 数据不完整（半包），等待下次数据
            break;
        }
        handleMsg(pdu);                                   // 处理完整消息，路由到对应Handler
        buffer.remove(0,pdu->uiPDULen);                    // 从缓冲区移除已处理数据
    }

}


