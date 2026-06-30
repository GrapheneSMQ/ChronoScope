#include "engines.h"

#include <QRegularExpression>
#include <QTimer>
#include <algorithm>
#include <cmath>

QVector<Task> DemoData::tasks()
{
    const QDate today = QDate::currentDate();
    return {
        {1, "完善 ChronoScope 大作业", Category::Study, Priority::Urgent, 150, today.addDays(1), "完成个人项目", false, 5},
        {2, "复习高数第六章", Category::Study, Priority::High, 110, today.addDays(5), "GPA 3.8", false, 4},
        {3, "小组讨论录屏脚本", Category::Social, Priority::High, 55, today.addDays(1), "完成个人项目", true, 4},
        {4, "操场跑步 5km", Category::Health, Priority::Normal, 45, today.addDays(3), "半程马拉松", false, 4},
        {5, "LeetCode 两题", Category::Work, Priority::High, 90, today.addDays(4), "实习 offer", false, 3},
        {6, "整理演示素材", Category::Creative, Priority::Normal, 50, today.addDays(2), "完成个人项目", false, 4},
        {7, "阅读二十页", Category::Life, Priority::Low, 40, today.addDays(6), "保持输入", false, 4},
    };
}

QVector<TimeBlock> DemoData::plannedBlocks(const QDate& date)
{
    const auto all = tasks();
    return {
        {all[1], date, QTime(8, 30), QTime(10, 20), false},
        {all[0], date, QTime(10, 40), QTime(12, 20), false},
        {{20, "午饭 + 休息", Category::Life, Priority::Low, 70, date, "", true, 5}, date, QTime(12, 20), QTime(13, 30), false},
        {all[2], date, QTime(14, 0), QTime(14, 55), false},
        {all[3], date, QTime(16, 20), QTime(17, 5), false},
        {all[4], date, QTime(19, 30), QTime(21, 0), false},
    };
}

QVector<TimeBlock> DemoData::actualBlocks(const QDate& date)
{
    const auto all = tasks();
    return {
        {all[1], date, QTime(8, 50), QTime(10, 15), true},
        {{30, "刷短视频", Category::Leisure, Priority::Low, 35, date, "", true, 2}, date, QTime(10, 20), QTime(10, 55), true},
        {all[0], date, QTime(11, 0), QTime(12, 35), true},
        {{31, "午饭 + 休息", Category::Life, Priority::Low, 80, date, "", true, 5}, date, QTime(12, 35), QTime(13, 55), true},
        {all[2], date, QTime(14, 5), QTime(15, 0), true},
        {{32, "补作业截图", Category::Creative, Priority::Normal, 65, date, "完成个人项目", true, 4}, date, QTime(15, 20), QTime(16, 25), true},
        {{33, "打游戏放松", Category::Leisure, Priority::Low, 85, date, "", true, 3}, date, QTime(20, 0), QTime(21, 25), true},
    };
}

QVector<GoalNode> DemoData::goals()
{
    return {
        {"GPA 达到 3.8", "高数/数据结构复习保持稳定推进", 0.66, QColor("#2f6fed")},
        {"完成个人项目", "ChronoScope 进入可录屏状态", 0.72, QColor("#7c3aed")},
        {"拿到实习 offer", "算法题与项目表达共同推进", 0.38, QColor("#d97706")},
        {"跑完半程马拉松", "每周三次跑步，先养成节奏", 0.46, QColor("#24a148")},
    };
}

