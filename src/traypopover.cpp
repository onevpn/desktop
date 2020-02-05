#include "traypopover.h"

#include "ui_traypopover.h"
#include <QApplication>

TrayPopover::TrayPopover(QWidget* parent)
    : QDialog(parent, Qt::Popup)
    , m_ui(new Ui::TrayPopover)
{
    m_ui->setupUi(this);
    connect(m_ui->exitButton, &QAbstractButton::clicked, []{ QApplication::exit(); });
    connect(m_ui->gotoFullModeButton, &QAbstractButton::clicked, this, &TrayPopover::gotoFullMode);
}

TrayPopover::~TrayPopover() = default;

void TrayPopover::steal(QWidget* victim)
{
    m_ui->slotLayout->addWidget(victim);
    adjustSize();
}

void TrayPopover::hideEvent(QHideEvent* event)
{
    emit closed();
    deleteLater();
    QDialog::hideEvent(event);
}
