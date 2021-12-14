#ifndef SERVER_H
#define SERVER_H
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QUuid>
#include <QByteArray>
#include <QString>


class Server : public QTcpServer
{
    Q_OBJECT

public:
    Server(QObject *parent = nullptr);
    virtual ~Server();

    QTcpServer *pTcpServer;
    QTcpSocket *pTcpSocket;

public slots:
    void processingRequest();
    void readyRead();
    void timeping();
    void timecheck();
    void joy();
    void checkRegInfo(QByteArray ba);
    void sendAnswer();

private:
    bool correctId = false;
    bool pingyes = false;
    bool noID = true;
    bool pingReg = false;

signals:

};

struct TCPJoyStruct //Структура данных с джойстика БСУ
{
    int16_t X,Y,Z;
    bool LButton, RButton;
    uint16_t buttons;
    uint16_t buttonsF;
};

class Client : public QObject // Класс TCP CLient для джойстика БСУ
{
    Q_OBJECT

public:
    Client(int a);
    virtual ~Client();
    void byteSort(TCPJoyStruct &Joy, QByteArray ba);
    void lJoyPack();
    void rJoyPack();
    int whichUUID = 0;
    char errorCode = 0;


public slots:
    void readyRead();
    void timeping();
    void timecheck();
    void newConnection();
    void sendInfo();

private:
    QTcpSocket *pTcpSocket;
    bool pingyes = false;

signals:
    void stopButtonPressed();
    void stopButtonReleased();

};

//Структуры посылок для TCP обмена
;
#pragma pack(push,1)
struct info_command {
    uint32_t packetDataSize = 0x0010;
    uint16_t packetType = 0x0000;
};
#pragma pack(pop)

;
#pragma pack(push,1)
struct request_command {
    uint32_t packetDataSize = 0x0010;
    uint16_t packetType = 0x0001;
};
#pragma pack(pop)

;
#pragma pack(push,1)
struct ping_command {
    uint32_t packetDataSize = 0x0000;
    uint16_t packetType = 0x0002;
};
#pragma pack(pop)

;
#pragma pack(push,1)
struct registration_messages{
    uint32_t packetDataSize = 0x0000;
    uint16_t packetType;
    //0x0003 - registration success
    //0x0004..0x0007 - error messages
};
#pragma pack(pop)

;
#pragma pack(push,1)
struct control_command {
    uint32_t packetDataSize = 0x000c;
    uint16_t packetType = 0x005f;
    int16_t xOffset =  -3;
    int16_t yOffset = -67;
    int16_t angleOffset = 78;
    int16_t yStickOffset = 0x8990;
    uint16_t buttons = 0x8990;
    uint16_t buttonsF = 0x8990;
};
#pragma pack(pop)


class RTCPThread : public QThread //Класс потока TCP обмена с правым джойстиком БСУ
{
    Q_OBJECT

public:
    void run();
    void stop();
    RTCPThread();
    void makeNewClient();

public slots:
    void waitTimeExpired();
signals:

private:
};

class LTCPThread : public QThread //Класс потока TCP обмена c левым джойстиком БСУ
{
    Q_OBJECT

public:
    void run();
    void stop();
    LTCPThread();
    void makeNewClient();

public slots:
    void waitTimeExpired();
signals:

private:
};

#endif // SERVER_H
