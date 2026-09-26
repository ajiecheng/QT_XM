#include"protocol.h"
#include"stdlib.h"
#include<QDebug>
// 消息协议实现：提供PDU内存分配函数

// 根据消息体长度在堆上申请PDU空间，零初始化后设置长度字段并返回
PDU* mkPDU(unsigned int uiMsgLen)
{
    unsigned int uiPDULen= sizeof(PDU) + uiMsgLen;  // 总长度 = PDU结构体固定长度 + 可变消息体长度
    PDU* pdu = (PDU*)malloc(uiPDULen);               // 在堆上分配PDU内存（包含柔性数组空间）
    if(pdu==NULL){                                    // 内存分配失败检查
        qDebug()<<"申请PDU内存失败";
        exit(1);                                      // 分配失败直接终止，避免后续空指针操作
    }
    memset(pdu,0,uiPDULen);                           // 将申请的内存全部清零
    pdu->uiMsgLen = uiMsgLen;                         // 设置实际消息体长度
    pdu->uiPDULen = uiPDULen;                         // 设置PDU总长度
    return pdu;                                       // 返回构建好的PDU指针
}
