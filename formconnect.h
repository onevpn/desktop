#ifndef FORMCONNECT_H
#define FORMCONNECT_H

#include <QWidget>
#include <QMenu>
#include <QScopedPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

#include "serverapi.h"

namespace Ui {
class FormConnect;
}

class FormConnect : public QWidget
{
    Q_OBJECT

public:
    explicit FormConnect(QWidget *parent = Q_NULLPTR);
    ~FormConnect();

    void setIP(const QString &ip, const QString &countryCode);

    enum { CONNECT_BUTTON_ON, CONNECT_BUTTON_OFF, CONNECT_BUTTON_CONNECTING, CONNECT_BUTTON_ERROR };

    void setConnectingState(int state);
    int connectingState() { return m_state; }

    void enableAll(bool enable);
    void fillServers(TConfig const&);

public slots:
    void setDownloaded(QString const&);
    void setUploaded(QString const&);

    int currentServer() const;
    void setCurrentServerIndex(int);

signals:
    void gotoPreferences();
    void connectionRequested(bool);
    void settingsRequested(bool);
    void logoutRequested();
    void paranoicModeRequested(bool);
    void serverChanged(int);

private slots:
    void onMovieChanged();
    void onMyAccount();
    void onSettingsClicked();

    void repolish(int);
    void makekillswitch(bool);
    void getUrl();
    void replyFinished(QNetworkReply*);

private:
    void showEvent(QShowEvent*);

private:
    QScopedPointer<Ui::FormConnect> ui;
    QString m_countryCode;
    int m_state;
    QMovie *m_movieConnecting;

    QNetworkAccessManager* m_accessManager;
    QMap<QNetworkReply*, int> m_reply2item;
};

#endif // FORMCONNECT_H
