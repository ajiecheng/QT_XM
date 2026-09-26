#include "client.h"
#include "uploader.h"

#include <QFile>
#include <QThread>
// 文件上传器实现：在子线程中逐块读取本地文件并通过信号发送

Uploader::Uploader()
{

}


/* // 构造函数：保存要上传的本地文件路径
Uploader::Uploader(const QString strFilePath)
{
    m_strUploadFilePath = strFilePath;
}*/

//构造函数
Uploader::Uploader(const QString strFilePath, qint64 startOffset)
    : m_strUploadFilePath(strFilePath),
      m_iStartOffset(startOffset),
      m_ackReceived(false),
      m_aborted(false)
{
}


// 在子线程中执行的槽函数：循环读取文件，每16KB封装为一个PDU并通过信号发出
/*void Uploader::uploadFile()
{
    QFile file(m_strUploadFilePath);                         // 打开要上传的本地文件
    if(!file.open(QIODevice::ReadOnly)){
        emit errorMsg("上传文件失败：打开文件失败");           // 打开失败：通知主线程显示错误
        emit finished();                                      // 通知线程完成
        return;
    }
    // 循环以16KB为块大小读取文件,宏定义
    while(true){
        PDU* datapdu = mkPDU(UPLOAD_BLOCK_SIZE);                          // 创建16KB容量的数据块PDU
        datapdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_REQUEST;
        qint64 ret = file.read(datapdu->caMsg,UPLOAD_BLOCK_SIZE);         // 读取最多16KB到caMsg
        if(ret < 0){
            emit errorMsg("上传文件失败：读取文件失败");       // 读取失败
            emit finished();
            return;
        }if(ret == 0){                                       // ret==0表示文件已读完
            break;
        }
        datapdu->uiMsgLen = ret;                             // 更新实际数据长度
        datapdu->uiPDULen = ret+sizeof(PDU);                 // 更新PDU总长度
        emit sendPDU(datapdu);                               // 通过信号将数据块发送给Client::sendMsg
    }
    file.close();                                            // 关闭本地文件
    emit finished();                                         // 通知线程完成
}
 */
void Uploader::uploadFile()
{
    QFile file(m_strUploadFilePath);
    if(!file.open(QIODevice::ReadOnly)){
        emit errorMsg("上传文件失败：打开文件失败");
        emit finished();
        return;
    }

    if(m_iStartOffset > 0){
        file.seek(m_iStartOffset);        // 断点续传：从已传位置继续读
    }

    qint64 total = file.size();
    qint64 sent = m_iStartOffset;          // 已发字节从断点开始算（原来是0）

   // qint64 total = file.size();                              // 文件总大小
   //qint64 sent = 0;                                         // 已读并发送的字节数
    PDU* datapdu = NULL;
    while(true){
        datapdu = mkPDU(UPLOAD_BLOCK_SIZE);
        datapdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_DATA_REQUEST;
        qint64 ret = file.read(datapdu->caMsg,UPLOAD_BLOCK_SIZE);
        if(ret < 0){
            free(datapdu);
            datapdu = NULL;
            emit errorMsg("上传文件失败：读取文件失败");
            emit finished();
            return;
        }
        if(ret == 0){
            free(datapdu);
            datapdu = NULL;
            break;
        }
        datapdu->uiMsgLen = ret;
        datapdu->uiPDULen = ret+sizeof(PDU);
        emit sendPDU(datapdu);

        sent += ret;                                         // 累加已发送字节
        int percent = total > 0 ? int(sent * 100 / total) : 100; // 算百分比（total>0防除零）
        emit progress(percent);                              // 通知主线程更新进度条

        if(sent == total){                    // 这是最后一块：不等ACK，等DATA_RESPOND
            break;
        }
/*   // 停等：阻塞等服务器ACK，再发下一块（流控）
        {
            QMutexLocker locker(&m_ackMutex);
            m_ackReceived = false;
            while(!m_ackReceived){
                m_ackCond.wait(&m_ackMutex);  // 原子解锁+阻塞；被唤醒后重新加锁
            }
        }
*/
        // 停等：阻塞等服务器ACK，再发下一块（流控）
               {
                   QMutexLocker locker(&m_ackMutex);
                   m_ackReceived = false;
                   while(!m_ackReceived && !m_aborted){   // 连接断开时也能退出
                       m_ackCond.wait(&m_ackMutex);
                   }
                   if(m_aborted){                          // 被中止：退出上传
                       emit errorMsg("上传中断：连接已断开");
                       break;
                   }
               }

    }
    file.close();
    emit finished();
}

// 中止上传（连接断开时由主线程调用）：唤醒停等，让uploadFile检查m_aborted后退出
void Uploader::abort()
{
    QMutexLocker locker(&m_ackMutex);
    m_aborted = true;
    m_ackCond.wakeOne();                          // 唤醒正在停等的uploadFile
}

// 收到服务器ACK（主线程调用）：唤醒正在停等的uploadFile
void Uploader::onAck()
{
    QMutexLocker locker(&m_ackMutex);
    m_ackReceived = true;
    m_ackCond.wakeOne();                       // 唤醒子线程继续发下一块
}


// 启动上传：创建新线程，将Uploader移到线程中，连接信号后启动
void Uploader::start()
{
    QThread* thread = new QThread;                           // 创建工作线程
    this->moveToThread(thread);                              // 将Uploader对象移到新线程
    connect(thread,&QThread::started,this,&Uploader::uploadFile); // 线程启动时开始上传

    connect(this,&Uploader::finished,thread,&QThread::quit); // 上传完成后退出线程事件循环

    connect(thread,&QThread::finished,thread,&QThread::deleteLater); // 线程结束后自动删除

    thread->start();                                         // 启动线程
}
