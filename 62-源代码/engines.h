#pragma once

#include "domain.h"

#include <QDate>
#include <QObject>
#include <QStringList>
#include <QVector>

class DemoData {
public:
    static QVector<Task> tasks();
    static QVector<TimeBlock> plannedBlocks(const QDate& date);
    static QVector<TimeBlock> actualBlocks(const QDate& date);
    static QVector<GoalNode> goals();
};

class ForensicsEngine {
public:
    ForensicsReport compare(const QDate& date,
                            const QVector<TimeBlock>& planned,
                            const QVector<TimeBlock>& actual) const;

private:
    bool isMatch(const TimeBlock& planned, const TimeBlock& actual) const;
    MatchKind classify(const TimeBlock& planned, const TimeBlock& actual) const;
};

class ChronotypeEngine {
public:
    QVector<double> efficiencyCurve(Category category = Category::Study) const;
    double efficiencyAt(const QTime& time, Category category) const;
    QString profileLabel() const;
    QStringList suggestions(const QVector<TimeBlock>& schedule) const;
};

class AutoScheduler {
public:
    QVector<TimeBlock> schedule(const QVector<Task>& tasks,
                                const QDate& date,
                                const QVector<TimeBlock>& fixed,
                                const ChronotypeEngine& chronotype) const;

private:
    double score(const Task& task, const TimeRange& slot,
                 const ChronotypeEngine& chronotype) const;
    QVector<TimeRange> freeSlots(const QVector<TimeBlock>& fixed) const;
};

class StressPredictor {
public:
    double predict(const QDate& date, const QVector<Task>& tasks) const;
    QString label(double score) const;
    QColor color(double score) const;
};

class QuickParser {
public:
    Task parseTask(const QString& text, int nextId) const;
    TimeBlock parseBlock(const QString& text, int nextId, const QDate& date) const;
};

class PomodoroTimer : public QObject {
    Q_OBJECT
public:
    explicit PomodoroTimer(QObject* parent = nullptr);

    int remainingSeconds() const { return m_remaining; }
    bool running() const { return m_running; }

public slots:
    void startPause();
    void reset();
    void setDuration(int seconds);  // 自定义时长

signals:
    void tick(int seconds);
    void completed();

private:
    class QTimer* m_timer = nullptr;
    int m_remaining = 25 * 60;
    int m_defaultDuration = 25 * 60;
    bool m_running = false;
};
