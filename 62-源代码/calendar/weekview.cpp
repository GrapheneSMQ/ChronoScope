#include "weekview.h"
#include "core/theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QWheelEvent>
#include <algorithm>

WeekView::WeekView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    QDate today = QDate::currentDate();
    m_weekStart = today.addDays(1 - today.dayOfWeek());
    setMinimumSize(LEFT_AXIS + 7 * MIN_COL_W + 16, TOTAL_H + 20);
}

QSize WeekView::sizeHint() const { return QSize(800, TOTAL_H + 40); }

void WeekView::setEvents(const QVector<Event>& events) {
    m_events = events;
    computeLayout();
    update();
}

void WeekView::setWeekStart(const QDate& monday) {
    m_weekStart = monday;
    computeLayout();
    update();
}

// ── 几何 ──

QRect WeekView::headerRect(int col) const {
    const int colW = qMax(MIN_COL_W, (width() - LEFT_AXIS - 8) / 7);
    return QRect(LEFT_AXIS + col * colW, TITLE_BAR_H, colW, HEADER_H);
}

int WeekView::columnFromPos(const QPoint& pos) const {
    if (pos.x() < LEFT_AXIS) return -1;
    const int colW = qMax(MIN_COL_W, (width() - LEFT_AXIS - 8) / 7);
    return qBound(0, (pos.x() - LEFT_AXIS) / colW, 6);
}

QDateTime WeekView::timeFromPos(const QPoint& pos) const {
    const int col = columnFromPos(pos);
    if (col < 0) return {};
    const QDate date = m_weekStart.addDays(col);
    const double hourFrac = double(pos.y() - GRID_TOP) / HOUR_H;
    const int hour = qBound(0, int(hourFrac), 23);
    const int minute = qBound(0, int((hourFrac - hour) * 60), 59);
    return QDateTime(date, QTime(hour, minute));
}

// ── 布局计算（重叠事件平分列宽） ──

void WeekView::computeLayout() {
    m_layout.clear();
    const int colW = qMax(MIN_COL_W, (width() > LEFT_AXIS + 100) ? (width() - LEFT_AXIS - 8) / 7 : MIN_COL_W);

    for (int col = 0; col < 7; ++col) {
        QDate date = m_weekStart.addDays(col);
        // 选出当天的所有事件
        QVector<Event> dayEvents;
        for (const auto& e : m_events) {
            if (e.startTime.date() <= date && e.endTime.date() >= date)
                dayEvents.append(e);
        }
        if (dayEvents.isEmpty()) continue;

        // 按开始时间排序
        std::sort(dayEvents.begin(), dayEvents.end(), [](const Event& a, const Event& b) {
            return a.startTime < b.startTime;
        });

        // 贪心分配 lane（解决重叠）
        struct Lane { QTime endTime; };
        QVector<Lane> lanes;

        for (const auto& ev : dayEvents) {
            QTime evStart = (ev.startTime.date() < date) ? QTime(0, 0) : ev.startTime.time();
            QTime evEnd = (ev.endTime.date() > date) ? QTime(23, 59) : ev.endTime.time();

            // 找第一个空闲 lane
            int laneIdx = -1;
            for (int l = 0; l < lanes.size(); ++l) {
                if (lanes[l].endTime <= evStart) {
                    laneIdx = l;
                    break;
                }
            }
            if (laneIdx < 0) {
                laneIdx = lanes.size();
                lanes.append(Lane{});
            }
            lanes[laneIdx].endTime = evEnd;

            const int totalLanes = lanes.size();
            const double laneW = double(colW - 4) / totalLanes;
            const int x = LEFT_AXIS + col * colW + 2 + int(laneIdx * laneW);
            const int w = int(laneW) - 3;

            const double startFrac = (evStart.hour() * 60 + evStart.minute()) / (24.0 * 60);
            const double endFrac = (evEnd.hour() * 60 + evEnd.minute()) / (24.0 * 60);
            const int y = GRID_TOP + int(startFrac * 24 * HOUR_H);
            const int h = qMax(28, int((endFrac - startFrac) * 24 * HOUR_H));

            m_layout.append({ev, QRect(x, y, w, h)});
        }
    }
}

