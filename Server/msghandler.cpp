#include "msghandler.h"
#include "myqtcpserver.h"
#include "operatedb.h"
#include "server.h"
#include <QDebug>
#include <QDir>
#include <QCryptographicHash>//MD5
#include <QFileInfo>//秒传

// 消息处理器实现：服务器端处理所有业务请求的核心逻辑

MsgHandler::MsgHandler()
{

}

// 注册处理：从PDU提取用户名密码，写入数据库，并为新用户创建个人目录
PDU* MsgHandler::regist(PDU *pdu)
{
    char caName[32] = {'\0'};                     // 用户名字段（定长32字节）
    char caPwd[32] = {'\0'};                      // 密码字段（定长32字节）
    memcpy(caName,pdu->caData,32);                 // caData前32字节为用户名的ASCII
    memcpy(caPwd,pdu->caData+32,32);              // caData后32字节为密码的ASCII
    bool ret = OperateDb::getInstance().handleRegist(caName,caPwd); // 数据库验证并插入记录
    qDebug()<<"注册返回 ret"<<ret;
    if(ret){
        QDir dir;
        dir.mkdir(QString("%1/%2").arg(Server::getInstance().m_strRootPath).arg(caName)); // 在根路径下创建用户专属目录
    }
    PDU*respdu = mkPDU();                         // 创建响应PDU（无消息体）
    memcpy(respdu->caData,&ret,sizeof(bool));     // 将注册结果（bool）放入caData返回
    respdu->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;
    return respdu;
}

// 登录处理：验证用户名密码，成功后将loginName引用赋值为当前用户名
PDU *MsgHandler::login(PDU *pdu,QString &loginName)
{
    char caName[32] = {'\0'};
    char caPwd[32] = {'\0'};
    memcpy(caName,pdu->caData,32);
    memcpy(caPwd,pdu->caData+32,32);
    bool ret = OperateDb::getInstance().handleLogin(caName,caPwd); // 数据库验证账号密码
    qDebug()<<"登录返回"<<ret;
    if(ret){
        loginName = caName;                        // 通过引用参数将用户名传回MyTcpSocket::m_strUserName
    }
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(bool));
    respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
    return respdu;
}

// 查找用户：查询目标用户的在线状态
PDU *MsgHandler::findUser(PDU *pdu)
{
    char caName[32] = {'\0'};
    memcpy(caName,pdu->caData,32);
    int ret = OperateDb::getInstance().handleFindUser(caName); // 返回值：-1失败，0离线，1在线，2不存在
    qDebug()<<"查找返回"<<ret;
    PDU* respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(int));
    respdu->uiMsgType = ENUM_MSG_TYPE_FIND_USER_RESPOND;
    return respdu;
}

// 获取在线用户列表：查询数据库中所有online=1的用户
PDU *MsgHandler::onlineUser()
{
    QStringList ret = OperateDb::getInstance().handleOnlineUser();    // 从数据库获取在线用户名列表
    qDebug()<<"在线用户"<<ret;
    PDU* respdu = mkPDU(ret.size()*32);                               // 每个用户名占32字节
    respdu->uiMsgType = ENUM_MSG_TYPE_ONLINE_USER_RESPOND;
    for(int i = 0 ; i<ret.size();i++){
        //memcpy(respdu->caMsg+32*i,ret.at(i).toStdString().c_str(),32); // 逐条拷贝用户名到caMsg
        strncpySafe(respdu->caMsg+i*32, 32,ret.at(i).toStdString().c_str());

        qDebug()<<"第"<<i+1<<"个在线用户"<<respdu->caMsg+32*i;
    }
    return respdu;
}

// 添加好友：检查目标用户状态，若在线则转发好友申请
PDU *MsgHandler::addFriend(PDU *pdu)
{
    char caCurName[32] = {'\0'};
    char caTarName[32] = {'\0'};                     // 目标用户名
    memcpy(caCurName,pdu->caData,32);
    memcpy(caTarName,pdu->caData+32,32);
    int ret = OperateDb::getInstance().handleAddFirend(caCurName,caTarName); // 检查是否为好友及目标在线状态
    qDebug()<<"添加好友 ret"<<ret;
    if(ret == 1){                                    // ret==1表示目标在线且非好友
        MyQTcpServer::getInstance().resend(caTarName,pdu); // 将好友申请转发给目标客户端（触发对面的addFriendResend）
    }
    PDU*respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(int));
    respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
    return respdu;
}

