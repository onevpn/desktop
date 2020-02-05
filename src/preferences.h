#pragma once

#include <QWidget>
#include <QScopedPointer>
#include "DAOs/configs.h"
#include "pubsub/Subscriber.h"
#include "DAOs/Log.h"

namespace Ui {
class Preferences;
}

class Preferences : public QWidget
	, private PubSub::Subscriber<Log::DisabledWidgetRequest>
{
    Q_OBJECT

public:
    Preferences(QWidget *parent = nullptr);
    ~Preferences();

    void setCurrentServer(Config::Server const&);
    void setPlan(Config::Plan const&);
    void setMessage(Config::Message const&);

    Config::Protocol currentProtocol() const;

signals:
    void goBack();

private slots:
    void saveSettings();
    void gotoGeneral();
    void gotoConnection();

protected:
    void showEvent(QShowEvent*) override;

private:
	void notify(Log::DisabledWidgetRequest const&) override;

private:
    QScopedPointer<Ui::Preferences> m_ui;
    Config::Server m_server;
    QList<Config::Protocol> m_protocols;
};
