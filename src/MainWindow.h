#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "HistoryTab.h"
#include "PredictTab.h"
#include "VerifyTab.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUI();
    
    HistoryTab *historyTab;
    PredictTab *predictTab;
    VerifyTab *verifyTab;
};

#endif // MAINWINDOW_H