// 同意好友申请：在数据库插入双向好友关系记录
PDU *MsgHandler::addFriendArgee(PDU *pdu)
{
    char caCurName[32] = {'\0'};
    char caTarName[32] = {'\0'};                     // 目标用户（发起申请的用户）
    memcpy(caCurName,pdu->caData,32);
    memcpy(caTarName,pdu->caData+32,32);
    bool ret = OperateDb::getInstance().handleAddFirendArgee(caCurName,caTarName); // 插入friend表记录
    qDebug()<<"同意添加好友 ret"<<ret;
    PDU*respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(bool));
    respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_ARGEE_RESPOND;
    return respdu;
}

// 刷新好友列表：查询当前用户的所有好友，按32字节一组打包返回
PDU *MsgHandler::flushFriend(PDU *pdu)
{
    char caCurName[32] = {'\0'};
    memcpy(caCurName,pdu->caData,32);
    QStringList res = OperateDb::getInstance().handleFlushFriend(caCurName); // 查询好友列表
    PDU*respdu = mkPDU(res.size()*32);               // 每个好友名占32字节
    respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
    for(int i = 0;i<res.size();i++)
    {
        //memcpy(respdu->caMsg+i*32,res.at(i).toStdString().c_str(),32); // 逐个好友名拷贝到caMsg
        strncpySafe(respdu->caMsg+i*32, 32,res.at(i).toStdString().c_str());
    }
    return respdu;
}

// 删除好友：从数据库中删除好友关系
PDU *MsgHandler::delFriend(PDU *pdu)
{
    char caCurName[32] = {'\0'};
    char caTarName[32] = {'\0'};                     // 目标用户（要删除的好友）
    memcpy(caCurName,pdu->caData,32);
    memcpy(caTarName,pdu->caData+32,32);
    bool ret = OperateDb::getInstance().handleDelFirend(caCurName,caTarName); // 数据库删除
    qDebug()<<"删除好友 ret"<<ret;
    PDU*respdu = mkPDU();
    memcpy(respdu->caData,&ret,sizeof(bool));
    respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FRIEND_RESPOND;
    return respdu;
}

// 聊天消息处理：提取目标用户名，将消息转发给对方
PDU *MsgHandler::chat(PDU *pdu)
{
    char caTarName[32] = {'\0'};                     // 聊天消息的目标用户
    memcpy(caTarName,pdu->caData+32,32);
    MyQTcpServer::getInstance().resend(caTarName,pdu); // 将聊天PDU直接转发
    return NULL;                                      // 聊天消息不需要回复响应
}

// 新建文件夹：在指定路径下创建子目录
PDU *MsgHandler::mkdir(PDU *pdu)
{
    QString strNewPath = QString("%1/%2").arg(pdu->caMsg).arg(pdu->caData); // 路径=当前路径+"/"+新文件夹名
    QDir dir;
    bool ret = dir.mkdir(strNewPath);                // 调用Qt API创建目录
    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_MKDIR_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}

// 刷新文件列表：获取指定路径下所有文件和目录的信息
PDU *MsgHandler::flushFile(PDU *pdu)
{
    QDir dir(pdu->caMsg);                             // 用caMsg中的路径构建QDir对象
    QFileInfoList fileInfoList =  dir.entryInfoList(); // 获取目录下所有条目（含.和..）
    int iFileCount =  fileInfoList.size();            // 条目总数
    PDU* respdu = mkPDU(iFileCount * sizeof (FileInfo)); // 按文件数*每个FileInfo大小申请caMsg空间
    respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
    // 遍历每个文件/目录，构建FileInfo结构体放入caMsg
    QString strFileName;
    for(int i = 0 ;i < iFileCount; i++ ){
        strFileName = fileInfoList[i].fileName();
        FileInfo* pFileInfo = (FileInfo*)respdu->caMsg+i; // 逐条偏移，第i个FileInfo的位置
        //memcpy(pFileInfo->caName,strFileName.toStdString().c_str(),32);
        strncpySafe(pFileInfo->caName, 32,strFileName.toStdString().c_str());
        if(fileInfoList[i].isDir()){
            pFileInfo->iFileType = 0;                 // 0表示目录
        }else{
            pFileInfo->iFileType = 1;                 // 1表示普通文件
        }
        qDebug()<<"strFileName 刷新文件"<<strFileName;
    }
    return respdu;
}

