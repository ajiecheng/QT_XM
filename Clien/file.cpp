#include "client.h"
#include "file.h"
#include "ui_file.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QCryptographicHash>//MD5头文件

// 文件管理页面实现：目录导航、文件上传、文件夹操作、文件分享

// 构造函数：初始化路径并首次刷新文件列表
File::File(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::File)
{
    ui->setupUi(this);
    m_strUserPath = QString("%1/%2").arg(Client::getInstance().m_strRootPath)
                        .arg(Client::getInstance().m_strUserName);  // 用户根路径
    m_strCurPath = m_strUserPath;                                    // 初始当前目录 = 根
    m_pShareFile = new ShareFile;                                    // 创建分享文件对话框

    // ---- 目录树相关初始化 ----
    connect(ui->treeWidget, &QTreeWidget::itemExpanded,
            this, &File::onTreeItemExpanded);                        // 展开→懒加载
    connect(ui->treeWidget, &QTreeWidget::currentItemChanged,
            this, &File::onTreeCurrentItemChanged);                  // 选中变化→更新当前目录

    ui->treeWidget->setHeaderLabel("我的网盘");                       // 顶部显示一列标题
    m_pRootItem = new QTreeWidgetItem;                               // 创建根节点
    m_pRootItem->setText(0, m_strUserPath);                          // 根节点显示用户根路径
    m_pRootItem->setIcon(0, QIcon(QPixmap(":/dir.png")));            // 文件夹图标
    m_pRootItem->setData(0, Qt::UserRole,   m_strUserPath);          // 存路径
    m_pRootItem->setData(0, Qt::UserRole+1, 0);                      // 存类型：0=目录
    ui->treeWidget->addTopLevelItem(m_pRootItem);                    // 挂到控件顶层
    loadDirIntoItem(m_pRootItem, m_strUserPath);                     // 首次加载根目录
}

File::~File()
{
    delete ui;
}

// 重新加载整棵目录树：清空后重建根节点并加载根目录
void File::flushFile()
{
    ui->treeWidget->clear();                                         // 清空整棵树

    m_pRootItem = new QTreeWidgetItem;                               // 重建根节点
    m_pRootItem->setText(0, m_strUserPath);
    m_pRootItem->setIcon(0, QIcon(QPixmap(":/dir.png")));
    m_pRootItem->setData(0, Qt::UserRole,   m_strUserPath);
    m_pRootItem->setData(0, Qt::UserRole+1, 0);
    ui->treeWidget->addTopLevelItem(m_pRootItem);

    loadDirIntoItem(m_pRootItem, m_strUserPath);                     // 重新加载根目录
}

// 请求某目录下的子项：记住父节点，把目录路径发给服务器
void File::loadDirIntoItem(QTreeWidgetItem* parentItem, const QString& dirPath)
{
    m_pCurParentItem = parentItem;                                   // 记住：回包后往谁下面挂子项
    QByteArray baPath = dirPath.toUtf8();                            // 用UTF-8，支持中文路径
    PDU* pdu = mkPDU(baPath.size()+1);                               // +1 预留结尾\0
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_REQUEST;
    memcpy(pdu->caMsg, baPath.constData(), baPath.size());
    Client::getInstance().sendMsg(pdu);
}


