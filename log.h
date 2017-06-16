//---------------------------------------------------------------------------
#pragma once
#include <QMutex>

class QWidget;

#define SAFE_DELETE(x)  if (x) { delete x; x = NULL; }

class ALog
{
public:
    static QWidget *parent_;
    static void Out(QString str);
    static void Clear();
    static QMutex mutex_;
};
//---------------------------------------------------------------------------
