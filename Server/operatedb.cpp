#include "operatedb.h"
#include "connectionpool.h"      // ← 新增
#include "scopedtransaction.h"   // ← 加这一行
#include "scopedconnection.h"    // ← 加这一行，解决借用数据库连接线程的归还问题
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
// 数据库操作实现：MySQL连接、用户管理、好友关系管理

// 单例实现：返回全局唯一的数据库操作实例
OperateDb& OperateDb::getInstance()
{
    static OperateDb instance;
    return instance;
}

// 连接MySQL数据库：本机localhost、3306端口、mydb2409库
/* void OperateDb::connectDb()
{
    m_db.setHostName("localhost");                   // 数据库主机地址
    m_db.setPort(3306);                              // MySQL默认端口
    m_db.setDatabaseName("mydb2409");                // 数据库名称
    m_db.setUserName("root");                        // 数据库用户名
    m_db.setPassword("123456");                      // 数据库密码
    if(m_db.open()){                                 // 尝试打开数据库连接
        qDebug() << "数据库连接成功";
    }else{
        qDebug() << "数据库连接失败" << m_db.lastError().text();
    }
}
*/
// 获取当前线程的独立数据库连接：用线程ID做连接名，第一次访问时创建并打开
/* QSqlDatabase OperateDb::db()
{
    QString connName = QString("conn_%1").arg((quintptr)QThread::currentThreadId());
    if(QSqlDatabase::contains(connName)){                 // 本线程已创建过
        return QSqlDatabase::database(connName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", connName); // 本线程新建独立连接
    db.setHostName("localhost");
    db.setPort(3306);
    db.setDatabaseName("mydb2409");
    db.setUserName("root");
    db.setPassword("123456");
    if(db.open()){
        qDebug() << "线程" << (quintptr)QThread::currentThreadId() << "数据库连接成功";
    }else{
        qDebug() << "线程" << (quintptr)QThread::currentThreadId() << "数据库连接失败" << db.lastError().text();
    }
    return db;
}

// 释放当前线程的数据库连接（该线程不再使用数据库时调用）
void OperateDb::releaseDb()
{
    QString connName = QString("conn_%1").arg((quintptr)QThread::currentThreadId());
    if(QSqlDatabase::contains(connName)){
        QSqlDatabase::removeDatabase(connName);
    }
}
  */
/*QSqlDatabase OperateDb::db()
{
    return ConnectionPool::getInstance().borrow();   // 借一条当前线程的连接
}

void OperateDb::releaseDb()
{
    ConnectionPool::getInstance().release();         // 归还（标记空闲）
}
*/
bool OperateDb::handleRegist(const char * caName,const char* caPwd)
{
    if(caName == NULL || caPwd == NULL){
        return false;
    }

    // 不再先 SELECT 查重，直接插入，靠 name 上的唯一索引 uk_name 兜底防重名
    QString sql = QString("insert into user_info(name,pwd) values('%1','%2')")
                      .arg(caName).arg(caPwd);
    qDebug()<<"handleRegist 添加用户"<<sql;

    //QSqlQuery q(db());
    ScopedConnection conn;  // 借连接，函数结束时析构自动归还
    QSqlQuery q(conn.db()); // 改用 conn 的连接

    q.exec(sql);

    if(q.lastError().isValid()){                     // 执行出了错
        // 错误码 1062 = Duplicate entry：唯一索引把重复用户名拦下了
        if(q.lastError().nativeErrorCode() == "1062"){
            qDebug()<<"handleRegist 用户名已存在";
            return false;
        }
        qDebug()<<"handleRegist 插入失败:"<<q.lastError().text();
        return false;
    }
    return true;                                     // 插入成功
}
/*  // 注册处理：先检查用户名是否已存在，不存在则插入user_info表
bool OperateDb::handleRegist(const char * caName,const char* caPwd)
{
    if(caName == NULL || caPwd == NULL){
        return false;
    }
    // 第一步：检查用户名是否已被注册
    QString sql = QString("select * from user_info where name = '%1'").arg(caName);
    qDebug()<<"handleRegist 判断用户是否存在"<<sql;
    //QSqlQuery q;
    QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接

    if(!q.exec(sql)){                                // SQL执行失败
        return false;
    }
    if(q.next()){                                    // 查询到结果说明用户名已存在
        return false;
    }
    // 第二步：用户名不存在，插入新用户记录
    sql=QString("insert into user_info(name,pwd) values('%1','%2')").arg(caName).arg(caPwd);
    qDebug()<<"handleRegist 添加用户"<<sql;
    return q.exec(sql);                              // 执行插入，返回成功/失败
}*/


