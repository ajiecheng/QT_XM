#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <QObject>
#include <QSqlDatabase>
#include <QMutex>
#include <QHash>

// 数据库连接池（Qt 正确版）
//
// 核心前提：QSqlDatabase 有"线程亲和性"——一条连接只能在创建它的线程里使用，
// 跨线程用会报错。所以池子采用"每线程一条连接"的模型：
//   * 同一线程反复 borrow()，拿到的是它自己那条连接（复用）；
//   * 连接总数有上限 m_maxConnections；
//   * 借出前做心跳检测(SELECT 1)，被 MySQL 掐断就自动重连；
//   * 借/还都用互斥锁保护，多线程安全。
class ConnectionPool : public QObject
{
    Q_OBJECT
public:
    static ConnectionPool& getInstance();   // 单例入口（规范第1条）

    QSqlDatabase borrow();                  // 借一条当前线程的连接
    void         release();                 // 归还（标记为空闲）

    void setMaxConnections(int n);
    void   setMaxIdleMs(qint64 ms);      // ← 新增：设置空闲超时（毫秒）
    int  connectionCount();                 // 当前登记了多少条连接（调试用）

    ~ConnectionPool();

private:
    explicit ConnectionPool(QObject* parent = nullptr);       // 规范第2条
    ConnectionPool(const ConnectionPool& instance) = delete;  // 规范第3条
    ConnectionPool& operator=(const ConnectionPool&) = delete; // 规范第4条

    QString      connNameForThread() const;          // 用线程ID生成唯一连接名
    bool         isAlive(const QSqlDatabase& db);     // SELECT 1 心跳
    QSqlDatabase createNew(const QString& name);      // 建连接并打开

    QMutex m_mutex;
    int    m_maxConnections = 8;     // 连接上限（配合后面固定 8 线程）
    qint64 m_maxIdleMs = 7LL * 3600 * 1000;      // ← 新增：空闲超时，默认 60 秒
    // 登记表：连接名 -> 最近一次 release 的时刻（0 = 正在使用中）
    // 用自己这张表统计数量，而不是 QSqlDatabase::connectionNames()，
    // 因为 connectionNames() 只返回"当前线程可见"的连接，跨线程统计会数错。
    QHash<QString, qint64> m_idleSince;
};

#endif // CONNECTIONPOOL_H
