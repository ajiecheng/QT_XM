#ifndef MSGHANDLER_H
#define MSGHANDLER_H
// 消息处理器：服务器端核心业务逻辑层，处理所有客户端请求
#include "protocol.h"

#include <QFile>
#include <QString>



class MsgHandler
{
public:
    MsgHandler();
    qint64 m_iUploadFileSize;        // 当前上传文件的总大小（字节数）
    qint64 m_iUploadFileReceived;    // 当前上传文件已接收的字节数（用于判断是否传输完成）
    QFile m_fUploadFile;             // 当前正在接收的上传文件对象
    char m_caUploadFileMd5[16];          // 当前上传文件的期望MD5（来自INIT请求，16字节二进制）
    void abortUpload();//中止上传文件



    // --- 用户相关 ---
    PDU* regist(PDU* pdu);           // 用户注册：检查重复、创建用户目录
    PDU* login(PDU* pdu,QString& loginName); // 用户登录：验证凭据、更新在线状态
    PDU* findUser(PDU* pdu);         // 查找用户：查询目标用户是否在线

    // --- 好友相关 ---
    PDU* onlineUser();               // 获取在线用户：查询所有online=1的用户
    PDU* addFriend(PDU* pdu);        // 添加好友：检查目标并转发申请
    PDU* addFriendArgee(PDU* pdu);   // 同意好友申请：插入数据库记录
    PDU* flushFriend(PDU* pdu);      // 刷新好友列表：查询并返回所有好友
    PDU* delFriend(PDU* pdu);        // 删除好友：从数据库删除关系

    // --- 聊天相关 ---
    PDU* chat(PDU* pdu);             // 聊天消息：转发给目标用户

    // --- 文件操作 ---
    PDU* mkdir(PDU* pdu);            // 新建文件夹：在指定路径创建目录
    PDU* flushFile(PDU* pdu);        // 刷新文件列表：获取当前路径下的文件和目录
    PDU* deldir(PDU* pdu);           // 删除文件夹：递归删除
    PDU* uploadFileInit(PDU* pdu);   // 上传文件初始化：创建文件并打开写入
    PDU* uploadFileData(PDU* pdu);   // 上传文件数据块：写入数据，完成后关闭文件
    PDU* shareFile(PDU* pdu);        // 分享文件：将文件路径转发给选中的好友
    PDU* shareFileAgree(PDU* pdu);   // 同意接收分享：复制文件到接收者目录

};

#endif // MSGHANDLER_H
