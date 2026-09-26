#include "client.h"
#include "index.h"
#include "reshandler.h"
#include <QMessageBox>
#include <string>
#include <QFile>
#include "uploader.h"


// 响应处理器实现：解析服务器返回的PDU数据，执行对应的UI更新操作

//ResHandler::ResHandler()
//{
//}

// 注册响应：从caData中提取bool结果，弹窗提示
void ResHandler::regist(PDU *pdu)
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof (bool));      // caData第1字节为bool结果
    if(ret){
        QMessageBox::information(&Client::getInstance(),"注册","注册成功");
    }else{
        QMessageBox::information(&Client::getInstance(),"注册","注册失败");
    }
}

// 登录响应：成功则显示主界面并隐藏登录窗口，失败则弹窗提示
void ResHandler::login(PDU *pdu)
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().show();             // 显示主界面（好友+文件页面）
        Client::getInstance().hide();            // 隐藏登录/注册窗口
    }else{
        QMessageBox::information(&Client::getInstance(),"登录","登录失败");
    }
}

// 查找用户响应：根据返回值弹窗提示用户状态
// 返回值含义：-1=失败，0=离线，1=在线，2=不存在
void ResHandler::findUser(PDU *pdu)
{
    int ret;
    memcpy(&ret,pdu->caData,sizeof(int));
    if(ret==-1){
        QMessageBox::information(Index::getInstance().getFriend(),"查找用户","查找用户失败");
    }else if(ret == 0){
        QMessageBox::information(Index::getInstance().getFriend(),"查找用户","该用户不在线");
    }else if(ret == 1){
        QMessageBox::information(Index::getInstance().getFriend(),"查找用户","该用户在线");
    }else if(ret == 2){
        QMessageBox::information(Index::getInstance().getFriend(),"查找用户","该用户不存在");
    }
}

// 在线用户列表响应：从caMsg中解析出所有在线用户名（每32字节一个）
void ResHandler::onlineUser(PDU *pdu)
{
    QStringList userList;
     char caTmp[32] = {'\0'};                    // 临时存储每个用户名
    for(int i = 0;i < int(pdu->uiMsgLen/32);i++){ // uiMsgLen/32 = 在线用户数
        memcpy(caTmp, pdu->caMsg+i*32, 32);       // 逐个提取用户名
        userList.append(caTmp);
    }
    Index::getInstance().getFriend()->m_pOnlineUser->updateOnlineUser(userList); // 更新在线用户弹窗
}

// 添加好友响应：根据返回值（-1失败/0离线/2已是好友）弹窗提示
void ResHandler::addFriend(PDU *pdu)
{
    int ret;
    memcpy(&ret,pdu->caData,sizeof(int));
    if(ret==-1){
        QMessageBox::information(Index::getInstance().getFriend(),"添加好友","添加好友失败");
    }else if(ret == 0){
        QMessageBox::information(Index::getInstance().getFriend(),"添加好友","该用户不在线");
    }else if(ret == 2){
        QMessageBox::information(Index::getInstance().getFriend(),"添加好友","已经是好友关系");
    }
}

// 收到好友申请（服务器转发）：弹窗询问用户是否同意
void ResHandler::addFriendResend(PDU *pdu)
{
    char caCurName[32] = {'\0'};
    memcpy(caCurName,pdu->caData,32);             // 提取发起申请的用户名
    int ret = QMessageBox::question(Index::getInstance().getFriend(),"添加好友",
                                     QString("是否同意 %1 的好友申请").arg(caCurName)); // 弹出询问框
    if(ret != QMessageBox::Yes){                  // 不同意则直接返回
        return;
    }
    PDU*respdu = mkPDU();                         // 同意：构建同意请求PDU
    memcpy(respdu->caData,pdu->caData,64);        // caData原样复制（两个用户名）
    respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_ARGEE_REQUEST;
    Client::getInstance().sendMsg(respdu);         // 发送同意好友申请
}

// 同意好友申请响应：成功则刷新好友列表
void ResHandler::addFriendAgree(PDU *pdu)
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().getFriend()->flushFriend(); // 添加成功后刷新好友列表
    }else{
        QMessageBox::information(Index::getInstance().getFriend(),"添加好友","添加好友失败");
    }
}