// 删除文件夹：递归删除指定目录及其下所有内容
PDU *MsgHandler::deldir(PDU *pdu)
{
    QString strNewPath = QString("%1/%2").arg(pdu->caMsg).arg(pdu->caData); // 构建完整删除路径
    QDir dir(strNewPath);
    bool ret = dir.removeRecursively();              // 递归删除（删除目录及其下所有文件和子目录）
    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}

// 上传文件初始化：创建目标文件并打开，准备接收数据块
/* PDU *MsgHandler::uploadFileInit(PDU *pdu)
{
    char caFileName[32] = {'\0'};
    memcpy(caFileName,pdu->caData,32);               // 文件名从caData前32字节提取
    memcpy(&m_iUploadFileSize, pdu->caData+32,sizeof (qint64)); // 文件总大小从caData后32字节提取
    memcpy(m_caUploadFileMd5, pdu->caData+40, 16);             // 读取客户端传来的MD5<追加>
    QString strPath = QString("%1/%2").arg(pdu->caMsg).arg(caFileName); // 拼接：当前路径/文件名
    m_fUploadFile.setFileName(strPath);              // 设置QFile的文件路径
    m_iUploadFileReceived = 0;                       // 重置已接收字节计数
    bool ret = m_fUploadFile.open(QIODevice::WriteOnly); // 以只写方式创建/覆盖文件

    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_INIT_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}
*/
// 上传文件初始化：先判断秒传，否则创建/覆盖目标文件，回传已收偏移offset
PDU *MsgHandler::uploadFileInit(PDU *pdu)
{

/* char caFileName[32] = {'\0'};
    memcpy(caFileName,pdu->caData,32);
    memcpy(&m_iUploadFileSize, pdu->caData+32,sizeof (qint64));
    memcpy(m_caUploadFileMd5, pdu->caData+40, 16);    // 读取客户端传来的MD5<追加>
    QString strPath = QString("%1/%2").arg(pdu->caMsg).arg(caFileName);
    m_fUploadFile.setFileName(strPath);
    m_iUploadFileReceived = 0;
*/
    memcpy(&m_iUploadFileSize, pdu->caData, sizeof(qint64));
    memcpy(m_caUploadFileMd5, pdu->caData+8, 16);

    // caMsg = 文件名 + '\0' + 路径
    QString strFileName = QString::fromUtf8(pdu->caMsg);       // 读到第一个\0为止，文件名可任意长
    int nameLen = strFileName.toUtf8().size() + 1;             // 文件名字节数 + 1个\0
    QString strPath = QString::fromUtf8(pdu->caMsg + nameLen); // \0之后是路径

    QString strFullPath = QString("%1/%2").arg(strPath).arg(strFileName);
    m_fUploadFile.setFileName(strFullPath);
    m_iUploadFileReceived = 0;

    qint64 offset = 0;                                  // 已收字节数
    QFileInfo info(strFullPath);
    if(info.exists()){
        qint64 existingSize = info.size();
        if(existingSize == m_iUploadFileSize){
            // 大小相同：算MD5，一致则秒传，不一致则覆盖重传
            QFile f(strFullPath);
            if(f.open(QIODevice::ReadOnly)){
                QCryptographicHash hash(QCryptographicHash::Md5);
                hash.addData(&f);
                QByteArray baMd5 = hash.result();
                f.close();
                if(baMd5 == QByteArray(m_caUploadFileMd5,16)){
                    offset = m_iUploadFileSize;          // 秒传
                }else{
                    offset = 0;                          // 同名不同内容：覆盖
                }
            }
        }else if(existingSize < m_iUploadFileSize){
            offset = existingSize;                       // 半成品：断点续传
        }else{
            offset = 0;                                  // 已存在文件更大：异常，覆盖
        }
    }

    m_iUploadFileReceived = offset;                      // 关键：已收字节从offset开始累加

    if(offset == 0){
        if(!m_fUploadFile.open(QIODevice::WriteOnly)){   // 覆盖写
            offset = -1;
        }
    }else if(offset < m_iUploadFileSize){
        if(!m_fUploadFile.open(QIODevice::Append)){      // 续传：追加写
            offset = -1;
        }
    }
    // offset == m_iUploadFileSize 时不打开（秒传）

    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_INIT_RESPOND;
    memcpy(respdu->caData,&offset,sizeof(qint64));
    return respdu;
/*
    qint64 offset = 0;                                // 回传给客户端：服务器已收的字节数

    // 秒传判断：目标文件已存在且MD5一致 → 无需重复上传
    QFileInfo info(strPath);
    if(info.exists()){
        QFile f(strPath);
        if(f.open(QIODevice::ReadOnly)){
            QCryptographicHash hash(QCryptographicHash::Md5);
            hash.addData(&f);
            QByteArray baMd5 = hash.result();
            f.close();
            if(baMd5 == QByteArray(m_caUploadFileMd5,16)){
                offset = m_iUploadFileSize;           // 秒传：已收==总大小，客户端跳过上传
            }
        }
    }

    // 非秒传时才打开文件准备接收（覆盖写）
    if(offset != m_iUploadFileSize){
        if(!m_fUploadFile.open(QIODevice::WriteOnly)){
            offset = -1;                              // 打开失败：回传-1表示错误
        }
    }

    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_INIT_RESPOND;
    memcpy(respdu->caData,&offset,sizeof(qint64));    // 回传已收偏移
    return respdu;

*/

}


