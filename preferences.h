#pragma once

#include <QWidget>
#include <QScopedPointer>
#include "serverapi.h"

namespace Ui {
class Preferences;
}

class Preferences : public QWidget
{
    Q_OBJECT

public:
    Preferences(QWidget *parent = nullptr);
    ~Preferences();

    void setCurrentServer(TServer const&);
    void setPlan(TPlan const&);
    void setMessage(TMessage const&);

    TProtocol currentProtocol() const;

signals:
    void goBack();

private slots:
    void saveSettings();
    void gotoGeneral();
    void gotoConnection();

protected:
    void showEvent(QShowEvent*);

private:
    QScopedPointer<Ui::Preferences> m_ui;
    TServer m_server;
    QList<TProtocol> m_protocols;
};
