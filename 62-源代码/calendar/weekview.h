#pragma once

#include "domain.h"
#include <QDate>
#include <QScrollArea>
#include <QWidget>

// ── 周视图 ──
// 7 列 × 24 小时时间线
// 事件按时间纵向定位，冲突时水平平分列宽
// 点击空白区域 → 添加事件；点击事件 → DayView

class WeekView : public QWidget {
    Q_OBJECT
public:
    explicit WeekView(QWidget* parent = nullptr);

    void setEvents(const QVector<Event>& events);
    void setWeekStart(const QDate& monday);
    QDate weekStart() const { return m_weekStart; }
    QSize sizeHint() const override;

signals:
    void dayHeaderClicked(const QDate& date);   // 点击日期头 → MonthView
    void slotClicked(const QDateTime& start);    // 点击空白时间段
    void eventClicked(const Event& event);       // 点击事件块
    void weekChanged(const QDate& newMonday);    // 翻周

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void showEvent(QShowEvent*) override;

private:
    struct LayoutBlock {
        Event event;
        QRect rect;
    };

    void computeLayout();
    QRect headerRect(int col) const;
    int columnFromPos(const QPoint& pos) const;
    QDateTime timeFromPos(const QPoint& pos) const;
    void navigateWeek(int delta);

    QDate m_weekStart;
    QVector<Event> m_events;
    QVector<LayoutBlock> m_layout;
    int m_hoveredBlock = -1;  // 悬停的事件块索引

    static constexpr int LEFT_AXIS = 62;
    static constexpr int TITLE_BAR_H = 34; // 标题栏（翻页 + 日期范围）
    static constexpr int HEADER_H = 58;    // 日期头部高度
    static constexpr int HOUR_H = 56;      // 每小时行高
    static constexpr int GRID_TOP = TITLE_BAR_H + HEADER_H; // 时间网格起始 y
    static constexpr int TOTAL_H = GRID_TOP + 24 * HOUR_H;
    static constexpr int MIN_COL_W = 82;
};
