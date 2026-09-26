#ifndef UPLOADER_H
#define UPLOADER_H
// 文件上传器头文件：在独立线程中循环读取本地文件并通过信号发送数据块
#include "protocol.h"

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
class Uploader : public QObject
{
    Q_OBJECT
public:
    Uploader();
    //Uploader(const QString strFilePath);   // 构造时指定要上传的本地文件路径
    Uploader(const QString strFilePath, qint64 startOffset = 0);   // 加偏移参数
    QString m_strUploadFilePath;           // 要上传的本地文件完整路径
    qint64 m_iStartOffset;            // 断点续传的起始偏移（默认0=从头传）

    void start();                          // 创建线程并将上传任务移到工作线程

    QMutex m_ackMutex;           // 保护ACK状态
    QWaitCondition m_ackCond;    // 停等条件变量
    bool m_ackReceived;          // 是否已收到ACK
    bool m_aborted;              // 是否被中止（连接断开时置true）

public slots:
    void abort();                // 中止上传：唤醒停等并退出
    void onAck();                // 收到服务器ACK，唤醒停等
    void uploadFile();                     // 在线程中循环读取文件并emit数据块

signals:
    void errorMsg(QString msg);            // 上传出错时发出的信号
    void sendPDU(PDU* pdu);               // 每读取一块数据就发出该信号（由Client::sendMsg接收）
    void finished();                       // 上传完成信号（通知线程退出）
    void progress(int percent);          // 上传进度（0~100百分比），供主线程更新进度条

};

#endif // UPLOADER_H
