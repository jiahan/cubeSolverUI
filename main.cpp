#include "cubesolver.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CubeSolver w;
    w.show();

    //w.calculateStep();

    return a.exec();
}
