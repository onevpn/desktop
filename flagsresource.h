#ifndef FLAGSRESOURCE_H
#define FLAGSRESOURCE_H

#include <QPixmap>
#include <QMap>

class FlagsResource
{
public:
    static FlagsResource &instance()
    {
        static FlagsResource fr;
        return fr;
    }

    QPixmap *getFlag(const QString &countryCode);


private:
    FlagsResource();
    ~FlagsResource();

    QMap<QString, QPixmap *> flags_;
};

#endif // FLAGSRESOURCE_H
