#include "serversmodel.h"
#include <QTime>
#include <QDebug>

QVariant ServersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (section == 0)
        {
            return "Location";
        }
    }
    return QVariant();
}

int ServersModel::columnCount(const QModelIndex &parent) const
{
    return COLUMNS_COUNT;
}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return servers_.count();
    }
    else
        return 0;
}

QVariant ServersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }
    if (role == Qt::DisplayRole)
    {
        if (index.column() == 0)
        {
            return servers_.at(index.row())->name;
        }
    }
    /*// country code for flags
    else if (role == Qt::UserRole + 1)
    {
        if (index.column() == 0)
        {
            return servers_.at(index.row())->countryCode;
        }
    }
    // connection speed
    else if (role == Qt::UserRole + 2)
    {
        if (index.column() == 0)
        {
            return servers_.at(index.row())->connectionSpeed_;
        }
    }*/

    return QVariant();
}

void ServersModel::setServers(QVector<TServer const *> &servers)
{
    beginInsertRows(QModelIndex(), 0, servers.count() - 1);
    servers_ = servers;
    endInsertRows();
}

void ServersModel::clear()
{
    beginResetModel();
    servers_.clear();
    endResetModel();
}

