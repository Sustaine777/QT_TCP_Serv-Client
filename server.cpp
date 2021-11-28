#include "server.h"
#include <QDebug>
#include <QTimer>
#include <QByteArray>
#include <QtEndian>

request_command req_com;
info_command inf_com;
ping_command ping_com;
control_command cont_com;
TCPJoyStruct LTCPJoy;
TCPJoyStruct RTCPJoy;
RTCPThread RTCPT;
LTCPThread LTCPT;

Client *c;
Client *b;


QUuid trueclientId = QUuid(0xefd339e0,0xa36d,0x4784,0x81,0xf2,0xb4,0xca,0xe6,0x39,0x58,0x93); ///Нужно отдельно для левой и правой руки

Server::Server(QObject *parent) : QTcpServer(parent)
{
    pTcpServer = new QTcpServer(this);
    pTcpSocket = nullptr;
    if (pTcpServer->listen(QHostAddress::LocalHost, 13303)) qDebug() << "listen";
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
    if ((ba[0] == char(0x10))&&(ba[1] == char(0x00))){
        ba.remove(2,2);
        if ((ba[2] == char(0x00))&&(ba[3] == char(0x00)))
        {
            ba.remove(0, 4);
            qDebug() << ba;
            QUuid clientId = QUuid(ba);
            if (clientId == trueclientId) correctId = true;
            else correctId = false;
        }
    }
    //Проверка на ping
    if ((ba[0] == char(0x00))&&(ba[1] == char(0x00))){
        ba.remove(2,2);
        if ((ba[2] == char(0x02))&&(ba[3] == char(0x00))) pingyes = true;
        else pingyes = false;
    }


}

void Server::timeping(){
    pTcpSocket->write((char*)&ping_com,sizeof(ping_com));
}

void Server::timecheck(){
    qDebug() << "im in server timecheck";
    if (pingyes) {qDebug() <<"allgood";
        pingyes = false;}
    else qDebug() <<"allbad";
}

void Server::joy(){
    pTcpSocket->write((char*)&cont_com,sizeof(cont_com));
}

Server::~Server()
{
    //pTcpSocket->close(); ///Нужно ли?
}

//Client class
bool LTCPTimeExpired = true;
bool RTCPTimeExpired = true;

Client::Client(int a)
{
    pTcpSocket = new QTcpSocket(this);
    qDebug() << "client up";
    ///if (pTcpSocket->bind(QHostAddress::LocalHost)) qDebug() <<"binded"; ///Нужно ли? На какой адрес/порт
    whichUUID = a;
    newConnection();
    connect(pTcpSocket, SIGNAL(readyRead()), SLOT(readyRead()));
}

void Client::newConnection(){
    qDebug() << "newConnection";
    //qDebug() <<  whichUUID;
    pTcpSocket->abort();
    //pTcpSocket->connectToHost(QHostAddress("192.168.8.21"), 8400);///Тут нужно на другой адрес  ///(192.168.1.110) //Какой порт - узнать?QHostAddress::LocalHost
    pTcpSocket->connectToHost(QHostAddress::LocalHost, 13303);
    connect(pTcpSocket, &QTcpSocket::connected, this, [&](){
       qDebug() <<"connected";
       //Таймер для отправки своего пинга для проверки
       QTimer *timer = new QTimer(this);
       connect(timer, SIGNAL(timeout()), this, SLOT(timeping()));
       timer->start(900);

       //Таймер для приема пинга от сервера
       QTimer *timer_1 = new QTimer (this);
       connect(timer_1, SIGNAL(timeout()), this, SLOT(timecheck()));
       timer_1->start(1100);
       });
}