ForensicsReport ForensicsEngine::compare(const QDate& date,
                                         const QVector<TimeBlock>& planned,
                                         const QVector<TimeBlock>& actual) const
{
    const int n = planned.size();
    const int m = actual.size();
    QVector<QVector<int>> dp(n + 1, QVector<int>(m + 1, 0));

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (isMatch(planned[i - 1], actual[j - 1])) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    QVector<BlockMatch> reversed;
    int i = n;
    int j = m;
    while (i > 0 && j > 0) {
        if (isMatch(planned[i - 1], actual[j - 1])) {
            BlockMatch match;
            match.planned = planned[i - 1];
            match.actual = actual[j - 1];
            match.kind = classify(match.planned, match.actual);
            match.delayMinutes = match.planned.start.secsTo(match.actual.start) / 60;
            reversed.push_back(match);
            --i;
            --j;
        } else if (dp[i - 1][j] >= dp[i][j - 1]) {
            reversed.push_back({planned[i - 1], {}, MatchKind::Skipped, 0});
            --i;
        } else {
            reversed.push_back({{}, actual[j - 1], MatchKind::Unplanned, 0});
            --j;
        }
    }
    while (i > 0) {
        reversed.push_back({planned[i - 1], {}, MatchKind::Skipped, 0});
        --i;
    }
    while (j > 0) {
        reversed.push_back({{}, actual[j - 1], MatchKind::Unplanned, 0});
        --j;
    }

    ForensicsReport report;
    report.date = date;
    report.plannedCount = n;
    report.actualCount = m;
    for (auto it = reversed.crbegin(); it != reversed.crend(); ++it) {
        report.matches.push_back(*it);
        switch (it->kind) {
        case MatchKind::Matched: ++report.matchedCount; break;
        case MatchKind::Delayed: ++report.matchedCount; ++report.delayedCount; break;
        case MatchKind::Shifted: ++report.matchedCount; break;
        case MatchKind::Skipped: ++report.skippedCount; break;
        case MatchKind::Unplanned: ++report.unplannedCount; break;
        }
    }

    report.completionRate = n == 0 ? 1.0 : double(report.matchedCount) / double(n);
    if (report.completionRate < 0.7) {
        report.insights << "计划完成率偏低，今天的计划容量可能超过真实可用精力。";
    }
    if (report.unplannedCount > 0) {
        report.insights << "出现计划外事项，建议在明日计划中预留一段弹性缓冲。";
    }
    if (report.delayedCount > 0) {
        report.insights << "多个任务延迟开始，上午第一个深度任务需要更清晰的启动仪式。";
    }
    if (report.skippedCount > 0) {
        report.insights << "有计划被跳过，优先检查它是否真的连接到当前目标。";
    }
    if (report.insights.isEmpty()) {
        report.insights << "计划与现实吻合度很好，可以复用今天的安排模板。";
    }
    return report;
}

bool ForensicsEngine::isMatch(const TimeBlock& planned, const TimeBlock& actual) const
{
    if (planned.task.title.isEmpty() || actual.task.title.isEmpty()) {
        return false;
    }
    const bool sameCategory = planned.task.category == actual.task.category;
    const bool sameGoal = !planned.task.goal.isEmpty() && planned.task.goal == actual.task.goal;
    const bool similarTitle = planned.task.title.left(2) == actual.task.title.left(2);

    const int start = std::max(minutesFromDayStart(planned.start), minutesFromDayStart(actual.start));
    const int end = std::min(minutesFromDayStart(planned.end), minutesFromDayStart(actual.end));
    const int overlap = std::max(0, end - start);
    const double ratio = double(overlap) / double(std::max(30, std::min(planned.minutes(), actual.minutes())));
    const int startDiff = std::abs(planned.start.secsTo(actual.start) / 60);
    return (sameCategory || sameGoal || similarTitle) && ratio >= 0.35 && startDiff <= 120;
}

MatchKind ForensicsEngine::classify(const TimeBlock& planned, const TimeBlock& actual) const
{
    const int delay = planned.start.secsTo(actual.start) / 60;
    if (std::abs(delay) <= 10) {
        return MatchKind::Matched;
    }
    if (std::abs(delay) <= 45) {
        return MatchKind::Delayed;
    }
    return MatchKind::Shifted;
}

QVector<double> ChronotypeEngine::efficiencyCurve(Category category) const
{
    QVector<double> base(24, 0.42);
    for (int hour = 0; hour < 24; ++hour) {
        const double morning = std::exp(-std::pow((hour - 9.5) / 3.0, 2.0));
        const double evening = std::exp(-std::pow((hour - 20.0) / 2.7, 2.0));
        const double trough = std::exp(-std::pow((hour - 15.5) / 1.8, 2.0));
        base[hour] = 0.36 + 0.48 * morning + 0.22 * evening - 0.20 * trough;
        if (category == Category::Creative) {
            base[hour] += 0.16 * evening;
        }
        if (category == Category::Health && hour >= 16 && hour <= 18) {
            base[hour] += 0.18;
        }
        if (category == Category::Leisure) {
            base[hour] = 0.58;
        }
        base[hour] = qBound(0.18, base[hour], 0.98);
    }
    return base;
}

