#include "connectionpool.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QDateTime>

ConnectionPool& ConnectionPool::getInstance()
{
    static ConnectionPool instance;
    return instance;
}

ConnectionPool::ConnectionPool(QObject* parent) : QObject(parent)
{
}

ConnectionPool::~ConnectionPool()
{
    // 程序退出时关闭池内连接，运行期借/还
    QMutexLocker locker(&m_mutex);
    for (const QString& name : m_idleSince.keys()) {
        QSqlDatabase::removeDatabase(name);
    }
}

// 连接名跟线程ID绑定，一个线程只有一条连接
QString ConnectionPool::connNameForThread() const
{
    return QString("pool_%1").arg((quintptr)QThread::currentThreadId());
}

// 心跳检测：执行 SELECT 1 
bool ConnectionPool::isAlive(const QSqlDatabase& db)
{
    if (!db.isOpen())
        return false;
    QSqlQuery q(db);
    return q.exec("SELECT 1");
}

QSqlDatabase ConnectionPool::createNew(const QString& name)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", name);
    db.setHostName("localhost");
    db.setPort(3306);
    db.setDatabaseName("mydb2409");
    db.setUserName("root");
    db.setPassword("123456");
    if (!db.open()) {
        qDebug() << "[ConnectionPool] 新建连接失败:" << db.lastError().text();
    }
    return db;
}

QSqlDatabase ConnectionPool::borrow()
{
    const QString name = connNameForThread();
    QMutexLocker locker(&m_mutex);

    // 心跳检测，活着就用
    if (m_idleSince.contains(name)) {
        {
            QSqlDatabase db = QSqlDatabase::database(name);

            bool tooIdle = false;
            if (m_idleSince[name] != 0) {
                qint64 idleMs = QDateTime::currentMSecsSinceEpoch() - m_idleSince[name];
                tooIdle = idleMs > m_maxIdleMs;
            }

            // 连接还活着 且 没闲置超时 
            if (isAlive(db) && !tooIdle) {
                m_idleSince[name] = 0;      // 标记使用
                return db;
            }

            // 连接失效||闲置超时，关掉重建
            if (tooIdle) {
                qDebug() << "[ConnectionPool] 连接闲置超时，回收重建" << name;
            } else {
                qDebug() << "[ConnectionPool] 连接已失效，重连" << name;
            }
            db.close();
        } // 作用域结束 db 析构，之后才能 removeDatabase

        QSqlDatabase::removeDatabase(name);
        m_idleSince.remove(name);
    }


    // 情况2：需要新建，先检查是否超过上限。
    // 连接是线程绑定的，别的线程还回来的连接我也用不了，
    // 所以池满时"等待"没有意义，直接拒绝（返回无效连接）。
    if (m_idleSince.size() >= m_maxConnections) {
        qDebug() << "[ConnectionPool] 已达上限" << m_maxConnections
                 << "，拒绝新建（线程" << (quintptr)QThread::currentThreadId() << "）";
        return QSqlDatabase();            
    }

    QSqlDatabase db = createNew(name);
    m_idleSince[name] = 0;
    // 新建连接时打印一次：连接名（含线程ID） + 当前连接总数
        qDebug() << "[ConnectionPool] 新建连接" << name
                 << "，当前总数" << m_idleSince.size();
    return db;
}

void ConnectionPool::release()
{
    const QString name = connNameForThread();
    QMutexLocker locker(&m_mutex);
    if (m_idleSince.contains(name)) {
        m_idleSince[name] = QDateTime::currentMSecsSinceEpoch();  // 记录空闲时刻
    }
}

void ConnectionPool::setMaxConnections(int n)
{
    QMutexLocker locker(&m_mutex);   // 与 borrow 里读 m_maxConnections 互斥
    m_maxConnections = n;
}

void ConnectionPool::setMaxIdleMs(qint64 ms)
{
    QMutexLocker locker(&m_mutex);
    m_maxIdleMs = ms;
}

int ConnectionPool::connectionCount()
{
    QMutexLocker locker(&m_mutex);
    return m_idleSince.size();
}