// 刷新好友列表响应：从caMsg解析好友名（每32字节一个），更新好友页面
void ResHandler::flushFriend(PDU *pdu)
{
    QStringList friendList;
    char caTmp[32] = {'\0'};
    for(int i = 0;i < int(pdu->uiMsgLen/32);i++){   // uiMsgLen/32 = 好友数量
        memcpy(caTmp, pdu->caMsg+i*32, 32);
        friendList.append(caTmp);                    // 收集好友名
    }
    Index::getInstance().getFriend()->updateFriend(friendList); // 更新好友列表UI
}

// 删除好友响应：成功则刷新好友列表
void ResHandler::delFriend(PDU *pdu)
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().getFriend()->flushFriend(); // 删除成功后刷新
    }else{
        QMessageBox::information(Index::getInstance().getFriend(),"删除好友","删除好友失败");
    }
}

// 收到聊天消息：提取发送者名和消息内容，显示在聊天窗口
void ResHandler::chat(PDU *pdu)
{
    Chat* c = Index::getInstance().getFriend()->m_pChat; // 获取聊天窗口指针
    if(c->isHidden()){
        c->show();                                       // 如果窗口隐藏，先显示
    }
    char caChatName[32] = {'\0'};
    memcpy(caChatName,pdu->caData,32);                   // 提取发送者用户名
    c->m_strChatName = caChatName;                       // 设置聊天对象名
    c->updageShow_TE(QString("%1: %2").arg(caChatName).arg(pdu->caMsg)); // 格式化显示消息
}

// 新建文件夹响应：成功则刷新文件列表
void ResHandler::mkdir(PDU *pdu)
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().getFile()->flushFile();     // 创建成功后刷新
    }else{
        QMessageBox::information(Index::getInstance().getFile(),"创建文件夹","创建文件夹失败");
    }
}

// 刷新文件列表响应：从caMsg解析FileInfo数组，更新文件页面列表
void ResHandler::flushFile(PDU *pdu)
{
    QList<FileInfo*> pFileList;
    int iFileCount = pdu->uiMsgLen / sizeof(FileInfo);   // 文件/目录数量
    for(int i = 0; i < iFileCount; i++){
        FileInfo* pFileInfo = new FileInfo;               // 在堆上创建FileInfo
        memcpy(pFileInfo,pdu->caMsg+i*sizeof(FileInfo),sizeof(FileInfo)); // 按偏移拷贝每个FileInfo
        pFileList.append(pFileInfo);                      // 加入列表
    }
    Index::getInstance().getFile()->updateFileList(pFileList); // 更新文件页面UI
}

// 删除文件夹响应：成功则刷新文件列表
void ResHandler::deldir(PDU *pdu)
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Index::getInstance().getFile()->flushFile();     // 删除成功后刷新
    }else{
        QMessageBox::information(Index::getInstance().getFile(),"删除文件夹","删除文件夹失败");
    }
}

// 上传文件初始化响应：服务器准备好接收后，启动客户端上传流程
/*void ResHandler::uploadFileInit(PDU *pdu)
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    if(ret){
        Client::getInstance().startUpload();             // 服务器就绪，开始上传文件数据
    }else{
        QMessageBox::information(Index::getInstance().getFile(),"上传文件","上传文件失败");
    }
}
  */
// 上传文件初始化响应：根据服务器回传的offset判断 失败/秒传/正常上传
void ResHandler::uploadFileInit(PDU *pdu)
{
    qint64 offset;
    memcpy(&offset,pdu->caData,sizeof(qint64));

    if(offset == -1){                                 // 服务器打开文件失败
        QMessageBox::information(Index::getInstance().getFile(),"上传文件","上传文件失败");
        return;
    }

    qint64 iFileSize = QFile(Index::getInstance().getFile()->m_strUploadFilePath).size(); // 本地文件大小
    if(offset == iFileSize){                          // 秒传：服务器已有完整且一致的该文件
        QMessageBox::information(Index::getInstance().getFile(),"上传文件","秒传成功：服务器已存在相同文件");
        Index::getInstance().getFile()->flushFile();  // 刷新文件列表
        return;
    }
    // offset==0（正常）或 0<offset<大小（续传）：都走startUpload，Uploader内部seek
    Client::getInstance().m_iUploadOffset = offset;
    Client::getInstance().startUpload();

    //Client::getInstance().startUpload();              // offset==0：正常从0开始上传
}

