#pragma once

#include <QColor>
#include <QDate>
#include <QDateTime>
#include <QMap>
#include <QString>
#include <QTime>
#include <QVector>

enum class Category {
    Study,
    Work,
    Health,
    Social,
    Sleep,
    Leisure,
    Creative,
    Life
};

enum class Priority {
    Low = 1,
    Normal = 2,
    High = 3,
    Urgent = 4
};

struct TimeRange {
    QTime start;
    QTime end;

    int minutes() const { return start.secsTo(end) / 60; }
    bool isValid() const { return start.isValid() && end.isValid() && start < end; }
};

struct Task {
    int id = 0;
    QString title;
    Category category = Category::Study;
    Priority priority = Priority::Normal;
    int estimatedMinutes = 60;
    QDate deadline;
    QString goal;
    bool completed = false;
    int quality = 4;
};

struct TimeBlock {
    Task task;
    QDate date;
    QTime start;
    QTime end;
    bool actual = false;

    int minutes() const { return start.secsTo(end) / 60; }
    TimeRange range() const { return {start, end}; }
};

enum class MatchKind {
    Matched,
    Delayed,
    Skipped,
    Unplanned,
    Shifted
};

struct BlockMatch {
    TimeBlock planned;
    TimeBlock actual;
    MatchKind kind = MatchKind::Matched;
    int delayMinutes = 0;
};

struct ForensicsReport {
    QDate date;
    QVector<BlockMatch> matches;
    int plannedCount = 0;
    int actualCount = 0;
    int matchedCount = 0;
    int delayedCount = 0;
    int skippedCount = 0;
    int unplannedCount = 0;
    double completionRate = 0.0;
    QStringList insights;
};

struct GoalNode {
    QString title;
    QString subtitle;
    double progress = 0.0;
    QColor color;
};

// ── 新模型：隐私级别 ──
enum class PrivacyLevel {
    Public = 0,      // 所有人可见详情
    FriendsOnly = 1, // 好友可见详情
    BusyOnly = 2,    // 仅显示忙碌
    Private = 3      // 不可见
};

inline QString privacyLabel(PrivacyLevel level) {
    switch (level) {
    case PrivacyLevel::Public: return "公开";
    case PrivacyLevel::FriendsOnly: return "好友可见";
    case PrivacyLevel::BusyOnly: return "仅显忙碌";
    case PrivacyLevel::Private: return "私密";
    }
    return "公开";
}

// ── 统一事件模型 ──
struct Event {
    int id = 0;
    QString title;
    QString description;
    QString location;
    QDateTime startTime;
    QDateTime endTime;
    Category category = Category::Study;
    Priority priority = Priority::Normal;
    QString goal;                    // 关联目标
    PrivacyLevel privacy = PrivacyLevel::Public;
    QString owner;                   // 所属用户名
    bool completed = false;

    int durationMinutes() const {
        return static_cast<int>(startTime.secsTo(endTime) / 60);
    }
    bool isMultiDay() const {
        return startTime.date() != endTime.date();
    }
    bool isValid() const {
        return startTime.isValid() && endTime.isValid() && startTime < endTime;
    }
};

// ── 用户档案 ──
struct UserProfile {
    QString username;
    QString passwordHash;    // 简单 SHA-256
    QString displayName;
    QString bio;
    int boostPoints = 0;
    QStringList friends;     // 好友用户名列表
    QStringList friendRequests; // 待处理好友请求
    QDateTime createdAt;
    QDate lastLoginDate;     // 上次登录日期（用于每日弹窗）
    QVector<GoalNode> goals; // 用户的年度目标
};

// ── 助力记录 ──
struct BoostLog {
    QString fromUser;
    QString toUser;
    QDateTime timestamp;
    QString message;
};

inline QString categoryName(Category category)
{
    switch (category) {
    case Category::Study: return "学习";
    case Category::Work: return "工作";
    case Category::Health: return "运动";
    case Category::Social: return "社交";
    case Category::Sleep: return "睡眠";
    case Category::Leisure: return "娱乐";
    case Category::Creative: return "创意";
    case Category::Life: return "生活";
    }
    return "其他";
}

inline QColor categoryColor(Category category)
{
    switch (category) {
    case Category::Study: return QColor("#2f6fed");
    case Category::Work: return QColor("#5b5ce2");
    case Category::Health: return QColor("#24a148");
    case Category::Social: return QColor("#d97706");
    case Category::Sleep: return QColor("#64748b");
    case Category::Leisure: return QColor("#db2777");
    case Category::Creative: return QColor("#7c3aed");
    case Category::Life: return QColor("#0891b2");
    }
    return QColor("#475569");
}

inline double priorityWeight(Priority priority)
{
    switch (priority) {
    case Priority::Urgent: return 1.0;
    case Priority::High: return 0.74;
    case Priority::Normal: return 0.46;
    case Priority::Low: return 0.24;
    }
    return 0.4;
}

inline int minutesFromDayStart(const QTime& time)
{
    return QTime(0, 0).secsTo(time) / 60;
}

inline QTime timeFromMinutes(int minutes)
{
    minutes = qBound(0, minutes, 23 * 60 + 59);
    return QTime(minutes / 60, minutes % 60);
}
