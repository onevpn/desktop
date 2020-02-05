#pragma once

#include "Interfaces/AbstractConnection.h"

class ConnectionFactory
{
public:
    AbstractConnection* createConnection();
};
