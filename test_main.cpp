#include <QApplication>
#include <QLabel>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QLabel label("WaveRider — Qt/MinGW compilation test OK!");
    label.setMinimumSize(400, 200);
    label.setAlignment(Qt::AlignCenter);
    label.setStyleSheet("font-size: 18px; font-weight: bold; color: #1abc9c;");
    label.show();
    return app.exec();
}