void Client::readyRead(){
    if (whichUUID ) RTCPTimeExpired = false;
    else LTCPTimeExpired = false;

    QByteArray ba;
    ba = pTcpSocket->readAll();
    //Проверка на соединение
    if ((ba[0] == char(0x10))&&(ba[1] == char(0x00))){
        ba.remove(2,2);
        if ((ba[2] == char(0x01))&&(ba[3] == char(0x00)))
        {
            QUuid clientId;
            if (whichUUID == 1)//ВМ - Правый
            clientId = QUuid(0xefd339e0,0xa36d,0x4784,0x81,0xf2,0xb4,0xca,0xe6,0x39,0x58,0x93);
            else clientId = QUuid(0xace519c0,0xa56f,0x1588,0x82,0xd2,0xb3,0xcd,0xe7,0x38,0x58,0x92);
            QByteArray bclientId = clientId.toByteArray();
            //qDebug() << bclientId;
            QByteArray sendba;
            sendba.insert(0,(char*)&inf_com,sizeof(inf_com));
            sendba = sendba + bclientId;
            sendba = qToLittleEndian(sendba); ///Сработает ли?
            pTcpSocket->write(sendba);
        }
    }

    //Проверка на ping
    if ((ba[0] == char(0x00))&&(ba[1] == char(0x00))){
        ba.remove(2,2);
        if ((ba[2] == char(0x02))&&(ba[3] == char(0x00))) pingyes = true;
        else pingyes = false;
    }

    //Данные с джойстика
    if ((ba[0] == char(0x0e))&&(ba[1] == char(0x00))){
        ba.remove(2,2);
        if ((ba[2] == char(0x3b))&&(ba[3] == char(0x01)))
        {
            if (whichUUID == 1){
                byteSort(RTCPJoy, ba);
                //qDebug() << RTCPJoy.X;
                rJoyPack();
            }
            else {
                byteSort(LTCPJoy, ba);
                lJoyPack();
            }
        }
    }
}

void Client::byteSort(TCPJoyStruct &Joy, QByteArray &ba){
    ba.remove(0, 4);
    for (int i=0; i<6; i++){
        switch (i){
        case 0:
            Joy.X = ((unsigned short)ba[1] << 8) | (unsigned char)ba[0];
            break;
        case 1:
            Joy.Y = ((unsigned short)ba[1] << 8) | (unsigned char)ba[0];
            break;
        case 2:
            Joy.Z = ((unsigned short)ba[1] << 8) | (unsigned char)ba[0];
            break;
        case 3:
            //yStickOffset
            break;
        case 4:
            //registers
            Joy.buttons = (unsigned char)ba[0] + ((unsigned char)ba[1] << 8);
            Joy.buttonsF = (unsigned char)ba[2] + ((unsigned char)ba[3] << 8);
            ///Байт читается справа налево? Нужно задать вопрос
            break;
        case 5:
            break;
        }
        ba.remove(0, 2);
    }
     ba.remove(0, 2);
}

void Client::rJoyPack(){

    /*
    snd.RJoy_X = RTCPJoy.X;
    snd.RJoy_Y = RTCPJoy.Y;
    snd.RJoy_Z = RTCPJoy.Z;

    //qDebug() << snd.RJoy_X;
    ///Возможно ли несколько вариантов одновременно? - узнать
    //ButtonA3
    if ((RTCPJoy.buttons & (1 << (1))) != 0)
        snd.RCircs = CTRL_COND_OFF;
    if ((RTCPJoy.buttons & (1 << (2))) != 0)
        snd.RCircs = CTRL_COND_MIDDLE_SP;
    if ((RTCPJoy.buttons & (1 << (3))) != 0)
        snd.RCircs = CTRL_COND_MIDDLE_POD;
    if ((RTCPJoy.buttons & (1 << (4))) != 0)
        snd.RCircs = CTRL_COND_LOW_SP;
    if ((RTCPJoy.buttons & (1 << (5))) != 0)
        snd.RCircs = CTRL_COND_HIGH_POD;

    //F1 курок
    if (((RTCPJoy.buttonsF & (1 << (0))) != 0))
        snd.RJoy_Func = 1; //проверить 1 - нажата или нет
    else
        snd.RJoy_Func = 0;

    //C1 center
    if ((RTCPJoy.buttons & (1 << (13))) != 0)
    {
        setup.Stop_On = true;
        emit stopButtonPressed();
    }
    else
    {
        setup.Stop_On = false;
        emit stopButtonReleased();
    }

    //c1
    if ((RTCPJoy.buttons & (1 << (14))) != 0)
    snd.RSxvatInt = setup.RSxvatIntension;

    //c1
    if ((RTCPJoy.buttons & (1 << (15))) != 0)
    snd.RSxvatInt = (-1)*setup.RSxvatIntension;

    //Смена режима работы - реализовать
    if ((RTCPJoy.buttons & (1 << (0))) != 0){
        ///?
    }
    else{

    }
    */
}

