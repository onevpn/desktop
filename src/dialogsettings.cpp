#include "dialogsettings.h"
#include "ui_dialogsettings.h"
#include <QSettings>
#include "settings.h"

DialogSettings::DialogSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSettings)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(ui->btnOk, SIGNAL(clicked(bool)), SLOT(onOk()));

    QSettings settings;
    ui->cbDnsLeaks->setChecked(settings.value("dnsLeaks", false).toBool());
    ui->cbKillSwitch->setChecked(settings.value("killSwitch", false).toBool());

    ui->lblTitle->setText("OneVPN " + QString(VERSION));
}

DialogSettings::~DialogSettings()
{
    delete ui;
}

void DialogSettings::onOk()
{
    QSettings settings;
    settings.setValue("dnsLeaks", ui->cbDnsLeaks->isChecked());
    settings.setValue("killSwitch", ui->cbKillSwitch->isChecked());
    accept();
}
