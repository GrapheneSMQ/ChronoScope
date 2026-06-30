#include "dayview.h"
#include "core/theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QResizeEvent>
#include <QPainterPath>

DayView::DayView(QWidget* parent) : QWidget(parent), m_date(QDate::currentDate()) {
    setMouseTracking(true);
    setMinimumSize(380, TOP_PAD + 24 * HOUR_H + 40);
}

QSize DayView::sizeHint() const { return QSize(560, TOP_PAD + 24 * HOUR_H + 40); }

void DayView::setEvents(const QVector<Event>& events) {
    m_events = events;
    computeLayout();
    update();
}

void DayView::setDate(const QDate& date) {
    m_date = date;
    computeLayout();
    update();
}

void DayView::computeLayout() {
    m_layout.clear();
    const int availW = width() - LEFT_PAD - 16;

    QVector<Event> dayEvents;
    for (const auto& e : m_events) {
        if (e.startTime.date() <= m_date && e.endTime.date() >= m_date)
            dayEvents.append(e);
    }
    std::sort(dayEvents.begin(), dayEvents.end(), [](const Event& a, const Event& b) {
        return a.startTime < b.startTime;
    });

    // 重叠检测（同 WeekView）
    struct Lane { QTime endTime; };
    QVector<Lane> lanes;
    for (const auto& ev : dayEvents) {
        QTime evStart = (ev.startTime.date() < m_date) ? QTime(0, 0) : ev.startTime.time();
        QTime evEnd = (ev.endTime.date() > m_date) ? QTime(23, 59) : ev.endTime.time();

        int laneIdx = -1;
        for (int l = 0; l < lanes.size(); ++l) {
            if (lanes[l].endTime <= evStart) { laneIdx = l; break; }
        }
        if (laneIdx < 0) { laneIdx = lanes.size(); lanes.append(Lane{}); }
        lanes[laneIdx].endTime = evEnd;

        const int totalLanes = lanes.size();
        const double laneW = double(availW - 8) / totalLanes;
        const int x = LEFT_PAD + int(laneIdx * laneW);
        const int w = int(laneW) - 6;

        const double startMin = evStart.hour() * 60 + evStart.minute();
        const double endMin = evEnd.hour() * 60 + evEnd.minute();
        const int y = TOP_PAD + int(startMin / (24.0 * 60) * 24 * HOUR_H);
        const int h = qMax(48, int((endMin - startMin) / (24.0 * 60) * 24 * HOUR_H));

        m_layout.append({ev, QRect(x, y, w, h)});
    }
}

QDateTime DayView::timeFromPos(const QPoint& pos) const {
    if (pos.x() < LEFT_PAD) return {};
    const double hourFrac = double(pos.y() - TOP_PAD) / HOUR_H;
    const int hour = qBound(0, int(hourFrac), 23);
    const int minute = qBound(0, int((hourFrac - hour) * 60), 59);
    return QDateTime(m_date, QTime(hour, minute));
}