// 更新文件列表UI：将服务器返回的FileInfo列表渲染到QListWidget
// 用服务器返回的数据，填充"当前父节点"的子项
void File::updateFileList(QList<FileInfo*> pFileList)
{
    foreach(FileInfo* pFileInfo, m_pFileInfoList){                   // 清理旧的FileInfo对象
        delete pFileInfo;
    }
    m_pFileInfoList.clear();
    m_pFileInfoList = pFileList;

    qDeleteAll(m_pCurParentItem->takeChildren());                    // 清掉父节点旧子项（含占位节点）
    QString strParentPath = m_pCurParentItem->data(0, Qt::UserRole).toString(); // 父节点的完整路径

    for(int i = 0; i < pFileList.size(); i++){
        FileInfo* pInfo = pFileList[i];
        QString strName = QString::fromUtf8(pInfo->caName);          // caName是char[32]，转QString

        if(strName == "." || strName == "..") continue;              // 过滤掉服务器返回的 . 和 ..

        QTreeWidgetItem* pItem = new QTreeWidgetItem;
        pItem->setText(0, strName);                                  // 第0列显示名字

        if(pInfo->iFileType == 0){                                   // 目录
            pItem->setIcon(0, QIcon(QPixmap(":/dir.png")));
            pItem->addChild(new QTreeWidgetItem);                    // 占位子节点→冒出▶箭头
            pItem->setData(0, Qt::UserRole,   strParentPath + "/" + strName); // 存完整路径
            pItem->setData(0, Qt::UserRole+1, 0);                    // 存类型：目录
        } else {                                                     // 文件
            pItem->setIcon(0, QIcon(QPixmap(":/file.png")));
            pItem->setData(0, Qt::UserRole,   strParentPath + "/" + strName); // 文件也存路径(分享用)
            pItem->setData(0, Qt::UserRole+1, 1);                    // 存类型：文件
        }

        m_pCurParentItem->addChild(pItem);                           // 挂到当前父节点下
    }

    m_pCurParentItem->setExpanded(true);                             // 加载完自动展开
}

// 更新上传进度条（由Uploader子线程的信号触发，在主线程执行）
void File::updateProgress(int percent)
{
    qDebug() << "上传进度:" << percent;   // 临时调试用
    ui->uploadProgressBar->setRange(0,100);   // 确保范围是0~100
    ui->uploadProgressBar->setValue(percent); // 设置当前进度
}

// 上传结束后复位进度条
void File::resetUploadProgress()
{
    ui->uploadProgressBar->setValue(0);
}


// 在主线程中逐块读取本地文件并直接通过socket发送（旧上传方案，Uploader是新方案）
void File::uploadFile()
{
    QFile file(m_strUploadFilePath);                                 // 打开要上传的本地文件
    if(!file.open(QIODevice::ReadOnly)){
        QMessageBox::information(this,"上传文件","打开文件失败");
        return;
    }
    // 创建一个4096字节的PDU作为数据块缓冲区，循环读取文件并发送
    PDU* datapdu = mkPDU(4096);
    datapdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_REQUEST;
    while(true){
        qint64 ret = file.read(datapdu->caMsg,4096);                 // 每次读取最多4096字节
        if(ret < 0){
            QMessageBox::information(this,"上传文件","读取文件失败");
            return;
        }if(ret == 0){                                               // ret==0表示文件读完
            break;
        }
        datapdu->uiMsgLen = ret;                                     // 更新实际读到的数据长度
        datapdu->uiPDULen = ret+sizeof(PDU);                         // 更新PDU总长度
        Client::getInstance().m_socket.write((char*)datapdu,datapdu->uiPDULen); // 直接写入socket发送
    }
    free(datapdu);                                                   // 释放数据块PDU
    datapdu = NULL;
    file.close();                                                    // 关闭本地文件
}

// 新建文件夹按钮：弹出输入对话框获取文件夹名，发送创建请求
void File::on_mkdir_PB_clicked()
{
    QString strName =  QInputDialog::getText(this,"新建文件夹","文件夹名"); // 弹出输入框
    if(strName.isEmpty()||strName.toStdString().size()>32){
        QMessageBox::information(this,"新建文件夹","文件夹长度非法");
        return;
    }
    qDebug()<<"查找用户 strName" << strName;
    PDU* pdu = mkPDU(m_strCurPath.toStdString().size()+1);          // caMsg放当前路径
    pdu->uiMsgType = ENUM_MSG_TYPE_MKDIR_REQUEST;
    //memcpy(pdu->caData,strName.toStdString().c_str(),32);           // caData放新文件夹名
    strncpySafe(pdu->caData, 32,strName.toStdString().c_str());
    memcpy(pdu->caMsg,m_strCurPath.toStdString().c_str(),m_strCurPath.toStdString().size());
    Client::getInstance().sendMsg(pdu);
}

// 手动刷新文件列表按钮
void File::on_flushFile_PB_clicked()
{
    flushFile();
}

