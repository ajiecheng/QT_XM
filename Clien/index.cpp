#include "index.h"
#include "ui_index.h"
// 主界面实现：管理好友和文件两个子页面的切换

// 构造函数：初始化UI
Index::Index(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Index)
{
    ui->setupUi(this);
}

// 单例实现：返回全局唯一的主界面实例
Index &Index::getInstance()
{
    static Index instance;
    return instance;
}

Index::~Index()
{
    delete ui;
}

// 获取嵌套在stackedWidget中的好友页面控件
Friend *Index::getFriend()
{
    return ui->friendPage;
}

// 获取嵌套在stackedWidget中的文件页面控件
File *Index::getFile()
{
    return ui->filePage;
}

// 点击"好友"按钮：切换到stackedWidget的第0页（好友页面）
void Index::on_firend_PB_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

// 点击"文件"按钮：切换到stackedWidget的第1页（文件页面）
void Index::on_file_PB_clicked()
{
     ui->stackedWidget->setCurrentIndex(1);
}
