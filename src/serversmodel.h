#pragma once
#include <QAbstractListModel>
#include "DAOs/configs.h"

class ServersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    using QAbstractListModel::QAbstractListModel;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    void setServers(QList<Config::Server> const &servers);
    void clear();

private:
    QList<Config::Server> m_servers;
    enum { COLUMNS_COUNT = 1 };
};

