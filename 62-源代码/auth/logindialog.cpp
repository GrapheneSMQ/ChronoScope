#include "logindialog.h"
#include "core/storage.h"
#include "core/theme.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

// ── 凭据持久化 key（存于 QSettings） ──
static const char* KEY_USER = "saved/username";
static const char* KEY_PASS = "saved/password";   // base64 编码

static QString obfuscate(const QString& plain) {
    return plain.toUtf8().toBase64();
}

static QString deobfuscate(const QString& b64) {
    return QString::fromUtf8(QByteArray::fromBase64(b64.toUtf8()));
}

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent) {
    buildUi();
    setWindowTitle("登录 ChronoScope");
    loadSavedCredentials();
}

QString LoginDialog::username() const {
    return m_userEdit->text().trimmed();
}

QString LoginDialog::password() const {
    return m_passEdit->text();
}

bool LoginDialog::rememberMe() const {
    return m_rememberCheck->isChecked();
}

QString LoginDialog::tryAutoLogin() {
    QSettings settings;
    const QString user = settings.value(KEY_USER).toString();
    const QString passB64 = settings.value(KEY_PASS).toString();
    if (user.isEmpty() || passB64.isEmpty()) return {};

    const QString pass = deobfuscate(passB64);
    auto* profile = JsonStorage::instance().login(user, pass);
    return profile ? user : QString();
}

void LoginDialog::saveCredentials(const QString& user, const QString& pass) {
    QSettings settings;
    settings.setValue(KEY_USER, user);
    settings.setValue(KEY_PASS, obfuscate(pass));
}

void LoginDialog::clearCredentials() {
    QSettings settings;
    settings.remove(KEY_USER);
    settings.remove(KEY_PASS);
}

void LoginDialog::loadSavedCredentials() {
    QSettings settings;
    const QString user = settings.value(KEY_USER).toString();
    const QString passB64 = settings.value(KEY_PASS).toString();
    if (!user.isEmpty()) {
        m_userEdit->setText(user);
        if (!passB64.isEmpty()) {
            m_passEdit->setText(deobfuscate(passB64));
        }
        m_rememberCheck->setChecked(true);
    }
}

void LoginDialog::buildUi() {
    setMinimumSize(360, 310);
    setMaximumSize(420, 370);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(Spacing::XXL, Spacing::XXL, Spacing::XXL, Spacing::XXL);
    root->setSpacing(Spacing::LG);

    auto* title = new QLabel("<h2 style='margin:0'>🔐 登录</h2>", this);
    root->addWidget(title);

    auto* subtitle = new QLabel("ChronoScope · 看见时间，理解自己", this);
    subtitle->setStyleSheet("color: #64748b;");
    root->addWidget(subtitle);

    root->addSpacing(Spacing::MD);

    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText("用户名");
    m_userEdit->setMinimumHeight(38);
    root->addWidget(m_userEdit);

    m_passEdit = new QLineEdit(this);
    m_passEdit->setPlaceholderText("密码");
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setMinimumHeight(38);
    root->addWidget(m_passEdit);

    // 记住我复选框
    m_rememberCheck = new QCheckBox("记住我，下次自动登录", this);
    m_rememberCheck->setStyleSheet("color: #64748b; font-size: 12px; spacing: 6px;");
    root->addWidget(m_rememberCheck);

    root->addSpacing(Spacing::SM);

    auto* buttons = new QDialogButtonBox(this);
    auto* loginBtn = buttons->addButton("登录", QDialogButtonBox::AcceptRole);
    auto* regBtn = buttons->addButton("注册新账号", QDialogButtonBox::ActionRole);
    regBtn->setStyleSheet("QPushButton { background: transparent; color: #4f46e5; border: 1px solid #4f46e5; border-radius: 6px; padding: 8px 14px; }");

    connect(loginBtn, &QPushButton::clicked, this, [this]() {
        const QString user = m_userEdit->text().trimmed();
        const QString pass = m_passEdit->text();
        if (user.isEmpty() || pass.isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入用户名和密码。");
            return;
        }
        auto* profile = JsonStorage::instance().login(user, pass);
        if (!profile) {
            QMessageBox::warning(this, "登录失败", "用户名或密码错误。");
            return;
        }

        // 保存或清除凭据
        if (m_rememberCheck->isChecked()) {
            saveCredentials(user, pass);
        } else {
            clearCredentials();
        }
        accept();
    });

    connect(regBtn, &QPushButton::clicked, this, &QDialog::reject);

    root->addWidget(buttons);
}
