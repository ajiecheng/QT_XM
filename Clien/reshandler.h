#ifndef RESHANDLER_H
#define RESHANDLER_H
// 响应处理器头文件：客户端侧处理服务器返回的各类响应，更新UI状态
#include "protocol.h"



class ResHandler
{
public:
    //  ResHandler();可以不要，系统自动调用默认构造

    // --- 用户相关响应 ---
    void regist(PDU* pdu);           // 注册响应：弹出成功/失败提示
    void login(PDU* pdu);            // 登录响应：成功则切换到主界面

    // --- 好友相关响应 ---
    void findUser(PDU* pdu);         // 查找用户响应：提示在线/离线/不存在
    void onlineUser(PDU* pdu);       // 在线用户列表响应：更新在线用户弹窗
    void addFriend(PDU* pdu);        // 添加好友响应：提示添加结果
    void addFriendResend(PDU* pdu);  // 收到好友申请：弹窗询问是否同意
    void addFriendAgree(PDU* pdu);   // 同意好友申请响应：刷新好友列表
    void flushFriend(PDU* pdu);      // 刷新好友列表响应：更新好友列表UI
    void delFriend(PDU* pdu);        // 删除好友响应：刷新好友列表

    // --- 聊天相关响应 ---
    void chat(PDU* pdu);             // 收到聊天消息：显示在聊天窗口

    // --- 文件操作响应 ---
    void mkdir(PDU* pdu);            // 新建文件夹响应：刷新文件列表
    void flushFile(PDU* pdu);        // 刷新文件列表响应：更新文件列表UI
    void deldir(PDU* pdu);           // 删除文件夹响应：刷新文件列表
    void uploadFileInit(PDU* pdu);   // 上传初始化响应：启动Uploader开始发送数据
    void uploadFileData(PDU* pdu);   // 上传完成响应：提示成功并刷新
    void shareFile(PDU* pdu);        // 分享文件响应：提示分享成功
    void shareFileResend(PDU* pdu);  // 收到分享文件通知：弹窗询问是否接收

    void uploadFileAck(PDU* pdu);

};

#endif // RESHANDLER_H
