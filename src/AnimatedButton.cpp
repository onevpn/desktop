#include "AnimatedButton.h"

#include <QEvent>
#include <QMovie>
#include <QDebug>

AnimatedButton::AnimatedButton(QWidget* parent)
    : QLabel(parent)
{
     m_movie = new QMovie(":/MainWindow/OneVPN/button_connecting.gif", QByteArray(), this);
     connect(m_movie, SIGNAL(finished()), SLOT(onFinished()));
}

void AnimatedButton::onFinished()
{
    if(m_movie && m_movie->state() != QMovie::Running)
        m_movie->start();
}

bool AnimatedButton::event(QEvent* event)
{
    switch(event->type())
    {
    case QEvent::MouseButtonPress:
        emit clicked();
        break;
    default:
        break;
    }
    return QLabel::event(event);
}

void AnimatedButton::showEvent(QShowEvent * event)
{
    QLabel::showEvent(event);
}

void AnimatedButton::startAnimation()
{
    if(m_movie->state() != QMovie::Running)
    {
        QSize sz = size();
        m_movie->setScaledSize(sz);
        setMovie(m_movie);
        m_movie->start();
    }
}

void AnimatedButton::stopAnimation()
{
    m_movie->stop();
    setMovie(Q_NULLPTR);
}
