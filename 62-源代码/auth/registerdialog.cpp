#include "registerdialog.h"
#include "core/storage.h"
#include "core/theme.h"

#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

RegisterDialog::RegisterDialog(QWidget* parent) : QDialog(parent) {
    buildUi();
    setWindowTitle("注册 ChronoScope");
}

QString RegisterDialog::username() const {
    return m_userEdit->text().trimmed();
}

void RegisterDialog::buildUi() {
    setMinimumSize(380, 370);
    setMaximumSize(440, 420);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(Spacing::XXL, Spacing::XXL, Spacing::XXL, Spacing::XXL);
    root->setSpacing(Spacing::MD);

    auto* title = new QLabel("<h2 style='margin:0'>✨ 创建账号</h2>", this);
    root->addWidget(title);

    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText("用户名（字母数字，至少 3 位）");
    m_userEdit->setMinimumHeight(38);
    root->addWidget(m_userEdit);

    m_displayEdit = new QLineEdit(this);
    m_displayEdit->setPlaceholderText("显示名称");
    m_displayEdit->setMinimumHeight(38);
    root->addWidget(m_displayEdit);

    m_passEdit = new QLineEdit(this);
    m_passEdit->setPlaceholderText("密码（至少 4 位）");
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setMinimumHeight(38);
    root->addWidget(m_passEdit);

    m_passConfirmEdit = new QLineEdit(this);
    m_passConfirmEdit->setPlaceholderText("确认密码");
    m_passConfirmEdit->setEchoMode(QLineEdit::Password);
    m_passConfirmEdit->setMinimumHeight(38);
    root->addWidget(m_passConfirmEdit);

    root->addSpacing(Spacing::SM);

    auto* buttons = new QDialogButtonBox(this);
    auto* regBtn = buttons->addButton("注册", QDialogButtonBox::AcceptRole);
    auto* backBtn = buttons->addButton("返回登录", QDialogButtonBox::AcceptRole);
    backBtn->setStyleSheet("QPushButton { background: transparent; color: #4f46e5; border: 1px solid #4f46e5; border-radius: 6px; padding: 8px 14px; }");

    connect(regBtn, &QPushButton::clicked, this, [this]() {
        const QString user = m_userEdit->text().trimmed();
        const QString display = m_displayEdit->text().trimmed();
        const QString pass = m_passEdit->text();
        const QString passConfirm = m_passConfirmEdit->text();

        if (user.length() < 3) {
            QMessageBox::warning(this, "提示", "用户名至少 3 个字符。"); return;
        }
        if (pass.length() < 4) {
            QMessageBox::warning(this, "提示", "密码至少 4 个字符。"); return;
        }
        if (pass != passConfirm) {
            QMessageBox::warning(this, "提示", "两次密码输入不一致。"); return;
        }
        if (JsonStorage::instance().userExists(user)) {
            QMessageBox::warning(this, "提示", "用户名已被注册。"); return;
        }

        UserProfile profile;
        profile.username = user;
        profile.passwordHash = QString::fromUtf8(
            QCryptographicHash::hash(pass.toUtf8(), QCryptographicHash::Sha256).toHex());
        profile.displayName = display.isEmpty() ? user : display;
        profile.createdAt = QDateTime::currentDateTime();

        if (!JsonStorage::instance().registerUser(profile)) {
            QMessageBox::warning(this, "错误", "注册失败，请重试。"); return;
        }
        QMessageBox::information(this, "注册成功",
                                 QString("欢迎加入 ChronoScope，%1！\n现在可以登录了。").arg(profile.displayName));
        accept();
    });

    connect(backBtn, &QPushButton::clicked, this, &QDialog::accept);
    root->addWidget(buttons);
}
