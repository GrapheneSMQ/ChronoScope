#pragma once

#include "domain.h"
#include <QDir>
#include <QString>
#include <QVector>

// ── 本地 JSON 持久化 ──
// 数据存储在 %APPDATA%/ChronoScope/ 下
// 多用户共享同一 JSON 文件（模拟本地社交）

class JsonStorage {
public:
    static JsonStorage& instance();

    // 用户管理
    bool registerUser(const UserProfile& user);
    UserProfile* login(const QString& username, const QString& password);
    UserProfile* user(const QString& username);
    QVector<UserProfile> allUsers() const;
    void updateUser(const UserProfile& user);
    bool userExists(const QString& username) const;

    // 好友管理
    void sendFriendRequest(const QString& from, const QString& to);
    void acceptFriendRequest(const QString& from, const QString& to);
    void removeFriend(const QString& user, const QString& friendName);
    QStringList pendingRequests(const QString& username) const;

    // 事件管理
    QVector<Event> eventsForUser(const QString& username) const;
    QVector<Event> eventsForDate(const QString& username, const QDate& date) const;
    QVector<Event> eventsForDateRange(const QString& username, const QDate& start, const QDate& end) const;
    void saveEvent(const Event& event);
    void deleteEvent(int eventId);
    int nextEventId();

    // 助力
    void addBoost(const BoostLog& boost);
    QVector<BoostLog> boostsForUser(const QString& username) const;
    int todayBoostCount(const QString& fromUser, const QString& toUser) const;

    // 持久化
    void save();
    void load();

private:
    JsonStorage();
    ~JsonStorage();
    QString dataPath() const;
    static QString hashPassword(const QString& password);

    QVector<UserProfile> m_users;
    QVector<Event> m_events;
    QVector<BoostLog> m_boosts;
    int m_nextEventId = 1000;
    bool m_loaded = false;
};
