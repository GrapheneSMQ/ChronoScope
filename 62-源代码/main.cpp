#include "auth/logindialog.h"
#include "auth/registerdialog.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ChronoScope");
    app.setOrganizationName("ChronoScope");

    // ── 鉴权流程（支持自动登录） ──
    QString username;

    // 1. 尝试自动登录
    username = LoginDialog::tryAutoLogin();

    // 2. 自动登录失败 → 手动登录/注册
    if (username.isEmpty()) {
        while (username.isEmpty()) {
            LoginDialog loginDlg;
            if (loginDlg.exec() == QDialog::Accepted) {
                username = loginDlg.username();
                break;
            }
            RegisterDialog regDlg;
            if (regDlg.exec() != QDialog::Accepted) {
                return 0;  // 用户取消注册 → 退出
            }
            // 注册成功 → 回到登录
        }
    }

    MainWindow window(username);
    window.show();
    return app.exec();
}
