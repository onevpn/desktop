#include "flagsresource.h"

extern double g_ui_scale;

FlagsResource::FlagsResource()
{
}

FlagsResource::~FlagsResource()
{
    for (QMap<QString, QPixmap *>::iterator it = flags_.begin(); it != flags_.end(); ++it)
    {
        if (it.value() != NULL)
        {
            delete it.value();
        }
    }
}

QPixmap *FlagsResource::getFlag(const QString &countryCode)
{
    QMap<QString, QPixmap *>::iterator it = flags_.find(countryCode);
    if (it != flags_.end())
    {
        return it.value();
    }
    else
    {
        QPixmap *pixmap =  new QPixmap(":/Flags/Images/flags/" + countryCode.toUpper() + ".png");
        if (!pixmap->isNull())
        {
            flags_[countryCode] = pixmap;
            return pixmap;
        }
        else
        {
            delete pixmap;
            flags_[countryCode] = NULL;
            return NULL;
        }
    }
}
