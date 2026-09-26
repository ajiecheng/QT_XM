#include "mytcpsocket.h"
#include "protocol.h"
#include "operatedb.h"
#include "myqtcpserver.h"
// 自定义TCP Socket实现：消息收发、粘包处理、消息路由分发

// 构造函数：连接信号槽，创建消息处理器
MyTcpSocket::MyTcpSocket()
{
    // 当底层socket有数据可读时，触发recvMsg处理
    connect(this,&MyTcpSocket::readyRead,this,&MyTcpSocket::recvMsg);
    // 当客户端断开连接时，触发clientoffline清理
    connect(this,&MyTcpSocket::disconnected,this,&MyTcpSocket::clientoffline);
    m_pmh = new MsgHandler;  // 创建消息处理器（含上传进度状态等）
    m_pThread = nullptr;
}

// 接收消息槽函数（处理TCP粘包问题）：将数据拼接到缓冲区，逐个解析并处理完整PDU
void MyTcpSocket::recvMsg()
{
    qDebug()<<"recvMsg 读取消息的长度"<<this->bytesAvailable();
    // 处理粘包：TCP是流式协议，可能一次收到多个包或半个包
    QByteArray data = this->readAll();           // 读取本次到达的所有数据
    buffer.append(data);                          // 追加到缓冲区尾部
    while(buffer.size() >= int(sizeof(PDU))){     // 缓冲区足够大时才尝试解析（至少一个PDU头）
        PDU* pdu = (PDU*)buffer.data();           // 将缓冲区首地址转为PDU指针（读取头部字段）
        if(buffer.size() < int(pdu->uiPDULen)){   // 如果缓冲区数据不完整（半包），等待下次数据
            break;
        }
        PDU* respdu = handleMsg(pdu);             // 处理完整消息并获取响应
        sendMsg(respdu);                           // 将处理结果发送回客户端
        buffer.remove(0,pdu->uiPDULen);            // 从缓冲区移除已处理的数据
    }
}

