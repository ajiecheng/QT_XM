#ifndef INDEX_H
#define INDEX_H
// 主界面（首页）头文件：使用QStackedWidget管理好友页面和文件页面
#include "file.h"
#include "friend.h"

#include <QWidget>

namespace Ui {
class Index;
}

class Index : public QWidget
{
    Q_OBJECT

public:
    static Index& getInstance();     // 单例模式获取主界面实例
    ~Index();
    Friend * getFriend();            // 获取好友页面指针（嵌套在stackedWidget中）
    File* getFile();                 // 获取文件页面指针（嵌套在stackedWidget中）

private slots:
    void on_firend_PB_clicked();     // 切换stackedWidget到好友页面（索引0）
    void on_file_PB_clicked();       // 切换stackedWidget到文件页面（索引1）

private:
    explicit Index(QWidget *parent = nullptr);
    Index(const Index& instance)=delete;       // 禁止拷贝构造
    Index& operator=(const Index&)=delete;     // 禁止拷贝赋值
    Ui::Index *ui;
};

#endif // INDEX_H
