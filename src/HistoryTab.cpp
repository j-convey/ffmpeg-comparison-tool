#include "HistoryTab.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>

HistoryTab::HistoryTab(QWidget *parent) : QWidget(parent) {
    setupUI();
    loadHistory();
}

void HistoryTab::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    historyTable = new QTableWidget(this);
    historyTable->setColumnCount(4);
    historyTable->setHorizontalHeaderLabels({"Date/Time", "Type", "Details", "Result"});
    historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    historyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    historyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    historyTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    layout->addWidget(historyTable);
}

void HistoryTab::addEntry(const QString& type, const QString& details, const QString& result) {
    int row = historyTable->rowCount();
    historyTable->insertRow(row);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    
    historyTable->setItem(row, 0, new QTableWidgetItem(timestamp));
    historyTable->setItem(row, 1, new QTableWidgetItem(type));
    historyTable->setItem(row, 2, new QTableWidgetItem(details));
    historyTable->setItem(row, 3, new QTableWidgetItem(result));
    
    // Save to CSV file
    QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/FFmpegComparisonTool_History.csv";
    QFile file(path);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        // Use a custom delimiter ";,;" to avoid issues with commas in filenames/results
        out << timestamp << ";,;" << type << ";,;" << details << ";,;" << result << "\n";
    }
}

void HistoryTab::loadHistory() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/FFmpegComparisonTool_History.csv";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList fields = line.split(";,;");
            if (fields.size() >= 4) {
                int row = historyTable->rowCount();
                historyTable->insertRow(row);
                historyTable->setItem(row, 0, new QTableWidgetItem(fields[0]));
                historyTable->setItem(row, 1, new QTableWidgetItem(fields[1]));
                historyTable->setItem(row, 2, new QTableWidgetItem(fields[2]));
                historyTable->setItem(row, 3, new QTableWidgetItem(fields[3]));
            }
        }
    }
}