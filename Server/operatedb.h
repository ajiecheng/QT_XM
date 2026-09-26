#ifndef OPERATEDB_H
#define OPERATEDB_H
// 数据库操作类：封装MySQL连接、用户管理、好友关系等所有数据库操作
#include <QObject>
#include <QSqlDatabase>
class OperateDb : public QObject
{
    Q_OBJECT
public:
    static OperateDb& getInstance();                   // 单例模式获取数据库操作实例
   // void connectDb();                                   // 连接MySQL数据库
    //QSqlDatabase db();          // 获取当前线程的独立数据库连接（没有则创建）
    //void releaseDb();           // 释放当前线程的数据库连接（线程退出时调用）
    // --- 用户账号相关 ---
    bool handleRegist(const char * caName, const char * caPwd);  // 注册：检查重复后插入user_info表
    bool handleLogin(const char * caName, const char * caPwd);   // 登录：验证账号密码，设置online=1
    void handleOffline(const char * caName);                     // 离线：设置online=0
    int handleFindUser(const char * caName);                     // 查找用户：返回在线状态（-1失败/0离线/1在线/2不存在）
    QStringList handleOnlineUser();                              // 获取在线用户：查询所有online=1的用户名

    // --- 好友关系相关 ---
     int handleAddFirend(const char * caCurName, const char * caTarName);        // 添加好友：检查是否已为好友及目标是否在线
     bool handleAddFirendArgee(const char * caCurName, const char * caTarName);  // 同意添加：插入friend表记录
     QStringList handleFlushFriend(const char *caCurName);                       // 刷新好友：查询当前用户的所有好友
     bool handleDelFirend(const char * caCurName, const char * caTarName);       // 删除好友：从friend表删除关系

    ~OperateDb();

private:
    explicit OperateDb(QObject *parent = nullptr);
     //数据库连接，单例实现，
    OperateDb(const OperateDb& instance) = delete;
    OperateDb& operator=(const OperateDb&) = delete;
    //QSqlDatabase m_db;                                   // Qt SQL数据库连接对象

signals:

};

#endif // OPERATEDB_H