void DayView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(Colors::Surface));

    // ── 标题栏 ──
    p.fillRect(QRect(0, 0, width(), TOP_PAD), QColor("#f8fafc"));
    p.setPen(QPen(QColor(Colors::BorderLight), 1));
    p.drawLine(0, TOP_PAD, width(), TOP_PAD);

    // 返回按钮（左侧）
    p.setFont(Fonts::body());
    p.setPen(QColor(Colors::Primary));
    p.drawText(QRect(8, 0, 72, TOP_PAD), Qt::AlignCenter, "← 返回");

    // 日期标题（居中）
    p.setFont(Fonts::section());
    p.setPen(QColor(Colors::TextPrimary));
    p.drawText(QRect(82, 0, width() - 164, TOP_PAD), Qt::AlignCenter,
               m_date.toString("yyyy 年 M 月 d 日  dddd"));

    // ── 时间轴 ──
    const int totalH = 24 * HOUR_H;
    for (int hour = 0; hour < 24; ++hour) {
        const int y = TOP_PAD + hour * HOUR_H;

        // 交替背景（上午/下午微色差）
        if (hour >= 8 && hour < 18) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(248, 250, 252, 80));
            p.drawRect(QRect(LEFT_PAD, y, width() - LEFT_PAD - 8, HOUR_H));
        }

        // 小时标签
        p.setFont(Fonts::body());
        p.setPen(QColor(Colors::TextMuted));
        p.drawText(QRect(4, y - 12, LEFT_PAD - 12, 24), Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1:00").arg(hour, 2, 10, QChar('0')));

        // 网格线
        p.setPen(QPen(QColor(Colors::BorderLight), 1));
        p.drawLine(LEFT_PAD, y, width() - 8, y);
    }

    // ── 当前时间红线 ──
    if (m_date == QDate::currentDate()) {
        QTime now = QTime::currentTime();
        const double nowMin = now.hour() * 60 + now.minute();
        const int nowY = TOP_PAD + int(nowMin / (24.0 * 60) * totalH);
        p.setPen(QPen(QColor(Colors::Danger), 2));
        p.drawLine(LEFT_PAD, nowY, width() - 8, nowY);
        // 红点
        p.setBrush(QColor(Colors::Danger));
        p.drawEllipse(QPointF(LEFT_PAD, nowY), 5, 5);
    }

    // ── 事件块（详情模式：更大更清晰） ──
    for (int i = 0; i < m_layout.size(); ++i) {
        const auto& lb = m_layout[i];
        const bool hovered = (i == m_hoveredBlock);

        QColor catColor = Colors::categoryColor(static_cast<int>(lb.event.category));
        p.setPen(Qt::NoPen);

        // 主体背景
        p.setBrush(catColor.lighter(hovered ? 135 : 148));
        p.drawRoundedRect(lb.rect, Radius::MD, Radius::MD);

        // 左侧色条
        p.setBrush(catColor);
        p.drawRoundedRect(QRect(lb.rect.left(), lb.rect.top() + 4, 5, lb.rect.height() - 8), 3, 3);

        const int pad = 10;
        // 标题
        p.setPen(QColor(Colors::TextPrimary));
        QFont titleFont = Fonts::body();
        titleFont.setBold(true);
        p.setFont(titleFont);
        QFontMetrics tfm(titleFont);
        const QString elidedTitle = tfm.elidedText(lb.event.title, Qt::ElideRight, lb.rect.width() - 20);
        p.drawText(lb.rect.adjusted(pad + 6, 6, -pad, -lb.rect.height() + 24),
                   Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);

        // 时间范围
        p.setPen(catColor.darker(130));
        p.setFont(Fonts::caption());
        p.drawText(lb.rect.adjusted(pad + 6, 24, -pad, -lb.rect.height() + 42),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   lb.event.startTime.toString("HH:mm") + " — " + lb.event.endTime.toString("HH:mm"));

        // 描述（如果空间足够）
        if (lb.rect.height() > 60 && !lb.event.description.isEmpty()) {
            p.setPen(QColor(Colors::TextSecondary));
            p.setFont(Fonts::small());
            QFontMetrics dfm(p.font());
            const QString elidedDesc = dfm.elidedText(lb.event.description, Qt::ElideRight,
                                                       lb.rect.width() - 20);
            p.drawText(lb.rect.adjusted(pad + 6, 42, -pad, -6),
                       Qt::AlignLeft | Qt::AlignTop, elidedDesc);
        }
    }
}

void DayView::mousePressEvent(QMouseEvent* event) {
    // 返回按钮
    if (event->pos().y() < TOP_PAD && event->pos().x() < 80) {
        emit backRequested();
        return;
    }

    // 事件块
    for (int i = m_layout.size() - 1; i >= 0; --i) {
        if (m_layout[i].rect.contains(event->pos())) return;
    }

    // 空白区域
    QDateTime slot = timeFromPos(event->pos());
    if (slot.isValid()) emit slotClicked(slot);
}

void DayView::mouseDoubleClickEvent(QMouseEvent* event) {
    for (int i = m_layout.size() - 1; i >= 0; --i) {
        if (m_layout[i].rect.contains(event->pos())) {
            emit eventDoubleClicked(m_layout[i].event);
            return;
        }
    }
}

void DayView::mouseMoveEvent(QMouseEvent* event) {
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

// ── 自适应尺寸 + 自动滚动 ──

void DayView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    ensureSizeAndScroll();
}

void DayView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    // 宽高变化后重算布局 + 匹配宽度
    computeLayout();
}

void DayView::ensureSizeAndScroll() {
    const int contentH = TOP_PAD + 24 * HOUR_H + 40;
    // 尝试获取所在滚动区域的视口宽度
    int vpW = 500;  // 默认值
    auto* sa = qobject_cast<QScrollArea*>(parentWidget());
    if (sa && sa->viewport()) {
        vpW = sa->viewport()->width();
    }
    resize(qMax(vpW, 380), contentH);

    // 自动滚动：有事件 → 滚动到第一个事件在上半屏；无事件 → 8:00
    int targetY = 8 * HOUR_H;  // 默认 8:00
    if (!m_layout.isEmpty()) {
        // 取第一个事件的 y，上移一个事件高度作为边距
        targetY = m_layout.first().rect.top() - 60;
    }
    targetY = qMax(0, targetY);

    if (sa && sa->verticalScrollBar()) {
        sa->verticalScrollBar()->setValue(targetY);
    }
}
