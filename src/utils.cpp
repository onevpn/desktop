#include "utils.h"

using namespace Utils;

double roundOff(double n)
{
    double d = n * 100.0;
    int i = d + 0.5;
    d = (float)i / 100.0;
    return d;
}

QString Utils::sizeInStr(quint64 size)
{
    static const char *SIZES[] = { "b", "kb", "mb", "gb" };
    int div = 0;
    size_t rem = 0;

    //qint64 size = limit - total;
    if (size > 0)
    {

        while (size >= 1024 && div < (sizeof SIZES / sizeof *SIZES))
        {
            rem = (size % 1024);
            div++;
            size /= 1024;
        }

        double size_d = (float)size + (float)rem / 1024.0;
        QString result = QString::number(roundOff(size_d)) + " " + SIZES[div];
        return result;
    }
    else
    {
        return "0 b";
    }
}
