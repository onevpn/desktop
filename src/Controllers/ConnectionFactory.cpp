#include "ConnectionFactory.h"

#ifdef Q_OS_MAC
#	include "Mac/Connection.h"
#elif defined Q_OS_LINUX
#   include "../Linux/connection.h"
#elif defined Q_OS_WIN
#   include "Windows/connection.h"
#endif

AbstractConnection* ConnectionFactory::createConnection()
{
    return new Connection();
}


