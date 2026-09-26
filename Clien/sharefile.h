#ifndef SHAREFILE_H
#define SHAREFILE_H
// 分享文件对话框头文件：选择好友并发送文件分享
#include <QWidget>

namespace Ui {
class ShareFile;
}

class ShareFile : public QWidget
{
    Q_OBJECT

public:
    explicit ShareFile(QWidget *parent = nullptr);
    ~ShareFile();
    void updateFirend_LW();                  // 从好友页面复制好友列表到分享对话框

private slots:
    void on_allSelect_PB_clicked();          // 全选所有好友
    void on_cancelSelect_PB_clicked();       // 取消全部选择
    void on_ok_PB_clicked();                 // 确认分享：构建分享PDU发送给服务器

private:
    Ui::ShareFile *ui;
};

#endif // SHAREFILE_H
