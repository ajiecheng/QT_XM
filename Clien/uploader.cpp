#include "client.h"
#include "uploader.h"

#include <QFile>
#include <QThread>

Uploader::Uploader()
{

}

Uploader::Uploader(const QString strFilePath, qint64 startOffset)
    : m_strUploadFilePath(strFilePath),
      m_iStartOffset(startOffset),
      m_ackReceived(false),
      m_aborted(false)
{
}

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
        int percent = total > 0 ? int(sent * 100 / total) : 100; 
        emit progress(percent);                              // 通知主线程更新进度条

        if(sent == total){                    // 这是最后一块：不等ACK，等DATA_RESPOND
            break;
        }
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
