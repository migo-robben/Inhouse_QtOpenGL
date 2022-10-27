#include <QtWidgets>
#include <QDialog>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QDialog window;
    window.resize(640, 480);
    window.show();

    return QApplication::exec();
}