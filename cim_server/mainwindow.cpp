#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 新建一个TCP server
    server = new QTcpServer();
    connect(server, &QTcpServer::newConnection, this, &MainWindow::server_New_Connect);
}

MainWindow::~MainWindow()
{
    server->close();
    server->deleteLater();
    delete ui;
}

void MainWindow::server_New_Connect()
{
    QString clientIP;
    quint16 clientPort;
    QString clientInfo;

    socket = server->nextPendingConnection();

    QObject::connect(socket, &QTcpSocket::readyRead, this, &MainWindow::socket_Read_Data);

    clientIP = (socket->peerAddress()).toString();
    clientPort = socket->peerPort();

    clientInfo = clientIP + ":" + QString::number(clientPort);
    ui->lineEdit->setText(clientInfo);
}

bool got_0x0B_Flag = false;
quint32 gotFrameCnt;
quint32 frameTotal;
quint32 dataTotalLen;
quint32 totalBytes;

void MainWindow::socket_Read_Data()
{
    QByteArray buffer;

    buffer = socket->readAll();
    if(!buffer.isEmpty())
    {
        /// QString str = ui->textEdit_Recv->toPlainText();
        /// str+=tr(buffer);
        /// ui->textEdit_Recv->setText(str);

        if((buffer[0] == 0xEB) && (buffer[1] == 0x90))
        {
            if(buffer[3] == 0x09)
            {
                QByteArray sendBuffer;
                quint32 i, sum = 0, len = 29;

                sendBuffer.resize(len);
                sendBuffer.fill(0, 29);
                sendBuffer[0] = 0xEB;
                sendBuffer[1] = 0x90;
                sendBuffer[2] = 0x08;
                sendBuffer[3] = 0x09;
                sendBuffer[4] = len >> 24;
                sendBuffer[5] = len >> 16;
                sendBuffer[6] = len >> 8;
                sendBuffer[7] = len;
                sendBuffer[26] = 0;
                for(i = 2; i < 29 - 2; i++)
                {
                    sum += sendBuffer[i];
                }
                sendBuffer[27] = sum >> 8;
                sendBuffer[28] = sum;

                ui->textBrowser->append("got <9>");
                socket->write(sendBuffer);
                ui->textBrowser->append("sent <9>");
            }
            else if(buffer[3] == 0x05)
            {
                QByteArray sendBuffer;
                quint32 i, sum = 0, len = 29;

                sendBuffer.resize(len);
                sendBuffer.fill(0, 29);
                sendBuffer[0] = 0xEB;
                sendBuffer[1] = 0x90;
                sendBuffer[2] = 0x08;
                sendBuffer[3] = 0x05;
                sendBuffer[4] = len >> 24;
                sendBuffer[5] = len >> 16;
                sendBuffer[6] = len >> 8;
                sendBuffer[7] = len;
                sendBuffer[26] = 0;
                for(i = 2; i < 29 - 2; i++)
                {
                    sum += sendBuffer[i];
                }
                sendBuffer[27] = sum >> 8;
                sendBuffer[28] = sum;

                ui->textBrowser->append("got <5>");
                socket->write(sendBuffer);
                ui->textBrowser->append("sent <5>");
            }
            else if(buffer[3] == 0x0B)
            {
                got_0x0B_Flag = true;
                dataTotalLen = buffer[4];
                dataTotalLen <<= 8;
                dataTotalLen |= buffer[3];
                dataTotalLen <<= 8;
                dataTotalLen |= buffer[2];
                dataTotalLen <<= 8;
                dataTotalLen |= buffer[1];

                frameTotal = (dataTotalLen - 32) / 850 + 1;
                ui->textBrowser->append("got <B> head");
            }
            else
            {
                ui->statusBar->showMessage("recv buffer[3] err!", 4000);
            }
        }
        else
        {
            if(got_0x0B_Flag == true)
            {
                totalBytes += buffer.count();
                if(++gotFrameCnt >= frameTotal)
                {
                    QByteArray sendBuffer;
                    quint32 i, sum = 0, len = 29;

                    sendBuffer.resize(len);
                    sendBuffer.fill(0, 29);
                    sendBuffer[0] = 0xEB;
                    sendBuffer[1] = 0x90;
                    sendBuffer[2] = 0x08;
                    sendBuffer[3] = 0x0B;
                    sendBuffer[4] = len >> 24;
                    sendBuffer[5] = len >> 16;
                    sendBuffer[6] = len >> 8;
                    sendBuffer[7] = len;
                    sendBuffer[26] = 0;
                    for(i = 2; i < 29 - 2; i++)
                    {
                        sum += sendBuffer[i];
                    }
                    sendBuffer[27] = sum >> 8;
                    sendBuffer[28] = sum;

                    socket->write(sendBuffer);
                    ui->textBrowser->append("sent <B>");
                }
                QString str = "frame seq:";
                str += QString::number(gotFrameCnt);
                str += ",len=";
                str += QString::number(buffer.count());
                str += ",totbytes=";
                str += QString::number(totalBytes);
                ui->textBrowser->append(str);
            }
            else
            {
                ui->statusBar->showMessage("recv err!", 4000);
            }
        }
    }
}

void MainWindow::on_pushButton_clicked()
{
    server->listen(QHostAddress::Any, 6677);
    ui->pushButton->setText("working...");
}

void MainWindow::on_pushButton_2_clicked()
{
    // 关闭socket并重新开始监听
    server->close();
    server->deleteLater();
    server = new QTcpServer();
    connect(server, &QTcpServer::newConnection, this, &MainWindow::server_New_Connect);

    // 重置各个控件
    ui->lineEdit->clear();
    ui->textBrowser->clear();
    ui->pushButton->setText("start");

    // 重置各个全局变量
    got_0x0B_Flag = false;
    gotFrameCnt = 0;
    frameTotal = 0;
    dataTotalLen = 0;
    totalBytes = 0;
}