double ChronotypeEngine::efficiencyAt(const QTime& time, Category category) const
{
    const auto curve = efficiencyCurve(category);
    return curve[qBound(0, time.hour(), 23)];
}

QString ChronotypeEngine::profileLabel() const
{
    return "上午型 + 晚间创意回升";
}

QStringList ChronotypeEngine::suggestions(const QVector<TimeBlock>& schedule) const
{
    QStringList result;
    for (const auto& block : schedule) {
        if ((block.task.category == Category::Study || block.task.category == Category::Work)
            && efficiencyAt(block.start, block.task.category) < 0.5) {
            result << QString("%1 安排在 %2，处于低谷时段；建议移到 08:30-11:30。")
                          .arg(block.task.title, block.start.toString("HH:mm"));
        }
    }
    result << "当前画像显示上午 08:00-11:00 最适合深度任务，16:00 后适合运动和轻量复盘。";
    return result;
}

QVector<TimeBlock> AutoScheduler::schedule(const QVector<Task>& tasks,
                                           const QDate& date,
                                           const QVector<TimeBlock>& fixed,
                                           const ChronotypeEngine& chronotype) const
{
    QVector<Task> sorted = tasks;
    std::sort(sorted.begin(), sorted.end(), [](const Task& a, const Task& b) {
        return priorityWeight(a.priority) * a.estimatedMinutes
            > priorityWeight(b.priority) * b.estimatedMinutes;
    });

    QVector<TimeRange> freeRanges = freeSlots(fixed);
    QVector<TimeBlock> result = fixed;
    for (const auto& task : sorted) {
        int bestIndex = -1;
        double bestScore = -1.0;
        for (int i = 0; i < freeRanges.size(); ++i) {
            if (freeRanges[i].minutes() < task.estimatedMinutes) {
                continue;
            }
            const double s = score(task, freeRanges[i], chronotype);
            if (s > bestScore) {
                bestScore = s;
                bestIndex = i;
            }
        }
        if (bestIndex < 0) {
            continue;
        }
        const QTime start = freeRanges[bestIndex].start;
        const QTime end = start.addSecs(task.estimatedMinutes * 60);
        result.push_back({task, date, start, end, false});
        freeRanges[bestIndex].start = end.addSecs(10 * 60);
        if (!freeRanges[bestIndex].isValid() || freeRanges[bestIndex].minutes() < 25) {
            freeRanges.removeAt(bestIndex);
        }
    }
    std::sort(result.begin(), result.end(), [](const TimeBlock& a, const TimeBlock& b) {
        return a.start < b.start;
    });
    return result;
}

double AutoScheduler::score(const Task& task, const TimeRange& slot,
                            const ChronotypeEngine& chronotype) const
{
    const double fit = chronotype.efficiencyAt(slot.start, task.category);
    const double priority = priorityWeight(task.priority);
    const int days = QDate::currentDate().daysTo(task.deadline);
    const double deadline = days < 0 ? 0.0 : 1.0 / (days + 1.0);
    const double morningBonus = (task.category == Category::Study && slot.start.hour() < 12) ? 0.12 : 0.0;
    return 0.42 * fit + 0.28 * priority + 0.20 * deadline + morningBonus;
}

QVector<TimeRange> AutoScheduler::freeSlots(const QVector<TimeBlock>& fixed) const
{
    QVector<TimeBlock> sorted = fixed;
    std::sort(sorted.begin(), sorted.end(), [](const TimeBlock& a, const TimeBlock& b) {
        return a.start < b.start;
    });
    QVector<TimeRange> freeRanges;
    QTime cursor(8, 0);
    for (const auto& block : sorted) {
        if (cursor.secsTo(block.start) >= 30 * 60) {
            freeRanges.push_back({cursor, block.start});
        }
        if (cursor < block.end) {
            cursor = block.end.addSecs(10 * 60);
        }
    }
    if (cursor < QTime(22, 0)) {
        freeRanges.push_back({cursor, QTime(22, 0)});
    }
    return freeRanges;
}

