#include "convoy.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Convoy w;
    w.show();

    return a.exec();
}