/* // 上传文件数据块：接收数据写入文件，累计接收量，完成后关闭文件
PDU *MsgHandler::uploadFileData(PDU *pdu)
{
    m_fUploadFile.write(pdu->caMsg,pdu->uiMsgLen);   // 将caMsg中的数据块写入文件
    m_iUploadFileReceived += pdu->uiMsgLen;           // 累加已接收字节数
    if(m_iUploadFileReceived < m_iUploadFileSize){    // 未接收完：返回NULL，不回复客户端
        return NULL;
    }
    m_fUploadFile.close();                            // 接收完毕：关闭文件
    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_RESPOND;
    return respdu;                                    // 回复上传完成确认
}




// 上传文件数据块：接收数据写入文件，收满后校验MD5，回传校验结果
PDU *MsgHandler::uploadFileData(PDU *pdu)
{
    m_fUploadFile.write(pdu->caMsg,pdu->uiMsgLen);   // 将caMsg中的数据块写入文件
    m_iUploadFileReceived += pdu->uiMsgLen;           // 累加已接收字节数
    if(m_iUploadFileReceived < m_iUploadFileSize){    // 未接收完：返回NULL，不回复客户端
        return NULL;
    }
    m_fUploadFile.close();                            // 接收完毕：关闭文件

    // 计算落盘文件的MD5，与客户端传来的期望值比对
    bool bMd5Ok = false;
    QFile f(m_fUploadFile.fileName());                // fileName()取回setFileName时的路径
    if(f.open(QIODevice::ReadOnly)){
        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(&f);
        QByteArray baMd5 = hash.result();             // 16字节二进制MD5
        bMd5Ok = (baMd5 == QByteArray(m_caUploadFileMd5, 16)); // 与期望MD5逐字节比较
        f.close();
    }

    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_RESPOND;
    memcpy(respdu->caData,&bMd5Ok,sizeof(bool));      // 把MD5校验结果带回客户端
    return respdu;
}

PDU *MsgHandler::uploadFileData(PDU *pdu)
{
    m_fUploadFile.write(pdu->caMsg,pdu->uiMsgLen);
    m_iUploadFileReceived += pdu->uiMsgLen;

    if(m_iUploadFileReceived < m_iUploadFileSize){
        // 未收完：回ACK（已收字节数），供客户端停等流控
        PDU* ackpdu = mkPDU();
        ackpdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_ACK;
        memcpy(ackpdu->caData, &m_iUploadFileReceived, sizeof(qint64));
        return ackpdu;
    }

    // 收满：close + MD5校验 + 回最终完成（保持你之前的MD5逻辑不变）
    m_fUploadFile.close();
    bool bMd5Ok = false;
    QFile f(m_fUploadFile.fileName());
    if(f.open(QIODevice::ReadOnly)){
        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(&f);
        QByteArray baMd5 = hash.result();
        bMd5Ok = (baMd5 == QByteArray(m_caUploadFileMd5, 16));
        f.close();
    }
    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_RESPOND;
    memcpy(respdu->caData,&bMd5Ok,sizeof(bool));
    return respdu;
}

*/
PDU *MsgHandler::uploadFileData(PDU *pdu)
{
    // 写入数据块，检查是否写成功（磁盘满/权限不足时返回<uiMsgLen）
    qint64 written = m_fUploadFile.write(pdu->caMsg, pdu->uiMsgLen);
    //written = -1;//上传试验，返回上传失败，中止上传
    if(written != pdu->uiMsgLen){
        m_fUploadFile.close();
        PDU* respdu = mkPDU();
        respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_RESPOND;
        int result = 1;                               // 1=写文件失败
        memcpy(respdu->caData, &result, sizeof(int));
        return respdu;
    }
    m_iUploadFileReceived += pdu->uiMsgLen;

    if(m_iUploadFileReceived < m_iUploadFileSize){
        PDU* ackpdu = mkPDU();
        ackpdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_ACK;
        memcpy(ackpdu->caData, &m_iUploadFileReceived, sizeof(qint64));
        return ackpdu;
    }

    m_fUploadFile.close();
    bool bMd5Ok = false;
    QFile f(m_fUploadFile.fileName());
    if(f.open(QIODevice::ReadOnly)){
        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(&f);
        QByteArray baMd5 = hash.result();
        bMd5Ok = (baMd5 == QByteArray(m_caUploadFileMd5, 16));
        f.close();
    }
    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_RESPOND;
    int result = bMd5Ok ? 0 : 2;                      // 0=成功, 2=MD5失败
    memcpy(respdu->caData, &result, sizeof(int));
    return respdu;
}


