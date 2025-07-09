#include "nirscontrolform.h"

#include <QApplication>
//#include "analogfs.h"

extern double ai_fs;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NirsControlForm w;
    w.show();
    return a.exec();
}
