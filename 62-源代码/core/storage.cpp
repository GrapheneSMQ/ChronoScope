#include "storage.h"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

JsonStorage& JsonStorage::instance() {
    static JsonStorage s;
    return s;
}

JsonStorage::JsonStorage() { load(); }
JsonStorage::~JsonStorage() { save(); }

QString JsonStorage::dataPath() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/chronoscope_data.json";
}

QString JsonStorage::hashPassword(const QString& pw) {
    return QString::fromUtf8(
        QCryptographicHash::hash(pw.toUtf8(), QCryptographicHash::Sha256).toHex());
}

// ── 序列化 / 反序列化 ──

void JsonStorage::save() {
    QJsonObject root;
    root["dataVersion"] = 2;

    // 用户
    QJsonArray usersArr;
    for (const auto& u : m_users) {
        QJsonObject obj;
        obj["username"] = u.username;
        obj["passwordHash"] = u.passwordHash;
        obj["displayName"] = u.displayName;
        obj["bio"] = u.bio;
        obj["boostPoints"] = u.boostPoints;
        QJsonArray friendsArr;
        for (const auto& f : u.friends) friendsArr.append(f);
        obj["friends"] = friendsArr;
        QJsonArray reqArr;
        for (const auto& r : u.friendRequests) reqArr.append(r);
        obj["friendRequests"] = reqArr;
        obj["createdAt"] = u.createdAt.toString(Qt::ISODate);
        obj["lastLoginDate"] = u.lastLoginDate.toString(Qt::ISODate);
        // 年度目标
        QJsonArray goalsArr;
        for (const auto& g : u.goals) {
            QJsonObject go;
            go["title"] = g.title;
            go["subtitle"] = g.subtitle;
            go["progress"] = g.progress;
            go["color"] = g.color.name();
            goalsArr.append(go);
        }
        obj["goals"] = goalsArr;
        usersArr.append(obj);
    }
    root["users"] = usersArr;

    // 事件
    QJsonArray eventsArr;
    for (const auto& e : m_events) {
        QJsonObject obj;
        obj["id"] = e.id;
        obj["title"] = e.title;
        obj["description"] = e.description;
        obj["location"] = e.location;
        obj["startTime"] = e.startTime.toString(Qt::ISODate);
        obj["endTime"] = e.endTime.toString(Qt::ISODate);
        obj["category"] = static_cast<int>(e.category);
        obj["priority"] = static_cast<int>(e.priority);
        obj["goal"] = e.goal;
        obj["privacy"] = static_cast<int>(e.privacy);
        obj["owner"] = e.owner;
        obj["completed"] = e.completed;
        eventsArr.append(obj);
    }
    root["events"] = eventsArr;
    root["nextEventId"] = m_nextEventId;

    // 助力
    QJsonArray boostsArr;
    for (const auto& b : m_boosts) {
        QJsonObject obj;
        obj["fromUser"] = b.fromUser;
        obj["toUser"] = b.toUser;
        obj["timestamp"] = b.timestamp.toString(Qt::ISODate);
        obj["message"] = b.message;
        boostsArr.append(obj);
    }
    root["boosts"] = boostsArr;

    QFile file(dataPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

void JsonStorage::load() {
    if (m_loaded) return;
    m_loaded = true;

    QFile file(dataPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return;
    QJsonObject root = doc.object();

    // 用户
    for (const auto& val : root["users"].toArray()) {
        QJsonObject obj = val.toObject();
        UserProfile u;
        u.username = obj["username"].toString();
        u.passwordHash = obj["passwordHash"].toString();
        u.displayName = obj["displayName"].toString();
        u.bio = obj["bio"].toString();
        u.boostPoints = obj["boostPoints"].toInt();
        for (const auto& f : obj["friends"].toArray())
            u.friends.append(f.toString());
        for (const auto& r : obj["friendRequests"].toArray())
            u.friendRequests.append(r.toString());
        u.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
        u.lastLoginDate = QDate::fromString(obj["lastLoginDate"].toString(), Qt::ISODate);
        for (const auto& gv : obj["goals"].toArray()) {
            QJsonObject go = gv.toObject();
            GoalNode g;
            g.title = go["title"].toString();
            g.subtitle = go["subtitle"].toString();
            g.progress = go["progress"].toDouble();
            g.color = QColor(go["color"].toString());
            u.goals.append(g);
        }
        m_users.append(u);
    }

    // 事件
    for (const auto& val : root["events"].toArray()) {
        QJsonObject obj = val.toObject();
        Event e;
        e.id = obj["id"].toInt();
        e.title = obj["title"].toString();
        e.description = obj["description"].toString();
        e.location = obj["location"].toString();
        e.startTime = QDateTime::fromString(obj["startTime"].toString(), Qt::ISODate);
        e.endTime = QDateTime::fromString(obj["endTime"].toString(), Qt::ISODate);
        e.category = static_cast<Category>(obj["category"].toInt());
        e.priority = static_cast<Priority>(obj["priority"].toInt());
        e.goal = obj["goal"].toString();
        e.privacy = static_cast<PrivacyLevel>(obj["privacy"].toInt());
        e.owner = obj["owner"].toString();
        e.completed = obj["completed"].toBool();
        m_events.append(e);
    }

    m_nextEventId = root["nextEventId"].toInt(1000);

    // 助力
    for (const auto& val : root["boosts"].toArray()) {
        QJsonObject obj = val.toObject();
        BoostLog b;
        b.fromUser = obj["fromUser"].toString();
        b.toUser = obj["toUser"].toString();
        b.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        b.message = obj["message"].toString();
        m_boosts.append(b);
    }

    // 数据迁移：ver<2 → 清零所有助力数据
    if (root["dataVersion"].toInt(0) < 2) {
        m_boosts.clear();
        for (auto& u : m_users) u.boostPoints = 0;
    }

    // 首次启动 → 创建演示用户供社交功能使用
    if (m_users.isEmpty()) {
        auto makeDemo = [&](const QString& name, const QString& display, const QString& pw, const QString& b) {
            UserProfile u;
            u.username = name; u.displayName = display; u.bio = b;
            u.passwordHash = QString::fromUtf8(
                QCryptographicHash::hash(pw.toUtf8(), QCryptographicHash::Sha256).toHex());
            u.createdAt = QDateTime::currentDateTime();
            m_users.append(u);
        };
        makeDemo("alice", "Alice", "1234", "CS 大三 · 跑步 + 刷题");
        makeDemo("bob", "Bob", "1234", "篮球 · 吉他 · 代码");
        makeDemo("charlie", "Charlie", "1234", "产品设计 · 摄影爱好者");
        save();
    }
}

// ── 用户管理 ──

bool JsonStorage::registerUser(const UserProfile& user) {
    if (userExists(user.username)) return false;
    m_users.append(user);
    save();
    return true;
}

UserProfile* JsonStorage::login(const QString& username, const QString& password) {
    const QString hash = hashPassword(password);
    for (auto& u : m_users) {
        if (u.username == username && u.passwordHash == hash)
            return &u;
    }
    return nullptr;
}

UserProfile* JsonStorage::user(const QString& username) {
    for (auto& u : m_users) {
        if (u.username == username) return &u;
    }
    return nullptr;
}

QVector<UserProfile> JsonStorage::allUsers() const {
    return m_users;
}

void JsonStorage::updateUser(const UserProfile& user) {
    for (auto& u : m_users) {
        if (u.username == user.username) {
            u = user;
            save();
            return;
        }
    }
}

bool JsonStorage::userExists(const QString& username) const {
    for (const auto& u : m_users) {
        if (u.username == username) return true;
    }
    return false;
}

// ── 好友管理 ──

void JsonStorage::sendFriendRequest(const QString& from, const QString& to) {
    if (auto* u = user(to)) {
        if (!u->friendRequests.contains(from) && !u->friends.contains(from)) {
            u->friendRequests.append(from);
            save();
        }
    }
}

void JsonStorage::acceptFriendRequest(const QString& from, const QString& to) {
    if (auto* a = user(to)) {
        a->friendRequests.removeAll(from);
        if (!a->friends.contains(from)) a->friends.append(from);
    }
    if (auto* b = user(from)) {
        if (!b->friends.contains(to)) b->friends.append(to);
    }
    save();
}

void JsonStorage::removeFriend(const QString& user, const QString& friendName) {
    if (auto* u = JsonStorage::user(user)) u->friends.removeAll(friendName);
    if (auto* f = JsonStorage::user(friendName)) f->friends.removeAll(user);
    save();
}

QStringList JsonStorage::pendingRequests(const QString& username) const {
    if (auto* u = const_cast<JsonStorage*>(this)->user(username))
        return u->friendRequests;
    return {};
}

// ── 事件管理 ──

QVector<Event> JsonStorage::eventsForUser(const QString& username) const {
    QVector<Event> result;
    for (const auto& e : m_events) {
        if (e.owner == username) result.append(e);
    }
    return result;
}

QVector<Event> JsonStorage::eventsForDate(const QString& username, const QDate& date) const {
    return eventsForDateRange(username, date, date.addDays(1));
}

QVector<Event> JsonStorage::eventsForDateRange(const QString& username, const QDate& start, const QDate& end) const {
    QVector<Event> result;
    const QDateTime rangeStart(start, QTime(0, 0));
    const QDateTime rangeEnd(end, QTime(23, 59, 59));
    for (const auto& e : m_events) {
        if (e.owner != username) continue;
        if (e.startTime <= rangeEnd && e.endTime >= rangeStart)
            result.append(e);
    }
    return result;
}

void JsonStorage::saveEvent(const Event& event) {
    for (int i = 0; i < m_events.size(); ++i) {
        if (m_events[i].id == event.id) {
            m_events[i] = event;
            save();
            return;
        }
    }
    m_events.append(event);
    save();
}

void JsonStorage::deleteEvent(int eventId) {
    m_events.erase(
        std::remove_if(m_events.begin(), m_events.end(),
                       [eventId](const Event& e) { return e.id == eventId; }),
        m_events.end());
    save();
}

int JsonStorage::nextEventId() {
    return m_nextEventId++;
}

// ── 助力 ──

void JsonStorage::addBoost(const BoostLog& boost) {
    m_boosts.append(boost);
    if (auto* u = user(boost.toUser)) {
        u->boostPoints += 10;
    }
    save();
}

QVector<BoostLog> JsonStorage::boostsForUser(const QString& username) const {
    QVector<BoostLog> result;
    for (const auto& b : m_boosts) {
        if (b.toUser == username) result.append(b);
    }
    return result;
}

int JsonStorage::todayBoostCount(const QString& fromUser, const QString& toUser) const {
    const QDate today = QDate::currentDate();
    int count = 0;
    for (const auto& b : m_boosts) {
        if (b.fromUser == fromUser && b.toUser == toUser && b.timestamp.date() == today)
            ++count;
    }
    return count;
}