// 上传块确认：唤醒正在停等的Uploader，让它发下一块
void ResHandler::uploadFileAck(PDU *pdu)
{
    qint64 received;
    memcpy(&received, pdu->caData, sizeof(qint64));   // 已收字节数（可后续用于服务器真实进度）
    if(Client::getInstance().m_pUploader){
        Client::getInstance().m_pUploader->onAck();
    }
}

// 上传文件完成响应：提示成功并刷新文件列表
/*void ResHandler::uploadFileData(PDU *pdu)
{
    QMessageBox::information(Index::getInstance().getFile(),"上传文件","上传文件成功");
    Index::getInstance().getFile()->flushFile();         // 上传完成后刷新文件列表
}

// 上传文件完成响应：根据MD5校验结果提示成功或失败
void ResHandler::uploadFileData(PDU *pdu)
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    Index::getInstance().getFile()->resetUploadProgress();  // 无论成败，先复位进度条
    if(ret){
        QMessageBox::information(Index::getInstance().getFile(),"上传文件","上传文件成功");
        Index::getInstance().getFile()->flushFile();     // 成功后刷新文件列表
    }else{
        QMessageBox::information(Index::getInstance().getFile(),"上传文件","上传文件失败：MD5校验不通过");
    }
}

   */

void ResHandler::uploadFileData(PDU *pdu)
{
    int result;
    memcpy(&result, pdu->caData, sizeof(int));

    // 写文件失败发生在中间块时，Uploader 还在停等ACK，必须先中止它
    if(result == 1){
        if(Client::getInstance().m_pUploader){
            Client::getInstance().m_pUploader->abort();
        }
    }
    Index::getInstance().getFile()->resetUploadProgress();

    if(result == 0){
        QMessageBox::information(Index::getInstance().getFile(),"上传文件","上传文件成功");
        Index::getInstance().getFile()->flushFile();
    }else if(result == 1){
        QMessageBox::information(Index::getInstance().getFile(),"上传文件","上传文件失败：服务器写文件失败");
    }else{
        QMessageBox::information(Index::getInstance().getFile(),"上传文件","上传文件失败：MD5校验不通过");
    }
}



// 分享文件响应（发送端）：提示文件已分享
void ResHandler::shareFile(PDU *pdu)
{
    QMessageBox::information(Index::getInstance().getFile(),"分享文件","文件已分享");
}

// 收到分享文件通知（接收端）：弹窗询问是否接收，同意则发送同意请求
void ResHandler::shareFileResend(PDU *pdu)
{
    QString strSharePath = QString(pdu->caMsg);           // caMsg为分享文件的完整路径
    int index = strSharePath.lastIndexOf('/');            // 从路径中提取文件名
    QString strFileName = strSharePath.right(strSharePath.size()-index-1);
    int ret = QMessageBox::question(Index::getInstance().getFile(),"分享文件",
                                     QString("%1 分享文件: %2\n是否接收").arg(pdu->caData).arg(strFileName));
    if(ret != QMessageBox::Yes){                          // 不同意接收则返回
        return;
    }
    PDU* respdu = mkPDU(pdu->uiMsgLen);                   // 同意：构建同意接收请求
    respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_ARGEE_REQUEST;
    //memcpy(respdu->caData,Client::getInstance().m_strUserName.toStdString().c_str(),32); // 接收者用户名
    strncpySafe(pdu->caData, 32,Client::getInstance().m_strUserName.toStdString().c_str());
    memcpy(respdu->caMsg,pdu->caMsg,pdu->uiMsgLen);       // 文件路径原样传回
    Client::getInstance().sendMsg(respdu);                 // 发送同意接收请求
}