// ── 绘制 ──

void WeekView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(Colors::Surface));

    const int colW = qMax(MIN_COL_W, (width() - LEFT_AXIS - 8) / 7);
    const QDate today = QDate::currentDate();

    // ── 标题栏（独立区域 y:0–TITLE_BAR_H） ──
    p.fillRect(QRect(0, 0, width(), TITLE_BAR_H), QColor("#f8fafc"));
    p.setPen(QPen(QColor(Colors::BorderLight), 1));
    p.drawLine(0, TITLE_BAR_H, width(), TITLE_BAR_H);

    const QDate weekEnd = m_weekStart.addDays(6);
    p.setFont(Fonts::section());
    p.setPen(QColor(Colors::TextPrimary));
    const QString title = QString("%1月%2日 — %3月%4日")
        .arg(m_weekStart.month()).arg(m_weekStart.day())
        .arg(weekEnd.month()).arg(weekEnd.day());
    p.drawText(QRect(LEFT_AXIS + 2, 0, 280, TITLE_BAR_H), Qt::AlignLeft | Qt::AlignVCenter, title);

    // 翻页箭头
    p.setPen(QColor(Colors::Primary));
    p.drawText(QRect(width() - 130, 0, 50, TITLE_BAR_H), Qt::AlignCenter, "◀");
    p.drawText(QRect(width() - 55, 0, 50, TITLE_BAR_H), Qt::AlignCenter, "▶");

    // ── 日期头部（y:TITLE_BAR_H–GRID_TOP） ──
    const QStringList dayNames = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    for (int col = 0; col < 7; ++col) {
        QRect hdr = headerRect(col);
        QDate date = m_weekStart.addDays(col);
        const bool isToday = (date == today);

        // 星期名（上半）
        p.setPen(isToday ? QColor(Colors::Primary) : QColor(Colors::TextMuted));
        p.setFont(Fonts::caption());
        p.drawText(QRect(hdr.left(), hdr.top() + 6, hdr.width(), 18),
                   Qt::AlignCenter, dayNames[col]);

        // 日期号（下半，带 today 高亮）
        QRect numRect(hdr.left() + 3, hdr.top() + 24, hdr.width() - 6, hdr.height() - 28);
        p.setPen(Qt::NoPen);
        if (isToday) {
            p.setBrush(QColor(Colors::PrimaryBg));
            p.drawRoundedRect(numRect, Radius::SM, Radius::SM);
            p.setPen(QPen(QColor(Colors::Primary), 2));
            p.drawRoundedRect(numRect, Radius::SM, Radius::SM);
        }
        QFont dayFont = Fonts::section();
        if (isToday) dayFont.setBold(true);
        p.setFont(dayFont);
        p.setPen(isToday ? QColor(Colors::Primary) : QColor(Colors::TextPrimary));
        p.drawText(numRect, Qt::AlignCenter, QString::number(date.day()));
    }

    // ── 时间轴 + 网格线（从 GRID_TOP 开始） ──
    p.setFont(Fonts::small());
    for (int hour = 0; hour < 24; ++hour) {
        const int y = GRID_TOP + hour * HOUR_H;

        p.setPen(QColor(Colors::TextMuted));
        p.drawText(QRect(4, y - 10, LEFT_AXIS - 10, 20), Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1:00").arg(hour, 2, 10, QChar('0')));

        p.setPen(QPen(QColor(Colors::BorderLight), 1, Qt::DotLine));
        p.drawLine(LEFT_AXIS, y, width() - 8, y);
    }

    // 纵向分隔线
    p.setPen(QPen(QColor(Colors::Border), 1));
    for (int col = 0; col <= 7; ++col) {
        const int x = LEFT_AXIS + col * colW;
        p.drawLine(x, GRID_TOP, x, height());
    }

    // ── 今日列高亮 ──
    int todayCol = today.dayOfWeek() - m_weekStart.dayOfWeek();
    if (todayCol >= 0 && todayCol < 7) {
        QRect todayRect(LEFT_AXIS + todayCol * colW, GRID_TOP, colW, 24 * HOUR_H);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252, 100));
        p.drawRect(todayRect);
    }

    // ── 事件块 ──
    for (int i = 0; i < m_layout.size(); ++i) {
        const auto& lb = m_layout[i];
        const bool hovered = (i == m_hoveredBlock);
        QRect r = lb.rect;

        if (hovered) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 0, 0, 18));
            p.drawRoundedRect(r.adjusted(1, 2, 1, 2), Radius::SM, Radius::SM);
        }

        QColor catColor = Colors::categoryColor(static_cast<int>(lb.event.category));
        p.setPen(Qt::NoPen);
        p.setBrush(catColor.lighter(hovered ? 130 : 145));
        p.drawRoundedRect(r, Radius::SM, Radius::SM);

        p.setBrush(catColor);
        p.drawRoundedRect(QRect(r.left(), r.top() + 3, 4, r.height() - 6), 2, 2);

        p.setPen(QColor(Colors::TextPrimary));
        QFont evFont = Fonts::small();
        evFont.setBold(true);
        p.setFont(evFont);
        QFontMetrics fm(evFont);
        const QString elided = fm.elidedText(lb.event.title, Qt::ElideRight, r.width() - 16);
        p.drawText(r.adjusted(10, 4, -6, -r.height()/2), Qt::AlignLeft | Qt::AlignVCenter, elided);

        p.setPen(catColor.darker(130));
        p.setFont(Fonts::small());
        QString timeStr = lb.event.startTime.toString("HH:mm");
        if (lb.event.durationMinutes() >= 60) {
            timeStr += " - " + lb.event.endTime.toString("HH:mm");
        }
        p.drawText(r.adjusted(10, r.height()/2, -6, -4), Qt::AlignLeft | Qt::AlignVCenter, timeStr);
    }
}

