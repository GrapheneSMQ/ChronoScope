#pragma once

#include "domain.h"
#include "engines.h"
#include "visualwidgets.h"

#include <QMainWindow>
#include <QPropertyAnimation>
#include <QStackedWidget>

class QLabel;
class QPushButton;
class QTabWidget;
class MonthView;
class WeekView;
class DayView;
class EventEditor;
class SocialPage;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& username, QWidget* parent = nullptr);

private slots:
    // 日历导航
    void showMonthView();
    void showWeekView(const QDate& startDate);
    void showDayView(const QDate& date);
    void navigateToWeekFromDay();

    // 事件
    void onSlotClicked(const QDateTime& start);
    void onEventClicked(const Event& event);
    void onEventDoubleClicked(const Event& event);
    void openEventEditor(const QDateTime& slotTime = {}, const Event& existing = {});

    // 社交
    // 自动排程
    void autoSchedule();
    void updatePomodoro(int seconds);
    void refreshAll();

private:
    void buildUi();
    void applyTheme();
    bool eventFilter(QObject* obj, QEvent* event) override;
    void animateSwitch(QWidget* from, QWidget* to, bool forward = true);
    void loadUserEvents();
    void runOnboarding();

    // 页面构建
    QWidget* buildSidebar();
    QWidget* buildCalendarPage();
    QWidget* buildGoalPage();
    QWidget* buildSocialPage();
    QWidget* buildAnalyticsPage();

    // 数据
    QString m_currentUser;
    QVector<Event> m_events;
    QVector<Task> m_tasks;
    QVector<TimeBlock> m_planned;
    QVector<TimeBlock> m_actual;
    QDate m_date;
    int m_nextTaskId = 100;

    ForensicsEngine m_forensics;
    ChronotypeEngine m_chronotype;
    AutoScheduler m_scheduler;
    QuickParser m_parser;
    PomodoroTimer* m_pomodoro = nullptr;

    // 日历层级
    QStackedWidget* m_calendarStack = nullptr;
    MonthView* m_monthView = nullptr;
    WeekView* m_weekView = nullptr;
    DayView* m_dayView = nullptr;
    QLabel* m_calendarBreadcrumb = nullptr;

    // 社交
    SocialPage* m_socialPage = nullptr;

    // 保留组件
    RadarChart* m_radar = nullptr;
    ChronotypeCurve* m_curve = nullptr;
    HeatmapWidget* m_heatmap = nullptr;
    GoalCascadeView* m_goalView = nullptr;

    // 侧边栏
    QLabel* m_userLabel = nullptr;
    QLabel* m_boostLabel = nullptr;
    QLabel* m_pressure = nullptr;
    QLabel* m_pomodoroLabel = nullptr;

    // 动画
    QPropertyAnimation* m_fadeAnim = nullptr;
};
