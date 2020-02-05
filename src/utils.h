#pragma once

#include <QString>

#define SAFE_DELETE(x) if (x) { delete x; x = NULL; }

enum PROTOCOL { OPENVPN = 1, PPTP = 2, L2TP = 4, SSTP = 8 };

enum PROTOCOL_OPENVPN { TCP, UDP };

namespace Utils
{
    QString sizeInStr(quint64 size);

}