// 连接中断时清理：关闭正在接收的半成品上传文件，重置接收状态
void MsgHandler::abortUpload()
{
    if(m_fUploadFile.isOpen()){
        m_fUploadFile.close();
    }
    m_iUploadFileReceived = 0;
}

// 分享文件：把文件路径转发给被选中的好友们
PDU *MsgHandler::shareFile(PDU *pdu)
{
    char strCurName[32] = {'\0'};
    memcpy(strCurName,pdu->caData,32);               // 提取分享者用户名（放在caData前32字节）
    int iFriendNum = 0;
    memcpy(&iFriendNum,pdu->caData+32,sizeof(int));  // 提取要分享的好友数量
    PDU* resendpdu = mkPDU(pdu->uiMsgLen-iFriendNum*32); // 构建转发PDU（去掉好友名列表部分）
    resendpdu->uiMsgType = pdu->uiMsgType;
    memcpy(resendpdu->caData,strCurName,32);
    memcpy(resendpdu->caMsg,pdu->caMsg+iFriendNum*32,pdu->uiMsgLen-iFriendNum*32); // 只拷贝文件路径部分

    char caRecvName[32] = {'\0'};
    for(int i = 0;i < iFriendNum; i++){
        memcpy(caRecvName, pdu->caMsg+i*32,32);      // 依次取出每个被分享好友的用户名
        MyQTcpServer::getInstance().resend(caRecvName, resendpdu); // 逐个转发文件分享通知
    }
    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_ARGEE_RESPOND;
    return respdu;
}

// 同意接收分享文件：将分享的文件复制到接收者的个人目录下
PDU *MsgHandler::shareFileAgree(PDU *pdu)
{
    QString strShareFriendPath = pdu->caMsg;        // pdu->caMsg为分享者的源文件完整路径
    int index = strShareFriendPath.lastIndexOf('/'); // 找到最后一个'/'的位置以提取文件名
    QString strFileName = strShareFriendPath.right(strShareFriendPath.size() - index -1); // 提取文件名

    QString strNewPath = QString("%1/%2/%3").arg(Server::getInstance().m_strRootPath)
            .arg(pdu->caData).arg(strFileName);      // 目标路径 = 根路径/接收者名/文件名
    bool ret = QFile::copy(strShareFriendPath, strNewPath); // Qt文件复制
    PDU* respdu = mkPDU();
    respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_ARGEE_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}
