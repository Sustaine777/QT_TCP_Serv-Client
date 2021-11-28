#include <QCoreApplication>
#include "server.h"

Server *s;
extern RTCPThread RTCPT;
extern LTCPThread LTCPT;
//Client *d;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    s = new Server;
    RTCPT.start();
    //d = new Client(0);
    //d = new Client;
    return a.exec();
}