// 删除文件夹按钮：选中后确认删除（仅删除目录，不删除文件）
// 删除文件夹按钮：选中目录节点后确认删除
void File::on_deldir_PB_clicked()
{
    QTreeWidgetItem* pItem = ui->treeWidget->currentItem();          // 获取当前选中节点
    if(pItem == nullptr){
        QMessageBox::information(this,"删除文件夹","请选择要删除的文件夹");
        return;
    }
    if(pItem->data(0, Qt::UserRole+1).toInt() != 0){                 // 用存好的类型判断：非目录则拒绝
        QMessageBox::information(this,"删除文件夹","请选择文件夹（当前选中的是文件）");
        return;
    }
    if(pItem == m_pRootItem){                                        // 保护：不允许删根目录
        QMessageBox::information(this,"删除文件夹","不能删除根目录");
        return;
    }

    QString strDelName = pItem->text(0);                             // 目录名
    int ret = QMessageBox::question(this,"删除文件夹",
                                    QString("是否确认删除 %1").arg(strDelName));
    if(ret != QMessageBox::Yes) return;

    // 服务器 deldir 用 caMsg(父路径) + caData(目录名) 拼接完整路径
    QTreeWidgetItem* pParent = pItem->parent();
    QString strParentPath = pParent->data(0, Qt::UserRole).toString();
    QByteArray baParent = strParentPath.toUtf8();
    QByteArray baName   = strDelName.toUtf8();

    PDU* pdu = mkPDU(baParent.size()+1);
    pdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_REQUEST;
    strncpySafe(pdu->caData, 32, baName.constData());               // caData放目录名
    memcpy(pdu->caMsg, baParent.constData(), baParent.size());      // caMsg放父路径
    Client::getInstance().sendMsg(pdu);

    // 本地立即移除该节点（服务器删除成功后，点"刷新"可重新同步）
    int idx = pParent->indexOfChild(pItem);
    QTreeWidgetItem* removed = pParent->takeChild(idx);
    delete removed;
}


// 双击列表项：如果是目录则进入，如果是文件则忽略
/* void File::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString strDirName = item->text();

    foreach(FileInfo* pFileInfo, m_pFileInfoList){                   // 查找到该条目
        if(strDirName == pFileInfo->caName && pFileInfo->iFileType != 0){
            return;                                                  // 是文件不是目录，不进入
        }
    }
    m_strCurPath = QString("%1/%2").arg(m_strCurPath).arg(strDirName); // 拼接新路径
    flushFile();                                                     // 进入后刷新
} */


// 树模式下"返回上一级"改为：折叠当前选中的节点（收起它的子项）
void File::on_return_PB_clicked()
{
    QTreeWidgetItem* pItem = ui->treeWidget->currentItem();
    if(pItem == nullptr){
        QMessageBox::information(this,"返回","请先选中一个目录节点");
        return;
    }
    pItem->setExpanded(false);                                       // 折叠选中节点
}

// 上传文件按钮：打开文件选择对话框，发送上传初始化请求
/*  void File::on_uploadFile_PB_clicked()
{
    m_strUploadFilePath = QFileDialog::getOpenFileName();            // 弹出文件选择对话框（获取本地绝对路径）
    int index = m_strUploadFilePath.lastIndexOf('/');
    QString strFileName =  m_strUploadFilePath.right(m_strUploadFilePath.size() - index - 1); // 提取文件名
    QFile file(m_strUploadFilePath);
    qint64 iFileSize = file.size();                                  // 获取文件大小（字节数）
    // caData前32字节存文件名，caData后32字节存文件大小，caMsg存服务器目标路径
    PDU* pdu = mkPDU(m_strCurPath.toStdString().size()+1);
    pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_INIT_REQUEST;
    memcpy(pdu->caData,strFileName.toStdString().c_str(),32);
    memcpy(pdu->caData+32, &iFileSize,32);                           // 将文件大小写入caData后32字节
    memcpy(pdu->caMsg,m_strCurPath.toStdString().c_str(),m_strCurPath.toStdString().size());
    Client::getInstance().sendMsg(pdu);                              // 发送初始化请求，等待服务器确认后开始上传
}
  */
