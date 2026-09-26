#ifndef PROTOCOL_H
#define PROTOCOL_H
// 消息协议头文件：定义客户端与服务器之间通信的消息格式和类型
#include <QObject>
// 消息类型枚举：覆盖注册、登录、好友管理、文件操作、聊天等所有业务
enum ENUM_MSG_TYPE{
    ENUM_MSG_TYPE_MIN =0,           // 枚举起始值（占位）

    ENUM_MSG_TYPE_REGIST_REQUEST,   // 注册请求
    ENUM_MSG_TYPE_REGIST_RESPOND,   // 注册响应
    ENUM_MSG_TYPE_LOGIN_REQUEST,    // 登录请求
    ENUM_MSG_TYPE_LOGIN_RESPOND,    // 登录响应
    ENUM_MSG_TYPE_FIND_USER_REQUEST,    // 查找用户请求
    ENUM_MSG_TYPE_FIND_USER_RESPOND,    // 查找用户响应
    ENUM_MSG_TYPE_ONLINE_USER_REQUEST,  // 获取在线用户请求
    ENUM_MSG_TYPE_ONLINE_USER_RESPOND,  // 获取在线用户响应
    ENUM_MSG_TYPE_ADD_FRIEND_REQUEST,   // 添加好友请求
    ENUM_MSG_TYPE_ADD_FRIEND_RESPOND,   // 添加好友响应
    ENUM_MSG_TYPE_ADD_FRIEND_ARGEE_REQUEST, // 同意添加好友请求
    ENUM_MSG_TYPE_ADD_FRIEND_ARGEE_RESPOND, // 同意添加好友响应
    ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST, // 刷新好友列表请求
    ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND, // 刷新好友列表响应
    ENUM_MSG_TYPE_DEL_FRIEND_REQUEST,   // 删除好友请求
    ENUM_MSG_TYPE_DEL_FRIEND_RESPOND,   // 删除好友响应
    ENUM_MSG_TYPE_CHAT_REQUEST,         // 聊天消息请求
    ENUM_MSG_TYPE_CHAT_RESPOND,         // 聊天消息响应
    ENUM_MSG_TYPE_MKDIR_REQUEST,        // 新建文件夹请求
    ENUM_MSG_TYPE_MKDIR_RESPOND,        // 新建文件夹响应
    ENUM_MSG_TYPE_FLUSH_FILE_REQUEST,   // 刷新文件列表请求
    ENUM_MSG_TYPE_FLUSH_FILE_RESPOND,   // 刷新文件列表响应
    ENUM_MSG_TYPE_DEL_DIR_REQUEST,      // 删除文件夹请求
    ENUM_MSG_TYPE_DEL_DIR_RESPOND,      // 删除文件夹响应
    ENUM_MSG_TYPE_UPLOAD_FILE_INIT_REQUEST,  // 上传文件初始化请求（传递文件名和大小）
    ENUM_MSG_TYPE_UPLOAD_FILE_INIT_RESPOND,  // 上传文件初始化响应
    ENUM_MSG_TYPE_UPLOAD_FILE_DATA_REQUEST,  // 上传文件数据块请求
    ENUM_MSG_TYPE_UPLOAD_FILE_DATA_RESPOND,  // 上传文件数据块响应
    ENUM_MSG_TYPE_SHARE_FILE_REQUEST,        // 分享文件请求
    ENUM_MSG_TYPE_SHARE_FILE_RESPOND,        // 分享文件响应
    ENUM_MSG_TYPE_SHARE_FILE_ARGEE_REQUEST,  // 同意接收分享文件请求
    ENUM_MSG_TYPE_SHARE_FILE_ARGEE_RESPOND,  // 同意接收分享文件响应
    ENUM_MSG_TYPE_UPLOAD_FILE_DATA_ACK,      // 上传数据块确认（流控：每块回一个已收字节数）


    ENUM_MSG_TYPE_MAX=0x00ffffff        // 枚举最大值（占位，用于范围校验）
};

// 文件信息结构体：用于传输文件列表中每个文件/目录的基本信息
struct FileInfo{
    char caName[32];    // 文件或目录名（定长32字节）
    int iFileType;      // 文件类型：0=目录，1=文件
};

// PDU（协议数据单元）：客户端与服务器通信的统一数据包格式
struct PDU{
    unsigned int uiPDULen;  // PDU总长度 = sizeof(PDU) + uiMsgLen
    unsigned int uiMsgLen;  // 实际消息长度（柔性数组caMsg的长度）
    unsigned int uiMsgType; // 消息类型，对应ENUM_MSG_TYPE枚举值
    char caData[64];        // 附带参数（前32字节通常为当前用户名，后32字节为目标用户名）
    char caMsg[];           // 柔性数组，存放实际消息体（可变长度）
};

// 上传分块大小：64KB（原来4096，块数太多、固定开销大）
#define UPLOAD_BLOCK_SIZE (64*1024)


// 根据消息长度在堆上分配PDU空间，初始化并返回指针
PDU* mkPDU(unsigned int uiMsgLen=0);

// 安全字符串拷贝：最多拷贝 dstSize-1 字节，末尾补 NUL，避免越界读源
inline void strncpySafe(char* dst, int dstSize, const char* src)
{
    if(dst == NULL || src == NULL || dstSize <= 0){
        return;
    }
    int n = 0;
    while(n < dstSize - 1 && src[n] != '\0'){
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
}


#endif // PROTOCOL_H
