#include <QCoreApplication>
#include "server.h"

Server *s;
Client *c;
//Client *d;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    s = new Server;
    c = new Client;
    //d = new Client;
    return a.exec();
}
