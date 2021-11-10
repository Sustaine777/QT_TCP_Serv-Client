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

class Client : public QObject
{
    Q_OBJECT

public:
    Client(QObject *parent = nullptr);
    virtual ~Client();

public slots:
    void readyRead();
    void timeping();
    void timecheck();

private:
    QTcpSocket *pTcpSocket;
    bool pingyes = false;

signals:

};

;
#pragma pack(push,1)
struct info_command {
    uint16_t packetDataSize = 0x0010;
    uint16_t packetType = 0x0000;
};
#pragma pack(pop)

;
#pragma pack(push,1)
struct request_command {
    uint16_t packetDataSize = 0x0010;
    uint16_t packetType = 0x0001;
};
#pragma pack(pop)

;
#pragma pack(push,1)
struct ping_command {
    uint16_t packetDataSize = 0x0000;
    uint16_t packetType = 0x0002;
};
#pragma pack(pop)

;
#pragma pack(push,1)
struct control_command {
    uint16_t packetDataSize = 0x000e;
    uint16_t packetType = 0x005f;
    uint16_t xOffset =  0x000e;
    uint16_t yOffset = 0x000e;
    uint16_t angleOffset = 0x000e;
    uint16_t yStickOffset = 0x8990;
    uint16_t LEDs = 0x0000;
    uint32_t buttons = 0x3FFFFFFF;
};
#pragma pack(pop)


struct JoyStruct
{
    uint16_t X,Y,Z;
    bool LButton,RButton;
    uint32_t buttons;
};

#endif // SERVER_H
