#ifndef SCOPEDCONNECTION_H
#define SCOPEDCONNECTION_H

#include <QSqlDatabase>
#include "connectionpool.h"

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