// 登录处理：验证用户名密码，成功后设置在线状态
bool OperateDb::handleLogin(const char *caName, const char *caPwd)
{
    if(caName == NULL || caPwd == NULL){
        return false;
    }
    // 第一步：验证用户名和密码是否匹配
    QString sql = QString("select * from user_info where name = '%1' and pwd ='%2'").arg(caName).arg(caPwd);
    qDebug()<<"handleLogin 判断登录的用户名及密码是否正确"<<sql;
    //QSqlQuery q;
    //QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接
    ScopedConnection conn;  // 借连接，函数结束时析构自动归还
    QSqlQuery q(conn.db()); // 改用 conn 的连接

    if(!q.exec(sql)||!q.next()){                    // SQL失败或无匹配结果
        return false;
    }
    // 第二步：验证通过，将online字段置为1
    sql=QString("update user_info set online=1 where name = '%1' and pwd ='%2'").arg(caName).arg(caPwd);
    qDebug()<<"handleLogin 用户online置为1："<<sql;
    return q.exec(sql);
}

// 离线处理：将用户online字段置为0
void OperateDb::handleOffline(const char *caName)
{
    if(caName == NULL){
        return;
    }
   // QSqlQuery q;
    //QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接
    ScopedConnection conn;  // 借连接，函数结束时析构自动归还
    QSqlQuery q(conn.db()); // 改用 conn 的连接

    QString sql=QString("update user_info set online=0 where name = '%1'").arg(caName);
    qDebug()<<"handleoffline 用户online置为0："<<sql;
    q.exec(sql);
}

// 查找用户：查询目标用户是否存在及在线状态
// 返回值：-1=失败，0=离线，1=在线，2=用户不存在
int OperateDb::handleFindUser(const char *caName)
{
    if(caName == NULL){
        return -1;
    }
    QString sql = QString("select online from user_info where name = '%1'").arg(caName);
    qDebug()<<"handleFindUser 判断用户是否存在且在线"<<sql;
   // QSqlQuery q;
    //QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接
    ScopedConnection conn;  // 借连接，函数结束时析构自动归还
    QSqlQuery q(conn.db()); // 改用 conn 的连接

    if(!q.exec(sql)){                                // SQL执行失败
        return -1;
    }
    if(q.next()){                                    // 查询到该用户
        return q.value(0).toInt();                   // 返回online字段值（0或1）
    }
    return 2;                                        // 未查到结果，用户不存在
}

// 获取在线用户列表：查询所有online=1的用户名
QStringList OperateDb::handleOnlineUser()
{
    QString sql = QString("select name from user_info where online = 1");
    qDebug()<<"handleRegist 获取在线用户"<<sql;
    //QSqlQuery q;
    //QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接
    ScopedConnection conn;  // 借连接，函数结束时析构自动归还
    QSqlQuery q(conn.db()); // 改用 conn 的连接

    QStringList result;
    if(!q.exec(sql)){
        return result;                               // 失败返回空列表
    }
    while(q.next()){                                 // 遍历查询结果
        result.append(q.value(0).toString());        // 收集每个在线用户名
    }
    return result;
}


OperateDb::~OperateDb()
{
    //m_db.close();                                    // 析构时关闭数据库连接
}

// 添加好友：检查好友关系是否已存在、目标用户是否在线
// 返回值：-1=失败，0=目标离线，1=目标在线可添加，2=已是好友
int OperateDb::handleAddFirend(const char *caCurName, const char *caTarName)
{
    if(caCurName == NULL || caTarName == NULL){
        return -1;
    }
    // 第一步：检查是否已经是好友关系（friend表无方向性，需双向检查）
    QString sql = QString(R"(
                          select * from friend
                          where(
                          user_id = ( select id from user_info where name = '%1')
                          and
                          friend_id = ( select id from user_info where name = '%2')
                          )
                          or(
                          friend_id = ( select id from user_info where name = '%1')
                          and
                          user_id = ( select id from user_info where name = '%2')
                          )
                          )").arg(caCurName).arg(caTarName);
    qDebug()<<"handleAddFirend 是否为好友"<<sql;
    //QSqlQuery q;
    //QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接
    ScopedConnection conn;  // 借连接，函数结束时析构自动归还
    QSqlQuery q(conn.db()); // 改用 conn 的连接

    q.exec(sql);
    if(q.next()){                                    // 已有好友关系
        return 2;
    }
    // 第二步：不是好友的话，再查询目标用户是否在线
    sql = QString("select online from user_info where name = '%1'").arg(caTarName);
    qDebug()<<"handleAddFirend 目标用户是否在线"<<sql;
    q.exec(sql);
    if(q.next()){
        return q.value(0).toInt();                   // 返回online字段值（1=在线，0=离线）
    }
    return -1;                                       // 目标用户不存在
}

