#pragma once

#include <QObject>

class QTimer;

class Paranoik : public QObject
{
    Q_OBJECT
public:
    Paranoik(QObject *parent);
    virtual ~Paranoik();
public slots:
    void start();
    void stop();
private slots:
    void makeWork();
signals:
    void shouldRestart();
private:
    QTimer* m_timer;
};