void Client::lJoyPack(){
/*
    snd.LJoy_X = LTCPJoy.X;
    snd.LJoy_Y = LTCPJoy.Y;
    snd.LJoy_Z = LTCPJoy.Z;

    ///Возможно ли несколько вариантов одновременно? - узнать
    if ((LTCPJoy.buttons & (1 << (1))) != 0)
        snd.LCircs = CTRL_COND_OFF;
    if ((LTCPJoy.buttons & (1 << (2))) != 0)
        snd.LCircs = CTRL_COND_MIDDLE_SP;
    if ((LTCPJoy.buttons & (1 << (3))) != 0)
        snd.LCircs = CTRL_COND_MIDDLE_POD;
    if ((LTCPJoy.buttons & (1 << (4))) != 0)
        snd.LCircs = CTRL_COND_LOW_SP;
    if ((LTCPJoy.buttons & (1 << (5))) != 0)
        snd.LCircs = CTRL_COND_HIGH_POD;

    if (((LTCPJoy.buttonsF & (1 << (0))) != 0))
        snd.LJoy_Func = 1; //проверить 1 - нажата или нет
    else
        snd.LJoy_Func = 0;

    if ((LTCPJoy.buttons & (1 << (13))) != 0)
    {
        setup.Stop_On = true;
        emit stopButtonPressed();
    }
    else
    {
        setup.Stop_On = false;
        emit stopButtonReleased();
    }

    if ((LTCPJoy.buttons & (1 << (14))) != 0)
    snd.LSxvatInt = setup.LSxvatIntension;

    if ((LTCPJoy.buttons & (1 << (15))) != 0)
    snd.LSxvatInt = (-1)*setup.LSxvatIntension;

    //Смена режима работы - реализовать
    if ((LTCPJoy.buttons & (1 << (0))) != 0){
        ///?
    }
    else{

    }

    */
}

void Client::timeping(){
    ping_com = qToLittleEndian(ping_com); ///Сработает ли?
    pTcpSocket->write((char*)&ping_com,sizeof(ping_com));
}

void Client::timecheck(){
    qDebug() << "im in client timecheck";
    if (pingyes) {qDebug() <<"allgood";
        pingyes = false;
        if (whichUUID ) RTCPTimeExpired = false;
        else LTCPTimeExpired = false;
    }
    else {qDebug() <<"allbad";
        if (whichUUID ) RTCPTimeExpired = false;
        else LTCPTimeExpired = false;
    }
}

Client::~Client()
{
    pTcpSocket ->close();
}

//TCP threads
RTCPThread::RTCPThread() : QThread( 0 )
{
    moveToThread(this);
}

void RTCPThread::run()
{
    //qDebug() << "Старт тестового потока";
    makeNewClient();//Возможно можно оставить вызов только через таймер
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(waitTimeExpired()));
    timer->start(5000);
    exec();
}

void RTCPThread::makeNewClient()
{
    c = new Client(1);
}

void RTCPThread::waitTimeExpired(){
    if (RTCPTimeExpired){
    std::destroy_at(c);
    makeNewClient();
    }
}

void RTCPThread::stop()
{
    std::destroy_at(c);
}

LTCPThread::LTCPThread() : QThread( 0 )
{
    moveToThread(this);
}

void LTCPThread::run()
{
    //qDebug() << "Старт тестового потока";
    makeNewClient();//Возможно можно оставить вызов только через таймер
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(waitTimeExpired()));
    timer->start(5000);
    exec();
}

void LTCPThread::makeNewClient()
{
    b = new Client(0);
}

void LTCPThread::waitTimeExpired(){
    if (LTCPTimeExpired){
    std::destroy_at(b);
    makeNewClient();
    }
}

void LTCPThread::stop()
{
    std::destroy_at(b);
}


