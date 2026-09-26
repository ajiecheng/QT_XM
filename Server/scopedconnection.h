#ifndef SCOPEDCONNECTION_H
#define SCOPEDCONNECTION_H

#include <QSqlDatabase>
#include "connectionpool.h"

// RAII 连接守卫：构造时借一条连接，析构时自动归还。
// 好处：函数无论从哪条路径 return，都会自动 release，
//       杜绝"忘了还"导致连接一直处于'使用中'、无法被空闲回收的问题。
class ScopedConnection
{
public:
    ScopedConnection() { m_db = ConnectionPool::getInstance().borrow(); }
    ~ScopedConnection() { if (m_db.isValid()) ConnectionPool::getInstance().release(); }

    QSqlDatabase db() const { return m_db; }
    bool isValid() const    { return m_db.isValid(); }

    // 禁止拷贝：借/还必须成对出现，拷贝会破坏这个约定
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

private:
    QSqlDatabase m_db;
};

#endif // SCOPEDCONNECTION_H