double StressPredictor::predict(const QDate& date, const QVector<Task>& tasks) const
{
    double rest = 1.0;
    for (const auto& task : tasks) {
        const int daysUntil = date.daysTo(task.deadline);
        if (daysUntil < 0 || daysUntil > 14 || task.completed) {
            continue;
        }
        const double urgency = 1.0 / (daysUntil + 1.0);
        const double difficulty = std::min(1.0, task.estimatedMinutes / 240.0);
        const double stress = qBound(0.0, priorityWeight(task.priority) * urgency * difficulty, 1.0);
        rest *= (1.0 - stress);
    }
    return (1.0 - rest) * 100.0;
}

QString StressPredictor::label(double score) const
{
    if (score >= 75) return "极限";
    if (score >= 52) return "忙碌";
    if (score >= 28) return "适中";
    return "轻松";
}

QColor StressPredictor::color(double score) const
{
    if (score >= 75) return QColor("#dc2626");
    if (score >= 52) return QColor("#f97316");
    if (score >= 28) return QColor("#facc15");
    return QColor("#22c55e");
}

Task QuickParser::parseTask(const QString& text, int nextId) const
{
    Task task;
    task.id = nextId;
    task.title = text.trimmed().isEmpty() ? "未命名任务" : text.trimmed();
    task.estimatedMinutes = text.contains("两小时") || text.contains("2h") ? 120 : 60;
    task.deadline = QDate::currentDate().addDays(text.contains("明天") ? 1 : 3);
    task.priority = text.contains("截止") || text.contains("大作业") ? Priority::High : Priority::Normal;

    if (text.contains("跑") || text.contains("运动")) task.category = Category::Health;
    else if (text.contains("讨论") || text.contains("开会")) task.category = Category::Social;
    else if (text.contains("设计") || text.contains("写作") || text.contains("素材")) task.category = Category::Creative;
    else if (text.contains("玩") || text.contains("游戏")) task.category = Category::Leisure;
    else if (text.contains("复习") || text.contains("作业") || text.contains("程设")) task.category = Category::Study;
    else task.category = Category::Life;
    return task;
}

TimeBlock QuickParser::parseBlock(const QString& text, int nextId, const QDate& date) const
{
    Task task = parseTask(text, nextId);
    QRegularExpression re("(\\d{1,2})[:点](\\d{1,2})?");
    QRegularExpressionMatch match = re.match(text);
    QTime start(18, 0);
    if (match.hasMatch()) {
        start = QTime(match.captured(1).toInt(), match.captured(2).isEmpty() ? 0 : match.captured(2).toInt());
        if (text.contains("下午") && start.hour() < 12) {
            start = start.addSecs(12 * 3600);
        }
    }
    return {task, date, start, start.addSecs(task.estimatedMinutes * 60), false};
}

PomodoroTimer::PomodoroTimer(QObject* parent) : QObject(parent), m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (m_remaining > 0) {
            --m_remaining;
            emit tick(m_remaining);
            return;
        }
        m_timer->stop();
        m_running = false;
        m_remaining = m_defaultDuration;
        emit completed();
        emit tick(m_remaining);
    });
}

void PomodoroTimer::startPause()
{
    m_running = !m_running;
    if (m_running) {
        m_timer->start(1000);
    } else {
        m_timer->stop();
    }
}

void PomodoroTimer::reset()
{
    m_timer->stop();
    m_running = false;
    m_remaining = m_defaultDuration;
    emit tick(m_remaining);
}

void PomodoroTimer::setDuration(int seconds)
{
    seconds = qBound(10, seconds, 7200); // 10秒 ~ 2小时
    m_defaultDuration = seconds;
    if (!m_running) {
        m_remaining = seconds;
        emit tick(m_remaining);
    }
}
