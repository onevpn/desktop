#pragma once

#include <QObject>
#include <QSharedMemory>

class SingleInstance : public QObject
{
	Q_OBJECT
public:
    SingleInstance(QString id, QObject * parent = Q_NULLPTR);

	bool isOtherInstanceRunning() const
		{ return m_otherInstanceRunning; }
	/// Returns true if no other instances available
	bool waitForOtherInstance(long long timeout = 100000); // ms
private:
	bool m_otherInstanceRunning;
	QSharedMemory m_sharedMemory;
	QString m_id;
};
