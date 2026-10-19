#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <QObject>
#include <QSqlDatabase>
#include <QMutex>
#include <QHash>


class ConnectionPool : public QObject
{
    Q_OBJECT
public:
    static ConnectionPool& getInstance();   

    QSqlDatabase borrow();                  // 借一条当前线程的连接
    void         release();                 // 归还（标记为空闲）

    void setMaxConnections(int n);
    void   setMaxIdleMs(qint64 ms);      // 设置空闲超时（毫秒）
    int  connectionCount();                 // 当前登记了多少条连接（调试用）

    ~ConnectionPool();

private:
    explicit ConnectionPool(QObject* parent = nullptr);       
    ConnectionPool(const ConnectionPool& instance) = delete;  
    ConnectionPool& operator=(const ConnectionPool&) = delete; 

    QString      connNameForThread() const;          // 用线程ID生成唯一连接名
    bool         isAlive(const QSqlDatabase& db);     // SELECT 1 心跳
    QSqlDatabase createNew(const QString& name);      // 建连接并打开

    QMutex m_mutex;
    int    m_maxConnections = 8;     // 连接上限（配合后面固定 8 线程）
    qint64 m_maxIdleMs = 7LL * 3600 * 1000;      // 空闲超时
    QHash<QString, qint64> m_idleSince;
};

#endif // CONNECTIONPOOL_H