// 根据消息类型路由到对应的业务处理函数（注册、登录、好友、文件、聊天等）
PDU *MyTcpSocket::handleMsg(PDU *pdu)
{
    qDebug()<< "handleMsg(读取消息) uiPDULen" << pdu->uiPDULen
            << "uiMsgLen" << pdu->uiMsgLen
            << "uiMsgType" << pdu->uiMsgType
            << "caData" << pdu->caData
            << "caData+32" << pdu->caData+32;
    // << "caMsg" <<QByteArray(pdu->caMsg, int(pdu->uiMsgLen));
    PDU* respdu = NULL;                          // 响应PDU，默认为空（不需响应时保持NULL）
    switch(pdu->uiMsgType){                      // 根据消息类型分派到MsgHandler的对应方法
    case ENUM_MSG_TYPE_REGIST_REQUEST:{          // 注册：验证用户名是否重复，创建用户目录
        respdu = m_pmh -> regist(pdu);
        break;
    }

    case ENUM_MSG_TYPE_LOGIN_REQUEST:{           // 登录：验证用户名密码，更新在线状态
        respdu = m_pmh -> login(pdu,m_strUserName);
        break;
    }
    case ENUM_MSG_TYPE_FIND_USER_REQUEST:{       // 查找用户：查询目标用户是否在线
        respdu = m_pmh -> findUser(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ONLINE_USER_REQUEST:{     // 获取在线用户：从数据库查询所有online=1的用户
        respdu = m_pmh -> onlineUser();
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:{      // 添加好友：检查目标用户状态，转发申请
        respdu = m_pmh -> addFriend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_ARGEE_REQUEST:{ // 同意好友申请：在数据库插入好友关系记录
        respdu = m_pmh -> addFriendArgee(pdu);
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:{    // 刷新好友：查询并返回当前用户的所有好友
        respdu = m_pmh -> flushFriend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_DEL_FRIEND_REQUEST:{      // 删除好友：从数据库中删除好友关系
        respdu = m_pmh -> delFriend(pdu);
        break;
    }
    case ENUM_MSG_TYPE_CHAT_REQUEST:{            // 聊天：将消息转发给目标用户
        respdu = m_pmh -> chat(pdu);
        break;
    }
    case ENUM_MSG_TYPE_MKDIR_REQUEST:{           // 新建文件夹：在用户目录下创建子目录
        respdu = m_pmh -> mkdir(pdu);
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:{      // 刷新文件列表：返回当前路径下所有文件和目录
        respdu = m_pmh -> flushFile(pdu);
        break;
    }
    case ENUM_MSG_TYPE_DEL_DIR_REQUEST:{         // 删除文件夹：递归删除指定目录
        respdu = m_pmh -> deldir(pdu);
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_INIT_REQUEST:{ // 上传文件初始化：打开目标文件准备写入
        respdu = m_pmh -> uploadFileInit(pdu);
        break;
    }
    case ENUM_MSG_TYPE_UPLOAD_FILE_DATA_REQUEST:{ // 上传文件数据块：接收文件内容写入磁盘
        respdu = m_pmh -> uploadFileData(pdu);
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:{       // 分享文件：将文件路径转发给选中的好友
        respdu = m_pmh -> shareFile(pdu);
        break;
    }
    case ENUM_MSG_TYPE_SHARE_FILE_ARGEE_REQUEST:{ // 同意接收分享文件：将文件复制到接收者目录
        respdu = m_pmh -> shareFileAgree(pdu);
        break;
    }


    default:
        break;
    }
    return respdu;
}

// 发送PDU消息：通过TCP写入数据后立即释放PDU内存
void MyTcpSocket::sendMsg(PDU*pdu)
{
    if(pdu==NULL){
        return;
    }

    this -> write((char*)pdu,pdu->uiPDULen);  // 将PDU原始字节数据写入TCP socket
    qDebug()<< "send(接收处理请求) uiPDULen" << pdu->uiPDULen
            << "uiMsgLen" << pdu->uiMsgLen
            << "uiMsgType" << pdu->uiMsgType
            << "caData" << pdu->caData
            << "caData+32" << pdu->caData+32;
    //<< "caMsg" <<QByteArray(pdu->caMsg, int(pdu->uiMsgLen));
    free(pdu);                                 // 发送后释放堆内存，防止内存泄漏
    pdu=NULL;                                  // 指针置空
}

// 跨线程安全写：由resend通过invokeMethod投递到本socket线程执行
void MyTcpSocket::forwardData(QByteArray data)
{
    qDebug() << "forwardData被调用, 长度" << data.size();
    this->write(data);   // 在socket所属线程写，安全
}

// 客户端离线处理：更新数据库在线状态为0，通知TCP服务器清理该socket
/*void MyTcpSocket::clientoffline()
{
    OperateDb::getInstance().handleOffline(m_strUserName.toStdString().c_str()); // 数据库online置0
    MyQTcpServer::getInstance().deleteSocket(this);                              // 从在线列表中移除
}

void MyTcpSocket::clientoffline()
{
    OperateDb::getInstance().handleOffline(m_strUserName.toStdString().c_str()); // B-3修
    MyQTcpServer::getInstance().deleteSocket(this);   // 从列表移除（B-2修线程安全）
    this->deleteLater();                              // 在本线程排队删除socket
    if(m_pThread){
        m_pThread->quit();                            // 退出线程事件循环
    }
}
 */

void MyTcpSocket::clientoffline()
{
    OperateDb::getInstance().handleOffline(m_strUserName.toStdString().c_str()); // 最后一次用本线程连接
    MyQTcpServer::getInstance().deleteSocket(this);   // 从列表移除
    m_pmh->abortUpload();               // 清理半成品上传文件

    //OperateDb::getInstance().releaseDb();             // 归还本线程的数据库连接（标记空闲）
    this->deleteLater();                              // 本线程排队删除socket

    // ⚠️ 删掉原来的：
    // if(m_pThread){
    //     m_pThread->quit();   // 线程是共享的，退出它会连累同线程的其他客户端
    // }
}

// 从socket缓冲区读取一条完整PDU消息：先读长度头，再按总长度读剩余数据
PDU* MyTcpSocket::readMsg()
{
    qDebug()<<"readMsg 读取消息的长度"<<this->bytesAvailable();

    unsigned int uiPDULen = 0;
    this->read((char* ) &uiPDULen,sizeof(unsigned int));  // 先读取4字节的PDU总长度字段

    PDU* pdu = mkPDU(uiPDULen-sizeof(PDU));               // 根据消息体长度申请PDU空间
    this->read((char*)pdu+sizeof(unsigned int),uiPDULen-sizeof(unsigned int)); // 读取剩余数据（含MsgLen、MsgType、caData、caMsg）
    qDebug()<< "readMsg(读取消息) uiPDULen" << pdu->uiPDULen
            << "uiMsgLen" << pdu->uiMsgLen
            << "uiMsgType" << pdu->uiMsgType
            << "caData" << pdu->caData
            << "caData+32" << pdu->caData+32;
            //<< "caMsg" <<QByteArray(pdu->caMsg, int(pdu->uiMsgLen));
    return pdu;                                            // 返回完整的PDU
}


// 析构函数：释放消息处理器
MyTcpSocket::~MyTcpSocket()
{
    delete m_pmh;
    m_pmh = NULL;
}