/* // 同意好友申请：在friend表中插入一对好友关系
bool OperateDb::handleAddFirendArgee(const char *caCurName, const char *caTarName)
{
    if(caCurName == NULL || caTarName == NULL){
        return false;
    }
    // 通过子查询分别获取两个用户的id，插入friend表
    QString sql = QString(R"(
                          insert into friend(user_id,friend_id)
                          select u1.id, u2.id
                          from user_info u1,user_info u2
                          where u1.name = '%1' and u2.name = '%2'
                          )").arg(caCurName).arg(caTarName);
     qDebug()<<"handleAddFirendArgee 添加好友记录"<<sql;
     //QSqlQuery q;
     QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接

     return q.exec(sql);
} */
// 同意好友申请：在friend表中双向插入两行好友关系（A→B 和 B→A），用事务保证成对
bool OperateDb::handleAddFirendArgee(const char *caCurName, const char *caTarName)
{
    if(caCurName == NULL || caTarName == NULL){
        return false;
    }
    ScopedConnection conn;              // ① 借连接，析构自动还
    QSqlDatabase db = conn.db();        // ② 拿到连接
    ScopedTransaction tx(db);           // ③ 开事务，析构自动 commit/rollback
    // ★ 关键：先把连接抓到变量里，transaction/commit/rollback 都调用在它上面
    //QSqlDatabase db = this->db();   // 借当前线程的连接
    //ScopedTransaction tx(db);       // 构造即开事务；函数退出时析构自动兜底

    // ① 正向一行：A 是 B 的好友
    //ScopedConnection conn;  // 借连接，函数结束时析构自动归还
    QSqlQuery q1(conn.db()); // 改用 conn 的连接

    //QSqlQuery q1(db);
    bool ok1 = q1.exec(QString(R"(
                        insert into friend(user_id,friend_id)
                        select u1.id, u2.id
                        from user_info u1,user_info u2
                        where u1.name = '%1' and u2.name = '%2'
                        )").arg(caCurName).arg(caTarName));
    // 防坑：INSERT...SELECT 若子查询查不到人，会"插入0行却不报错"
    //       所以这里还要确认真的插进了 1 行
    ok1 = ok1 && (q1.numRowsAffected() == 1);

    // ② 反向一行：B 是 A 的好友
    //QSqlQuery q2(db);
    QSqlQuery q2(conn.db()); // 改用 conn 的连接

    bool ok2 = q2.exec(QString(R"(
                        insert into friend(user_id,friend_id)
                        select u1.id, u2.id
                        from user_info u1,user_info u2
                        where u1.name = '%1' and u2.name = '%2'
                        )").arg(caTarName).arg(caCurName));
    ok2 = ok2 && (q2.numRowsAffected() == 1);

    // ③ 两条都成功 → 提交；否则什么都不用写，ScopedTransaction 析构自动 rollback
    if (ok1 && ok2) {
        tx.commit();
        return true;
    }
    return false;
}

// 刷新好友列表：双向存两行后，A 的好友 = friend 表里所有 user_id=A 的 friend_id
QStringList OperateDb::handleFlushFriend(const char *caCurName)
{
    QStringList res;
    if(caCurName == NULL){
        return res;
    }
    // 双向存储后无需 union：查一侧即可拿到全部好友
    QString sql = QString(R"(select name from user_info
                          where id in
                          (
                          select friend_id from friend
                          where user_id = (select id from user_info where name='%1')
                          ))").arg(caCurName);

    //QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接
    ScopedConnection conn;  // 借连接，函数结束时析构自动归还
    QSqlQuery q(conn.db()); // 改用 conn 的连接

    q.exec(sql);
    while(q.next()){
        res.append(q.value(0).toString());           // 收集所有好友用户名
    }
    qDebug()<<"handleFlushFriend 刷新好友";
    return res;
}
/* // 刷新好友列表：用union双向查询，获取当前用户的所有好友用户名
QStringList OperateDb::handleFlushFriend(const char *caCurName)
{
    QStringList res;
    if(caCurName == NULL){
        return res;
    }
    // friend表只存用户id，需通过子查询分别查询"我添加的"和"添加我的"好友
    QString sql = QString(R"(select name from user_info
                          where id in
                          (
                          select friend_id from friend
                          where user_id = (select id from user_info where name='%1')
                          union
                          select user_id from friend
                          where friend_id = (select id from user_info where name='%1')
                          ))").arg(caCurName);
    //QSqlQuery q;
    QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接

    q.exec(sql);
    while(q.next()){
        res.append(q.value(0).toString());           // 收集所有好友用户名
    }
    qDebug()<<"handleFlushFriend 刷新好友";
    return res;
} */

// 删除好友：双向两行用一条 DELETE 同时删掉，numRowsAffected 判断是否成功
bool OperateDb::handleDelFirend(const char *caCurName, const char *caTarName)
{
    if(caCurName == NULL || caTarName == NULL){
        return false;
    }
    // 一条 DELETE 用 or 匹配 (A,B) 和 (B,A) 两行，天然原子，不需要事务
    QString sql = QString(R"(
                  delete from friend
                  where (
                  user_id = ( select id from user_info where name = '%1')
                  and
                  friend_id = ( select id from user_info where name = '%2')
                  )
                  or (
                  user_id = ( select id from user_info where name = '%2')
                  and
                  friend_id = ( select id from user_info where name = '%1')
                  )
                  )").arg(caCurName).arg(caTarName);
    qDebug()<<"handleDelFirend 删除好友关系"<<sql;

    //QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接
    ScopedConnection conn;  // 借连接，函数结束时析构自动归还
    QSqlQuery q(conn.db()); // 改用 conn 的连接

    q.exec(sql);
    if(q.lastError().isValid()){                    // SQL 执行失败
        qDebug()<<"handleDelFirend 删除失败:"<<q.lastError().text();
        return false;
    }
    // 删了 0 行 = 本来就不是好友；删了 2 行 = 成功删掉双向两行
    return q.numRowsAffected() > 0;
}
/*// 删除好友：先检查是否存在好友关系，确认后从friend表删除
bool OperateDb::handleDelFirend(const char *caCurName, const char *caTarName)
{
    if(caCurName == NULL || caTarName == NULL){
        return false;
    }
    // 第一步：检查是否确实是好友关系
    QString sql = QString(R"(
                          select * from friend
                          where(
                          user_id = ( select id from user_info where name = '%1')
                          and
                          friend_id = ( select id from user_info where name = '%2')
                          )
                          or(
                          friend_id = ( select id from user_info where name = '%1')
                          and
                          user_id = ( select id from user_info where name = '%2')
                          )
                          )").arg(caCurName).arg(caTarName);
    qDebug()<<"handleDelFirend 是否为好友"<<sql;
    //QSqlQuery q;
    QSqlQuery q(db());   // 绑定当前线程的连接，而不是默认连接

    q.exec(sql);
    if(!q.next()){                                   // 不存在好友关系
        return false;
    }
    // 第二步：确认存在好友关系，执行删除
    sql = QString(R"(
                  delete from friend
                  where(
                  user_id = ( select id from user_info where name = '%1')
                  and
                  friend_id = ( select id from user_info where name = '%2')
                  )
                  or(
                  friend_id = ( select id from user_info where name = '%1')
                  and
                  user_id = ( select id from user_info where name = '%2')
                  )
                  )").arg(caCurName).arg(caTarName);
    qDebug()<<"handleDelFirend 删除好友关系"<<sql;
    return q.exec(sql);
}
 */


// 构造函数：初始化Qt MySQL数据库驱动。改成按需后这个构造函数没有被调用
OperateDb::OperateDb(QObject *parent) : QObject(parent)
{
    //m_db = QSqlDatabase::addDatabase("QMYSQL");      // 添加MySQL数据库驱动
}
