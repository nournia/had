#include <QtGui/QApplication>
#include "mainwindow.h"

 #include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // translation
    QTranslator translator;
    translator.load("had_fa");
    a.installTranslator(& translator);
    a.setFont(QFont("B Mitra", 14));

    MainWindow w;
    w.show();

    return a.exec();
}
