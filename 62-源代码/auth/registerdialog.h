#pragma once

#include "domain.h"
#include <QDialog>
#include <QLineEdit>

class RegisterDialog : public QDialog {
    Q_OBJECT
public:
    explicit RegisterDialog(QWidget* parent = nullptr);
    QString username() const;

private:
    void buildUi();
    QLineEdit* m_userEdit;
    QLineEdit* m_displayEdit;
    QLineEdit* m_passEdit;
    QLineEdit* m_passConfirmEdit;
};
