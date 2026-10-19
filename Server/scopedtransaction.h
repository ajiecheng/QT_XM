#ifndef SCOPEDTRANSACTION_H
#define SCOPEDTRANSACTION_H

#include <QSqlDatabase>

class ScopedTransaction
{
public:
    explicit ScopedTransaction(QSqlDatabase db)
        : m_db(db), m_committed(false)
    {
        m_db.transaction();
    }
    ~ScopedTransaction()
    {
        if (!m_committed) {
            m_db.rollback();
        }
    }

    void commit()
    {
        if (m_db.commit()) {
            m_committed = true;   // 提交成功
        }
    }

    // 禁止拷贝
    ScopedTransaction(const ScopedTransaction&) = delete;
    ScopedTransaction& operator=(const ScopedTransaction&) = delete;

private:
    QSqlDatabase m_db;    
    bool m_committed;     // 是否已经提交过
};

#endif 
