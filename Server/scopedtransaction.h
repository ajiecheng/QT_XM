#ifndef SCOPEDTRANSACTION_H
#define SCOPEDTRANSACTION_H

#include <QSqlDatabase>

// RAII 事务守卫：构造时开事务，析构时自动决定 commit 还是 rollback。
// 好处：函数无论从哪条路径 return，都不会"忘关事务"——
//       忘 commit 会让连接一直停在手动事务模式，污染下一个用这条连接的人。
class ScopedTransaction
{
public:
    // 构造：立刻开启事务（等价于 MySQL 的 START TRANSACTION）
    explicit ScopedTransaction(QSqlDatabase db)
        : m_db(db), m_committed(false)
    {
        m_db.transaction();
    }

    // 析构：只要没被显式 commit，就自动 rollback 兜底
    ~ScopedTransaction()
    {
        if (!m_committed) {
            m_db.rollback();
        }
    }

    // 业务成功时调用它：提交并标记"已提交，析构别再回滚了"
    void commit()
    {
        if (m_db.commit()) {
            m_committed = true;   // 提交成功才置位；失败则留给析构 rollback
        }
    }

    // 禁止拷贝：一个事务只能有一个"收尾权"
    ScopedTransaction(const ScopedTransaction&) = delete;
    ScopedTransaction& operator=(const ScopedTransaction&) = delete;

private:
    QSqlDatabase m_db;    // QSqlDatabase 是值语义句柄，拷贝不新建连接，指向同一条连接
    bool m_committed;     // 是否已经提交过
};

#endif // SCOPEDTRANSACTION_H
