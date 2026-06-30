#pragma once

#include "domain.h"
#include <QDate>
#include <QWidget>

// ── 月视图日历 ──
// 6×7 网格，每格显示日期号 + 最多 3 条事件摘要
// 点击某天 → 进入以该天为起点的 WeekView

class MonthView : public QWidget {
    Q_OBJECT
public:
    explicit MonthView(QWidget* parent = nullptr);

    void setEvents(const QVector<Event>& events);
    void setDate(const QDate& date);         // 设置显示月份
    QDate currentDate() const { return m_currentDate; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void dayClicked(const QDate& date);       // 点击某天
    void monthChanged(const QDate& date);     // 翻月

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

private:
    QRect cellRect(int row, int col) const;
    QDate cellDate(int row, int col) const;
    int rowColFromPos(const QPoint& pos, int& row, int& col) const;
    QVector<Event> eventsForCell(const QDate& date) const;
    void navigateMonth(int delta);

    QDate m_currentDate;            // 当前显示月份的第一天
    QVector<Event> m_events;        // 该月的所有事件
    int m_hoveredRow = -1;
    int m_hoveredCol = -1;
};
