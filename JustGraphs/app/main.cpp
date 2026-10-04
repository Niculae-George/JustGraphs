#include "app/GraphVersePlatform.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    GraphVersePlatform window;
    window.show();
    return app.exec();
}
