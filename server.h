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


private:
    bool correctId = false;
    bool pingyes = false;

signals:

};
struct TCPJoyStruct //Структура данных с джойстика БСУ
{
    uint16_t X,Y,Z;
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
    void byteSort(TCPJoyStruct &Joy, QByteArray &ba);
    void lJoyPack();
    void rJoyPack();
    int whichUUID = 0;


public slots:
    void readyRead();
    void timeping();
    void timecheck();
    void newConnection();

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

class LTCPThread : public QThread //Класс потока TCP обмена с правым джойстиком БСУ
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

;
#pragma pack(push,1)
struct control_command {
    uint32_t packetDataSize = 0x000e;
    uint16_t packetType = 0x005f;
    uint16_t xOffset =  0x8990;
    uint16_t yOffset = 0x2340;
    uint16_t angleOffset = 0x0341;
    uint16_t yStickOffset = 0x8990;
    uint16_t buttons = 0x8990;
    uint16_t buttonsF = 0x8990;
};
#pragma pack(pop)




#endif // SERVER_H
