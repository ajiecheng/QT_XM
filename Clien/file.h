#ifndef FILE_H
#define FILE_H
// 文件管理页面头文件：浏览、新建、删除文件夹，上传、分享文件
#include "protocol.h"
#include "sharefile.h"

#include <QTreeWidget>
#include <QWidget>

namespace Ui {
class File;
}

class File : public QWidget
{
    Q_OBJECT

public:
    explicit File(QWidget *parent = nullptr);
    ~File();
    QString m_strUserPath;                 // 用户的根路径（由rootPath+用户名组成）
    QString m_strCurPath;                  // 当前选中目录的路径（供上传/新建/删除/分享使用）
    QString m_strUploadFilePath;           // 准备上传的本地文件路径
    void flushFile();                      // 重新加载整棵目录树
    void loadDirIntoItem(QTreeWidgetItem* parentItem, const QString& dirPath); // 请求某目录的子项并挂到父节点
    void updateFileList(QList<FileInfo*> pFileList); // 用服务器返回的数据填充当前父节点
    QList<FileInfo*> m_pFileInfoList;      // 最近一次请求目录下的FileInfo列表
    void uploadFile();                     // 在主线程中循环读取本地文件并发送数据块（旧方案）
    QString m_strShareFileName;            // 当前选中要分享的文件名
    ShareFile* m_pShareFile;               // 分享文件对话框
    void resetUploadProgress();            // 上传结束后把进度条复位为0

    QTreeWidgetItem* m_pRootItem;          // 目录树的根节点
    QTreeWidgetItem* m_pCurParentItem;     // 当前正在加载子项的父节点

private slots:
    void on_mkdir_PB_clicked();            // 新建文件夹
    void on_flushFile_PB_clicked();        // 手动刷新目录树
    void on_deldir_PB_clicked();           // 删除选中的文件夹
    void on_return_PB_clicked();           // 折叠当前节点（树模式下代替"返回上级"）
    void on_uploadFile_PB_clicked();       // 选择本地文件上传
    void on_shareFile_PB_clicked();        // 分享选中文件给好友
    void onTreeItemExpanded(QTreeWidgetItem* item);            // 展开目录→懒加载子项
    void onTreeCurrentItemChanged(QTreeWidgetItem* cur, QTreeWidgetItem* prev); // 选中变化→更新m_strCurPath

public slots:
   void updateProgress(int percent);        // 槽：更新上传进度条（由子线程信号触发，主线程执行）

private:
    Ui::File *ui;
};

#endif // FILE_H