// ── 交互 ──

void WeekView::mousePressEvent(QMouseEvent* event) {
    // 翻页箭头（标题栏区域）
    if (event->pos().y() < TITLE_BAR_H && event->pos().x() > width() - 150) {
        if (event->pos().x() < width() - 95)
            navigateWeek(-1);
        else if (event->pos().x() > width() - 65)
            navigateWeek(1);
        return;
    }

    // 日期头部点击 → MonthView
    if (event->pos().y() >= TITLE_BAR_H && event->pos().y() < GRID_TOP && event->pos().x() > LEFT_AXIS) {
        const int col = columnFromPos(event->pos());
        if (col >= 0 && col < 7) {
            emit dayHeaderClicked(m_weekStart.addDays(col));
            return;
        }
    }

    // 检查事件块点击
    for (int i = m_layout.size() - 1; i >= 0; --i) {
        if (m_layout[i].rect.contains(event->pos())) {
            emit eventClicked(m_layout[i].event);
            return;
        }
    }

    // 空白区域点击 → 添加事件
    QDateTime slotTime = timeFromPos(event->pos());
    if (slotTime.isValid()) {
        emit slotClicked(slotTime);
    }
}

void WeekView::mouseMoveEvent(QMouseEvent* event) {
    int newHovered = -1;
    for (int i = m_layout.size() - 1; i >= 0; --i) {
        if (m_layout[i].rect.contains(event->pos())) {
            newHovered = i;
            break;
        }
    }
    if (newHovered != m_hoveredBlock) {
        m_hoveredBlock = newHovered;
        update();
    }
}

void WeekView::wheelEvent(QWheelEvent* event) {
    navigateWeek(event->angleDelta().y() > 0 ? -1 : 1);
}

void WeekView::navigateWeek(int delta) {
    m_weekStart = m_weekStart.addDays(delta * 7);
    emit weekChanged(m_weekStart);
    computeLayout();
    update();
}

void WeekView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // 匹配父滚动区域宽度，保持自然高度
    const int contentH = TOTAL_H + 20;
    int vpW = 600;
    auto* sa = qobject_cast<QScrollArea*>(parentWidget());
    if (sa && sa->viewport()) vpW = sa->viewport()->width();
    resize(qMax(vpW, minimumWidth()), contentH);

    // 滚动到早上 7:00 附近
    if (sa && sa->verticalScrollBar()) {
        sa->verticalScrollBar()->setValue(GRID_TOP + 7 * HOUR_H);
    }
}
