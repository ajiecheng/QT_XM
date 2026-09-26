# 基于 Qt 的网络云盘系统（Cloud Disk）

这是一个使用 **C++ / Qt** 编写的 C/S 架构网络云盘系统，包含**客户端**和**服务器端**两部分。支持用户注册登录、好友管理、即时聊天、云盘文件的上传/下载/分享、目录管理等完整功能。

> 这是一个学习项目，代码注释为中文，适合用来理解 Qt 网络编程、TCP 通信协议设计和 MySQL 数据库操作。

---

## 目录结构

```
QT/
├── Clien/        # 客户端（GUI 程序）
│   ├── client.*      # 客户端主类：TCP 连接、配置加载、消息收发路由
│   ├── reshandler.*  # 响应处理器：处理服务器返回的各种响应
│   ├── uploader.*    # 文件上传器：分块上传、断点续传
│   ├── file.*        # 云盘文件窗口
│   ├── friend.*      # 好友列表窗口
│   ├── chat.*        # 聊天窗口
│   ├── sharefile.*   # 分享文件窗口
│   ├── index.*       # 登录/注册界面
│   ├── onlineuser.*  # 在线用户列表
│   ├── protocol.*    # 通信协议（PDU 定义、消息类型枚举）
│   ├── main.cpp      # 客户端入口
│   ├── client.config # 客户端配置文件（服务器 IP / 端口 / 文件根路径）
│   └── Clien.pro     # 客户端 Qt 工程文件
│
└── Server/       # 服务器端（TCP 服务器）
    ├── server.*        # 服务器主类：加载配置、启动 TCP 监听
    ├── myqtcpserver.*  # 自定义 TCP 服务器：管理在线连接
    ├── mytcpsocket.*   # 自定义 TCP Socket：粘包处理、消息收发
    ├── msghandler.*    # 消息处理器：核心业务逻辑（注册/登录/好友/文件/聊天）
    ├── operatedb.*     # 数据库操作类：MySQL 连接、用户/好友增删查改
    ├── connectionpool.*    # 数据库连接池
    ├── scopedconnection.*  # RAII 数据库连接封装
    ├── scopedtransaction.* # RAII 事务封装
    ├── protocol.*      # 通信协议（与客户端一致）
    ├── main.cpp        # 服务器入口
    ├── server.config   # 服务器配置文件
    └── Server.pro      # 服务器 Qt 工程文件
```

---

## 功能特性

- **用户系统**：注册、登录、查找用户、在线状态管理
- **好友系统**：添加好友、同意申请、删除好友、刷新好友列表
- **即时聊天**：好友之间实时文字聊天
- **云盘文件**：
  - 新建/删除文件夹
  - 刷新文件列表
  - 文件**分块上传**（64KB 分块、断点续传、MD5 校验）
  - 文件**分享**给好友、同意接收分享
- **通信协议**：自定义 PDU（协议数据单元）格式，柔性数组支持变长消息

---

## 技术栈

| 项目 | 说明 |
| ---- | ---- |
| 语言 | C++11 |
| 框架 | Qt 5（Core / Gui / Network / Sql / Widgets 模块） |
| 数据库 | MySQL |
| 网络 | Qt Network（QTcpSocket / QTcpServer） |
| 构建 | qmake（.pro 工程文件） |

---

## 通信协议（PDU）

客户端与服务器通过统一的 PDU 结构通信：

```cpp
struct PDU {
    unsigned int uiPDULen;   // PDU 总长度 = sizeof(PDU) + uiMsgLen
    unsigned int uiMsgLen;   // 消息体长度（柔性数组 caMsg 的长度）
    unsigned int uiMsgType;  // 消息类型（对应 ENUM_MSG_TYPE 枚举）
    char caData[64];         // 参数区（前 32 字节：当前用户名；后 32 字节：目标用户名）
    char caMsg[];            // 柔性数组，存放实际消息体
};
```

消息类型定义在 `protocol.h` 的 `ENUM_MSG_TYPE` 枚举中，覆盖注册、登录、好友、聊天、文件操作等全部业务。

---

## 环境要求

- 操作系统：Linux / Windows / macOS 均可（项目在 Linux 下开发）
- Qt 5.x（含 Qt Network、Qt Sql 模块）
- MySQL 5.7+ 及 Qt 的 MySQL 驱动（`libqsqlmysql.so`）
- C++11 编译器（g++ / MSVC / MinGW）

---

## 数据库初始化

在 MySQL 中创建数据库和表（服务器端使用）：

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

> 数据库连接信息（主机、端口、用户名、密码）需在服务器端代码中配置（`Server/operatedb.cpp` 或对应配置）。

---

## 编译与运行

### 1. 编译服务器端

```bash
cd Server
qmake Server.pro
make
./Server
```

### 2. 编译客户端

```bash
cd Clien
qmake Clien.pro
make
./Clien
```

或者直接用 Qt Creator 打开 `.pro` 工程文件，配置好构建套件后点击运行。

### 3. 配置说明

`client.config` / `server.config` 文件内容格式：

```
<服务器 IP>
<端口号>
<云盘文件根路径>
```

示例（默认值）：

```
127.0.0.1
5000
./filesys
```

> 运行前请确保服务器端已启动并连接好 MySQL 数据库，文件根路径目录存在且有读写权限。

---

## 使用说明

1. 先启动**服务器端**（`Server`）。
2. 再启动**客户端**（`Clien`），在登录界面注册账号或登录。
3. 登录后可进行：查找/添加好友、与好友聊天、管理云盘文件、上传/分享文件等操作。

---
