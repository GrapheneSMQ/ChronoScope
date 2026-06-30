#pragma once

#include "domain.h"
#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>

// ── 登录弹窗（支持“记住我”） ──

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);
    QString username() const;
    QString password() const;
    bool rememberMe() const;

    // 自动登录：检查已保存凭据 → 返回用户名（成功）或空字符串（失败）
    static QString tryAutoLogin();

    // 保存 / 清除凭据
    static void saveCredentials(const QString& user, const QString& pass);
    static void clearCredentials();

private:
    void buildUi();
    void loadSavedCredentials();
    QLineEdit* m_userEdit;
    QLineEdit* m_passEdit;
    QCheckBox* m_rememberCheck;
};
