#pragma once

#include <QDialog>
#include <memory>

namespace Ui { class TrayPopover; }

class TrayPopover : public QDialog
{
    Q_OBJECT

public:
    TrayPopover(QWidget* parent = nullptr);
    ~TrayPopover();
    void steal(QWidget*);

signals:
    void closed();
    void gotoFullMode();

private:
    void hideEvent(QHideEvent*) override;

private:
    std::unique_ptr<Ui::TrayPopover> m_ui;
};
