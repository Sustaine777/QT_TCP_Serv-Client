#include "server.h"
#include <QDebug>
#include <QTimer>
#include <QByteArray>
#include <QtEndian>

request_command req_com;
info_command inf_com;
ping_command ping_com;
control_command cont_com;
JoyStruct JoyData;

QUuid trueclientId = QUuid(0xefd339e0,0xa36d,0x4784,0x81,0xf2,0xb4,0xca,0xe6,0x39,0x58,0x93); ///Нужно отдельно для левой и правой руки

Server::Server(QObject *parent) : QTcpServer(parent)
{
    pTcpServer = new QTcpServer(this);
    pTcpSocket = nullptr;
    if (pTcpServer->listen(QHostAddress("192.168.145.1"), 13303)) qDebug() << "listen";
    else qDebug() << "error listen";
    connect(pTcpServer, SIGNAL(newConnection()), this, SLOT(processingRequest()));
}

void Server::processingRequest()
{
    qDebug() << "connected";
    qDebug() << "request in process";
    pTcpSocket = pTcpServer->nextPendingConnection();
    connect(pTcpSocket, SIGNAL(readyRead()), SLOT(readyRead()));
    connect(pTcpSocket, &QTcpSocket::disconnected, pTcpSocket, &QTcpSocket::deleteLater);
    pTcpSocket->write((char*)&req_com,sizeof(req_com));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeping()));
    timer->start(1000);

    QTimer *timer_1 = new QTimer (this);
    connect(timer_1, SIGNAL(timeout()), this, SLOT(timecheck()));
    timer_1->start(1000);

    QTimer *timer_2 = new QTimer (this);
    connect(timer_2, SIGNAL(timeout()), this, SLOT(joy()));
    timer_2->start(500);
}

void Server::readyRead()
{
    QByteArray ba;
    ba = pTcpSocket->readAll();
    //qDebug() << ba;
    //Проверка на UUID.
    ///Можно ли так делать (перегонять uuid в ByteArray и обратно)?
    if ((ba[0] == char(0x10))&&(ba[1] == char(0x00)))
        if ((ba[2] == char(0x00))&&(ba[3] == char(0x00)))
        {
            ba.remove(0, 4);
            qDebug() << ba;
            QUuid clientId = QUuid(ba);
            if (clientId == trueclientId) correctId = true;
            else correctId = false;
        }

    //Проверка на ping
    if ((ba[0] == char(0x00))&&(ba[1] == char(0x00))){
        if ((ba[2] == char(0x02))&&(ba[3] == char(0x00))) pingyes = true;
        else pingyes = false;
    }

}

void Server::timeping(){
    pTcpSocket->write((char*)&ping_com,sizeof(ping_com));
}

void Server::timecheck(){
    qDebug() << "im in server timecheck";
    if (pingyes) qDebug() <<"allgood";
    else qDebug() <<"allbad";
}

void Server::joy(){
    pTcpSocket->write((char*)&cont_com,sizeof(cont_com));
}

Server::~Server()
{
    //pTcpSocket->close(); ///Нужно ли?
}


Client::Client(QObject *parent) : QObject(parent)
{
    pTcpSocket = new QTcpSocket(this);
    ///if (pTcpSocket->bind(QHostAddress::LocalHost)) qDebug() <<"binded"; ///Нужно ли? На какой адрес/порт
    pTcpSocket->connectToHost("192.168.145.1", 13303);///Тут нужно на другой адрес  ///(192.168.1.110) //Какой порт - узнать?QHostAddress::LocalHost
    connect(pTcpSocket, &QTcpSocket::connected, this, [&](){
       pTcpSocket->write("hello from client");

       //Таймер для отправки своего пинга для проверки
       QTimer *timer = new QTimer(this);
       connect(timer, SIGNAL(timeout()), this, SLOT(timeping()));
       timer->start(900);

       //Таймер для приема пинга от сервера
       QTimer *timer_1 = new QTimer (this);
       connect(timer_1, SIGNAL(timeout()), this, SLOT(timecheck()));
       timer_1->start(1100);
    });
    connect(pTcpSocket, SIGNAL(readyRead()), SLOT(readyRead()));
}

void Client::timeping(){
    ping_com = qToLittleEndian(ping_com); ///Сработает ли?
    pTcpSocket->write((char*)&ping_com,sizeof(ping_com));
}

void Client::readyRead(){
    QByteArray ba;
    ba = pTcpSocket->readAll();
    //Проверка на соединение
    if ((ba[0] == char(0x10))&&(ba[1] == char(0x00)))
        if ((ba[2] == char(0x01))&&(ba[3] == char(0x00)))
        {
            QUuid clientId = QUuid(0xefd339e0,0xa36d,0x4784,0x81,0xf2,0xb4,0xca,0xe6,0x39,0x58,0x93);
            QByteArray bclientId = clientId.toByteArray();
            qDebug() << bclientId;
            QByteArray sendba;
            sendba.insert(0,(char*)&inf_com,sizeof(inf_com));
            sendba = sendba + bclientId;
            sendba = qToLittleEndian(sendba); ///Сработает ли?
            pTcpSocket->write(sendba);
        }

    //Проверка на ping
    if ((ba[0] == char(0x00))&&(ba[1] == char(0x00))){
        if ((ba[2] == char(0x02))&&(ba[3] == char(0x00))) pingyes = true;
        else pingyes = false;
    }

    //Данные с джойстика
    if ((ba[0] == char(0x0e))&&(ba[1] == char(0x00)))
        if ((ba[2] == char(0x5f))&&(ba[3] == char(0x00)))
        {
            //qDebug() << ba[0];
            //qDebug() << ba[1];
            //qDebug() << ba[2];
            //qDebug() << ba[3];
            //Разбор данных джойстика
            ba.remove(0, 4);
            for (int i=0; i<6; i++){
                switch (i){
                case 0:
                    JoyData.X = ((unsigned short)ba[1] << 8) | (unsigned char)ba[0];
                    break;
                case 1:
                    JoyData.Y = ((unsigned short)ba[1] << 8) | (unsigned char)ba[0];
                    break;
                case 2:
                    JoyData.Z = ((unsigned short)ba[1] << 8) | (unsigned char)ba[0];
                    break;
                case 3:
                    //yStickOffset
                    break;
                case 4:
                    //LEDS
                    break;
                case 5:
                    //registers
                    qDebug() << ba;
                    JoyData.buttons = (unsigned char)ba[0] + ((unsigned char)ba[1] << 8) + ((unsigned char)ba[2] << 16) + ((unsigned char)ba[3] << 24);
                    qDebug() <<JoyData.buttons;
                    ///В основной программе нужно реализовать считывание кнопок
                    ///решение ниже работает, теперь адаптировать под нужные биты.
                    ///bool bit = (JoyData.buttons & (1 << (bitNumber-1))) != 0;
                    ///Байт читается справа налево? Нужно задать вопрос
                    //bool bit = (JoyData.buttons & (1 << (regNumber))) != 0; //Или 31-regNumber
                    //qDebug() << bit;
                    break;
                }
                ba.remove(0, 2);
            }
        }

}

void Client::timecheck(){
    qDebug() << "im in client timecheck";
    if (pingyes) qDebug() <<"allgood";
    else qDebug() <<"allbad";
}

Client::~Client()
{
    pTcpSocket ->close();
}

