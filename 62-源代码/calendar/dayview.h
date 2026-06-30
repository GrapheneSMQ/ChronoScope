#pragma once

#include "domain.h"
#include <QDate>
#include <QScrollArea>
#include <QWidget>

// ── 日视图 ──
// 午夜到午夜完整时间线，可滚动
// 每小时 80px 高，事件完整显示
// 点击空白 → 添加事件；双击事件 → 编辑

class DayView : public QWidget {
    Q_OBJECT
public:
    explicit DayView(QWidget* parent = nullptr);

    void setEvents(const QVector<Event>& events);
    void setDate(const QDate& date);
    QDate date() const { return m_date; }
    QSize sizeHint() const override;

signals:
    void slotClicked(const QDateTime& start);
    void eventDoubleClicked(const Event& event);
    void backRequested();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void showEvent(QShowEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    struct LayoutBlock {
        Event event;
        QRect rect;
    };

    void computeLayout();
    QDateTime timeFromPos(const QPoint& pos) const;
    void ensureSizeAndScroll();

    QDate m_date;
    QVector<Event> m_events;
    QVector<LayoutBlock> m_layout;
    int m_hoveredBlock = -1;

    static constexpr int LEFT_PAD = 70;
    static constexpr int HOUR_H = 80;
    static constexpr int TOP_PAD = 48;  // 加高的标题栏
};
