#pragma once

#include <QLabel>

class QMovie;

class AnimatedButton : public QLabel
{
    Q_OBJECT
public:
    AnimatedButton(QWidget* = Q_NULLPTR);
    void startAnimation();
    void stopAnimation();

signals:
    void clicked(bool checked = false);

private slots:
    void onFinished();

private:
    bool event(QEvent*) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent* ) Q_DECL_OVERRIDE;

private:
    QMovie* m_movie;
};