// 上传文件按钮：打开文件选择对话框，计算MD5后发送上传初始化请求
void File::on_uploadFile_PB_clicked()
{
    m_strUploadFilePath = QFileDialog::getOpenFileName();            // 弹出文件选择对话框
    if(m_strUploadFilePath.isEmpty()){                               // 用户取消选择
        return;
    }
    int index = m_strUploadFilePath.lastIndexOf('/');
    QString strFileName =  m_strUploadFilePath.right(m_strUploadFilePath.size() - index - 1);

    QFile file(m_strUploadFilePath);
    qint64 iFileSize = file.size();                                  // 获取文件大小（字节数）

    // 计算本地文件的MD5（用于完整性校验，后续秒传/断点续传也要用）
    QByteArray baMd5;
    QCryptographicHash hash(QCryptographicHash::Md5);
    if(file.open(QIODevice::ReadOnly)){
        hash.addData(&file);                                         // 读整个文件算摘要
        baMd5 = hash.result();                                       // 16字节二进制MD5
        file.close();
    }else{
        QMessageBox::information(this,"上传文件","打开文件失败");
        return;
    }

                              // 发送初始化请求
    /* // caData布局：前32字节文件名，32~40文件大小(qint64)，40~56 MD5(16字节)，caMsg放目标路径
    PDU* pdu = mkPDU(m_strCurPath.toStdString().size()+1);
    pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_INIT_REQUEST;
    memcpy(pdu->caData,strFileName.toStdString().c_str(),32);
    memcpy(pdu->caData+32, &iFileSize,sizeof(qint64));               // 只写8字节，修掉原来写32字节的脏写法
    memcpy(pdu->caData+40, baMd5.constData(),16);                    // 写入16字节MD5
    memcpy(pdu->caMsg,m_strCurPath.toStdString().c_str(),m_strCurPath.toStdString().size());
    Client::getInstance().sendMsg(pdu);     */
    // caMsg = 文件名 + '\0' + 目标路径（UTF-8），解决长中文名超32字节被截断的问题
    QByteArray baMsg = strFileName.toUtf8();
    baMsg.append('\0');
    baMsg.append(m_strCurPath.toUtf8());

    PDU* pdu = mkPDU(baMsg.size());
    pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_INIT_REQUEST;
    memcpy(pdu->caData, &iFileSize, sizeof(qint64));          // caData[0:8] 文件大小
    memcpy(pdu->caData+8, baMd5.constData(), 16);             // caData[8:24] MD5
    memcpy(pdu->caMsg, baMsg.constData(), baMsg.size());      // caMsg 文件名+\0+路径
    Client::getInstance().sendMsg(pdu);

}

// 分享文件按钮：选中文件后打开分享对话框
// 分享文件按钮：选中文件后打开分享对话框
void File::on_shareFile_PB_clicked()
{
    QTreeWidgetItem* pItem = ui->treeWidget->currentItem();          // 改：treeWidget
    if(pItem == NULL){
        QMessageBox::information(this,"分享文件","请选择要分享的文件");
        return;
    }
    m_strShareFileName = pItem->text(0);                             // 改：text(0)
    m_pShareFile->updateFirend_LW();                                 // 从好友列表刷新分享目标列表
    if(m_pShareFile->isHidden()){
       m_pShareFile->show();                                         // 显示分享对话框
    }
}

// 展开目录节点：懒加载它的子项
void File::onTreeItemExpanded(QTreeWidgetItem* item)
{
    QString strDirPath = item->data(0, Qt::UserRole).toString();     // 取出节点存的完整路径
    loadDirIntoItem(item, strDirPath);                               // 发请求加载子项
}

// 选中节点变化：更新 m_strCurPath（供上传/新建/分享使用）
void File::onTreeCurrentItemChanged(QTreeWidgetItem* cur, QTreeWidgetItem* prev)
{
    Q_UNUSED(prev);
    if(cur == nullptr) return;

    int iType = cur->data(0, Qt::UserRole+1).toInt();                // 节点存的类型
    QString strPath = cur->data(0, Qt::UserRole).toString();         // 节点存的路径

    if(iType == 0){                                                  // 目录：当前目录=该目录本身
        m_strCurPath = strPath;
    } else {                                                         // 文件：当前目录=它的父目录
        QTreeWidgetItem* pParent = cur->parent();
        m_strCurPath = pParent ? pParent->data(0, Qt::UserRole).toString() : m_strUserPath;
    }
}
