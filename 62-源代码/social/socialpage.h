#pragma once

#include "domain.h"
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>

// ── 社交页面 ──
// 好友列表 + 搜索 + 助力 + 查看好友日历

class SocialPage : public QWidget {
    Q_OBJECT
public:
    explicit SocialPage(const QString& currentUser, QWidget* parent = nullptr);

    void setUser(const QString& user);
    void refresh();

signals:
    void boostSent(const QString& toUser);

private:
    void buildUi();
    void searchUsers();
    void sendRequest();
    void acceptRequest(const QString& from);
    void sendBoost(const QString& to);
    void loadFriends();
    void loadRequests();

    QString m_currentUser;
    QListWidget* m_friendList;
    QListWidget* m_requestList;
    QLineEdit* m_searchEdit;
    QListWidget* m_searchResults;
    QLabel* m_boostLabel;
    QPushButton* m_boostBtn;
};
