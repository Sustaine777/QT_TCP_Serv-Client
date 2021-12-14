#include "server.h"
#include <QDebug>
#include <QTimer>
#include <QByteArray>
#include <QtEndian>

registration_messages reg_mes;
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

QUuid rightManId = QUuid(0xefd339e0,0xa36d,0x4784,0x81,0xf2,0xb4,0xca,0xe6,0x39,0x58,0x93);
QUuid leftManId = QUuid(0xace519c0,0xa56f,0x1588,0x82,0xd2,0xb3,0xcd,0xe7,0x38,0x58,0x92);

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

    pingReg = false;

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeping()));
    timer->start(5000);


    QTimer *timer_1 = new QTimer (this);
    connect(timer_1, SIGNAL(timeout()), this, SLOT(timecheck()));
    timer_1->start(2000);


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
    if ((ba[0] == char(0x10))&&(ba[1] == char(0x00))&&(ba[2] == char(0x00))&&(ba[3] == char(0x00))){
        if ((ba[4] == char(0x00))&&(ba[5] == char(0x00)))
        {
            pingReg = true;
            noID = false;
            checkRegInfo(ba);
            sendAnswer();
        }
    }

}

void Server::checkRegInfo(QByteArray ba){
    ba.remove(0, 6);
    qDebug() << ba;
    QUuid clientId = QUuid(ba);
    if (clientId == rightManId) {correctId = true; qDebug() << "server::IDchecked";}
    else {correctId = false; qDebug() << "server::IDwrong";}
}

void Server::sendAnswer(){
    if (correctId) reg_mes.packetType = 0x0003;
    if (!correctId) reg_mes.packetType = 0x0005;
    if (noID) reg_mes.packetType = 0x0004;
    pTcpSocket->write((char*)&reg_mes,sizeof(reg_mes));
}


void Server::timeping(){
    //pTcpSocket->write((char*)&ping_com,sizeof(ping_com));
}

void Server::timecheck(){
    if (pingReg) {noID = false;}
    else  {noID = true;}
}

void Server::joy(){
    //\pTcpSocket->write((char*)&cont_com,sizeof(cont_com));
}

Server::~Server()
{

}

//Client class
bool LTCPTimeExpired = true;
bool RTCPTimeExpired = true;

Client::Client(int a)
{
    qDebug() << "New client";
    pTcpSocket = new QTcpSocket(this);
    whichUUID = a;
    newConnection();
    connect(pTcpSocket, SIGNAL(readyRead()), SLOT(readyRead()));
}

void Client::newConnection(){
    qDebug() << "New connection";
    pTcpSocket->abort();
    //pTcpSocket->connectToHost(QHostAddress("192.168.8.21"), 8400);
    pTcpSocket->connectToHost(QHostAddress::LocalHost, 13303);
    connect(pTcpSocket, &QTcpSocket::connected, this, &Client::sendInfo);
    connect(pTcpSocket, &QTcpSocket::connected, this, [&](){

        /*
       //Таймер для отправки своего пинга для проверки
       QTimer *timer = new QTimer(this);
       connect(timer, SIGNAL(timeout()), this, SLOT(timeping()));
       timer->start(900);
       */

       //Таймер для проверки наличия данных от сервера
       QTimer *timer_1 = new QTimer (this);
       connect(timer_1, SIGNAL(timeout()), this, SLOT(timecheck()));
       timer_1->start(5000);
       });
}

void Client::sendInfo(){
    pingyes = true;
    QUuid clientId;
    if (whichUUID == 1)//ВМ - Правый
    clientId = rightManId;
    else clientId = leftManId;
    QByteArray bclientId = clientId.toByteArray();
    QByteArray sendba;
    sendba.insert(0,(char*)&inf_com,sizeof(inf_com));
    sendba = sendba + bclientId;
    //sendba = qToLittleEndian(sendba);
    pTcpSocket->write(sendba);
}

