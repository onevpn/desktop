#ifndef DIALOGRESTOREINTERNET_H
#define DIALOGRESTOREINTERNET_H

#include <QDialog>

namespace Ui {
class DialogRestoreInternet;
}

class DialogRestoreInternet : public QDialog
{
    Q_OBJECT

public:
    explicit DialogRestoreInternet(QWidget *parent = 0);
    ~DialogRestoreInternet();

    enum RESULT { CLOSE, RESTORE, RESTORE_AND_RECONNECT };

    RESULT getResult() { return result_; }

private slots:
    void onClickClose();
    void onClickRestore();
    void onClickRestoreAndReconnect();

private:
    Ui::DialogRestoreInternet *ui;
    RESULT result_;
};

#endif // DIALOGRESTOREINTERNET_H
