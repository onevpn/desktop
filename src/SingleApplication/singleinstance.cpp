#include "singleinstance.h"

#include <QByteArray>
#include <QDebug>
#include <QTimer>
#include <QEventLoop>
#include <QCoreApplication>

SingleInstance::SingleInstance(QString id, QObject *parent)
	: QObject(parent)
	, m_otherInstanceRunning(false)
	, m_id(id)
{
	m_sharedMemory.setKey(id);
	if (m_sharedMemory.attach())
	{
		m_otherInstanceRunning = true; // but it can be a memory from crashed instance
		m_sharedMemory.detach();
	}
	else
	{
		m_otherInstanceRunning = false;
		if (!m_sharedMemory.create(sizeof(quint64)))
		{
			qWarning() << "Unable to create shared memory segment";
			return;
		}
		m_sharedMemory.lock();
		quint64 *pid = (quint64 *)m_sharedMemory.data();
		*pid = QCoreApplication::applicationPid();
		m_sharedMemory.unlock();
	}
}

bool SingleInstance::waitForOtherInstance(long long timeout)
{
	if (!m_otherInstanceRunning)
		return true;

	long long waitCount = timeout / 1000LL;
	QEventLoop waitLoop;
	for (long long i = 0LL; i < waitCount; i++)
	{
		do {
			if (!m_sharedMemory.attach())
			{
				// memory was released
				if (!m_sharedMemory.create(sizeof(quint64)))
				{
					qWarning() << "Unable to create shared memory segment";
					break;
				}
				m_sharedMemory.lock();
				quint64 *pid = (quint64 *)m_sharedMemory.data();
				*pid = QCoreApplication::applicationPid();
				m_sharedMemory.unlock();
				return true;
			}
			else
				m_sharedMemory.detach();
		} while (false);
		QTimer::singleShot(1000, &waitLoop, SLOT(quit()));
		waitLoop.exec();
	}
	return false;
}
