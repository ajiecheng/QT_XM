# 基于 Qt 的网盘即时通讯系统

基于QT平台提供的事件循环机制、信号槽机制和GUI界面实现的项目开发。采用C/S架构，支持用户注册登录、好友管理、即时聊天、云盘文件的上传/下载/分享、目录管理等功能。

> 学习项目，用来理解 Qt 网络编程、TCP 通信协议设计和 MySQL 数据库操作以及数据流业务处理逻辑。

## 技术栈
C++11、Qt、MySQL、TCP/IP 套接字编程、多线程、自定义通信协议

## 通信协议
```C++
自定义应用层二进制通信协议
客户端与服务器通过统一的 PDU 结构通信：
struct PDU {
    unsigned int uiPDULen;   // PDU 总长度 = sizeof(PDU) + uiMsgLen
    unsigned int uiMsgLen;   // 消息体长度（柔性数组 caMsg 的长度）
    unsigned int uiMsgType;  // 消息类型（对应 ENUM_MSG_TYPE 枚举）
    char caData[64];         // 参数区（前 32 字节：当前用户名；后 32 字节：目标用户名）
    char caMsg[];            // 柔性数组，存放实际消息体
};
```
## 环境要求
- Qt 5.4以上（含 Qt Network、Qt Sql 模块）
- MySQL 5.7以上
---

## 数据库初始化
在 MySQL 中创建数据库和表：
```sql
CREATE DATABASE cloud_disk DEFAULT CHARACTER SET utf8;
USE cloud_disk;
-- 用户表
CREATE TABLE user_info (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(32) NOT NULL UNIQUE,     -- 用户名
    pwd  VARCHAR(32) NOT NULL,            -- 密码
    online TINYINT DEFAULT 0              -- 在线状态：0 离线 / 1 在线
);
-- 好友关系表
CREATE TABLE friend (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(32) NOT NULL,            -- 用户
    friend_name VARCHAR(32) NOT NULL      -- 好友
);
```
> 数据库连接信息需在服务器端代码中配置
---

## 编译与运行
直接用 Qt Creator 打开 `.pro` 工程文件，配置好构建套件后点击运行。

### 3. 配置说明

`client.config` / `server.config` 文件内容格式：
```
127.0.0.1
5000
./filesys
```
> 运行前请确保服务器端已启动并连接好 MySQL 数据库，文件根路径目录存在且有读写权限。
---

## 使用说明
1. 先启动服务器端（`Server`）。
2. 再启动客户端（`Clien`），在登录界面注册账号或登录。
3. 登录后可进行：查找/添加好友、与好友聊天、管理云盘文件、上传/分享文件等操作。
---
