#ifndef KILLSWITCH_H
#define KILLSWITCH_H

#include <QObject>
#include <QString>

class KillSwitch : public QObject
{

    Q_OBJECT

public:
    KillSwitch(QObject *parent);
    virtual ~KillSwitch();

    bool killInternet();    //delete default gateway
    bool restoreInternet();
    void saveDefaultGateway();
    bool isActive();

private:
    struct GatewayData
    {
        QString interface_;
        QString host_;
    };

    QList<GatewayData> loadGatewayFromSettings();
};

#endif // KILLSWITCH_H
