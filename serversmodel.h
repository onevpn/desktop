#ifndef SERVERSMODEL_H
#define SERVERSMODEL_H

#include <QAbstractListModel>
#include <QTimer>
#include "serverapi.h"

class ServersModel : public QAbstractListModel
{
    Q_OBJECT
public:
    using QAbstractListModel::QAbstractListModel;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    void setServers(QVector<TServer const*> &servers);
    void clear();

private:
    QVector<TServer const*> servers_;
    enum { COLUMNS_COUNT = 1 };
};

#endif // SERVERSMODEL_H
