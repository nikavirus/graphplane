#include "scene3d.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Scene3D w;
    w.resize(600, 600);
    w.show();

    return a.exec();
}
