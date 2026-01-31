#ifndef HISTORYTAB_H
#define HISTORYTAB_H

#include <QWidget>
#include <QTableWidget>

class HistoryTab : public QWidget {
    Q_OBJECT

public:
    explicit HistoryTab(QWidget *parent = nullptr);
    void addEntry(const QString& type, const QString& details, const QString& result);

private:
    void setupUI();
    void loadHistory();
    
    QTableWidget *historyTable;
};

#endif // HISTORYTAB_H