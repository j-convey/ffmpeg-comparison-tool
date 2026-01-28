#include "MainWindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application icon
    app.setWindowIcon(QIcon(":/ffmpeg-icon.ico"));
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