void Client::readyRead(){
    pingyes = true;
    //if (whichUUID) RTCPTimeExpired = false;
    //else LTCPTimeExpired = false;
    QByteArray ba;
    ba = pTcpSocket->readAll();

    /*
    //Проверка на соединение
    if ((ba[0] == char(0x10))&&(ba[1] == char(0x00))&&(ba[2] == char(0x00))&&(ba[3] == char(0x00))){
        if ((ba[4] == char(0x01))&&(ba[5] == char(0x00)))
        {

        }
    }
    */

    //Проверка на ping
    if ((ba[0] == char(0x00))&&(ba[1] == char(0x00))&&(ba[2] == char(0x00))&&(ba[3] == char(0x00))){
        if ((ba[4] == char(0x02))&&(ba[5] == char(0x00))) {pingyes = true; qDebug() << "PING RECEIVED";}
    }

    //Ответ на запрос о регистрации
    if ((ba[0] == char(0x00))&&(ba[1] == char(0x00))&&(ba[2] == char(0x00))&&(ba[3] == char(0x00))){
        if ((ba[4] == char(0x03))&&(ba[5] == char(0x00))) {pingyes = true; qDebug() << "REG.SUCCESS";}
        else if ((ba[4] == char(0x04))&&(ba[5] == char(0x00))) {pingyes = false; errorCode = 4; qDebug() << "REG.ERROR:NO ID";}
        else if ((ba[4] == char(0x05))&&(ba[5] == char(0x00))) {pingyes = false; errorCode = 5; qDebug() << "REG.ERROR:WRONG ID";}
        else if ((ba[4] == char(0x06))&&(ba[5] == char(0x00))) {pingyes = false; errorCode = 6; qDebug() << "REG.ERROR:TOO MUCH CONNECTIONS";}
        else if ((ba[4] == char(0x07))&&(ba[5] == char(0x00))) {pingyes = false; errorCode = 7; qDebug() << "REG.ERROR:WAIT TIME EXPIRED";}
    }

    //Данные с джойстика
    if ((ba[0] == char(0x0c))&&(ba[1] == char(0x00))&&(ba[2] == char(0x00))&&(ba[3] == char(0x00))){
        if ((ba[4] == char(0x5f))&&(ba[5] == char(0x00)))
        {
            qDebug() << "JOYDATA RECEIVED";
            if (whichUUID == 1){
                byteSort(RTCPJoy, ba);
                rJoyPack();
            }
            else {
                byteSort(LTCPJoy, ba);
                lJoyPack();
            }
        }
    }

}

void Client::byteSort(TCPJoyStruct &Joy, QByteArray ba){
    ba.remove(0, 6);
    for (int i=0; i<6; i++){
        switch (i){
        case 0:
            Joy.X = ((unsigned short)ba[1] << 8) | ba[0];
            //qDebug() << "Joy.X";
            //qDebug() << Joy.X;
            break;
        case 1:
            Joy.Y = ((unsigned short)ba[1] << 8) | ba[0];
            //qDebug() << "Joy.Y";
            //qDebug() << Joy.Y;
            break;
        case 2:
            Joy.Z = ((unsigned short)ba[1] << 8) | ba[0];
            //qDebug() << "Joy.Z";
            //qDebug() << Joy.Z;
            break;
        case 3:
            //yStickOffset
            break;
        case 4:
            //registers
            Joy.buttons = (unsigned char)ba[0] + ((unsigned char)ba[1] << 8);
            Joy.buttonsF = (unsigned char)ba[2] + ((unsigned char)ba[3] << 8);
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
    /*
    ping_com = qToLittleEndian(ping_com); ///Сработает ли?
    pTcpSocket->write((char*)&ping_com,sizeof(ping_com));
    */
}

void Client::timecheck(){
    if (pingyes) {
        pingyes = false;
        if (whichUUID ) RTCPTimeExpired = false;
        else LTCPTimeExpired = false;
    }
    else {
        if (whichUUID ) RTCPTimeExpired = true;
        else LTCPTimeExpired = true;
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
    timer->start(7000);
    exec();
}

void RTCPThread::makeNewClient()
{
    c = new Client(1);
}

void RTCPThread::waitTimeExpired(){
    if (RTCPTimeExpired){
    delete c;
    qDebug() << "RIGHT CLIENT KILLED";
    makeNewClient();
    }
}

void RTCPThread::stop()
{
    delete c;
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
    timer->start(7000);
    exec();
}

void LTCPThread::makeNewClient()
{
    b = new Client(0);
}

void LTCPThread::waitTimeExpired(){
    if (LTCPTimeExpired){
    delete b;
    qDebug() << "LEFT CLIENT KILLED";
    makeNewClient();
    }
}

void LTCPThread::stop()
{
    delete b;
}


