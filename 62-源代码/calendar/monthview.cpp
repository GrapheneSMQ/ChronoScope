#include "monthview.h"
#include "core/theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

MonthView::MonthView(QWidget* parent) : QWidget(parent), m_currentDate(QDate::currentDate())
{
    setMouseTracking(true);
    // 将 m_currentDate 调整到当月第一天
    m_currentDate = QDate(m_currentDate.year(), m_currentDate.month(), 1);
}

QSize MonthView::sizeHint() const { return QSize(720, 580); }
QSize MonthView::minimumSizeHint() const { return QSize(520, 480); }

void MonthView::setEvents(const QVector<Event>& events) {
    m_events = events;
    update();
}

void MonthView::setDate(const QDate& date) {
    m_currentDate = QDate(date.year(), date.month(), 1);
    update();
}

// ── 几何计算 ──

QRect MonthView::cellRect(int row, int col) const {
    const int leftPad = 28;
    const int rightPad = 28;
    const int topPad = 58;    // 标题行高度
    const int headerH = 28;   // 星期头部高度
    const int availW = width() - leftPad - rightPad;
    const int availH = height() - topPad - headerH - 12;
    const int cellW = availW / 7;
    const int cellH = availH / 6;
    const int gap = 4;        // 格子间微间距
    return QRect(leftPad + col * cellW + gap/2,
                 topPad + headerH + row * cellH + gap/2,
                 cellW - gap, cellH - gap);
}

QDate MonthView::cellDate(int row, int col) const {
    // 当月第一天是星期几（1=周一, 7=周日）
    int startDow = m_currentDate.dayOfWeek(); // 1=Mon, 7=Sun
    int dayOffset = row * 7 + col - (startDow - 1);
    return m_currentDate.addDays(dayOffset);
}

int MonthView::rowColFromPos(const QPoint& pos, int& row, int& col) const {
    for (row = 0; row < 6; ++row) {
        for (col = 0; col < 7; ++col) {
            if (cellRect(row, col).contains(pos)) return 0;
        }
    }
    return -1;
}

QVector<Event> MonthView::eventsForCell(const QDate& date) const {
    QVector<Event> result;
    for (const auto& e : m_events) {
        if (e.startTime.date() <= date && e.endTime.date() >= date)
            result.append(e);
    }
    // 按开始时间排序
    std::sort(result.begin(), result.end(), [](const Event& a, const Event& b) {
        return a.startTime < b.startTime;
    });
    return result;
}

// ── 绘制 ──

void MonthView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(Colors::Bg));

    const int leftPad = 28;
    const int rightPad = 28;
    const int topPad = 58;
    const int headerH = 28;
    const int availW = width() - leftPad - rightPad;
    const int cellW = availW / 7;

    // ── 标题栏：年月 + 翻页箭头 ──
    p.setFont(Fonts::title());
    p.setPen(QColor(Colors::TextPrimary));
    const QString monthTitle = m_currentDate.toString("yyyy 年 M 月");
    p.drawText(QRect(leftPad, 0, 280, topPad), Qt::AlignLeft | Qt::AlignVCenter, monthTitle);

    // 翻页箭头
    p.setFont(Fonts::section());
    p.setPen(QColor(Colors::Primary));
    QRect prevBtn(width() - 140, 0, 50, topPad);
    QRect nextBtn(width() - 60, 0, 50, topPad);
    p.drawText(prevBtn, Qt::AlignCenter, "◀");
    p.drawText(nextBtn, Qt::AlignCenter, "▶");

    // ── 星期头 ──
    const QStringList dayNames = {"一", "二", "三", "四", "五", "六", "日"};
    p.setFont(Fonts::caption());
    p.setPen(QColor(Colors::TextMuted));
    for (int col = 0; col < 7; ++col) {
        QRect headerRect(leftPad + col * cellW, topPad, cellW, headerH);
        p.drawText(headerRect, Qt::AlignCenter, dayNames[col]);
    }

    const QDate today = QDate::currentDate();
    const int startDow = m_currentDate.dayOfWeek(); // 1=Mon
    const int cellH = cellRect(0, 0).height();
    const int gap = 4;

    // ── 6 行 × 7 列 日期格 ──
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            QRect cell = cellRect(row, col);
            QDate date = cellDate(row, col);
            const bool isCurrentMonth = (date.month() == m_currentDate.month());
            const bool isToday = (date == today);
            const bool isHovered = (row == m_hoveredRow && col == m_hoveredCol);
            const auto events = eventsForCell(date);

            // 背景
            p.setPen(Qt::NoPen);
            if (isToday) {
                p.setBrush(QColor(Colors::PrimaryBg));
                p.drawRoundedRect(cell, Radius::MD, Radius::MD);
                p.setPen(QPen(QColor(Colors::Primary), 2));
                p.drawRoundedRect(cell, Radius::MD, Radius::MD);
            } else if (isHovered) {
                p.setBrush(QColor("#f1f5f9"));
                p.drawRoundedRect(cell, Radius::MD, Radius::MD);
            } else if (!isCurrentMonth) {
                p.setBrush(QColor("#f8fafc"));
                p.drawRoundedRect(cell, Radius::MD, Radius::MD);
            }

            // 日期数字（紧凑，上方）
            QFont dateFont = Fonts::body();
            if (isToday) dateFont.setBold(true);
            p.setFont(dateFont);
            p.setPen(isCurrentMonth ? QColor(Colors::TextPrimary) : QColor(Colors::TextMuted));
            p.drawText(cell.adjusted(4, 2, -4, -cell.height() + 18), Qt::AlignRight | Qt::AlignTop,
                       QString::number(date.day()));

            // 事件摘要（最多 2 条，15px 行高）
            const int maxVisible = 2;
            const int lineH = 15;
            const int startY = cell.top() + 20;
            for (int i = 0; i < qMin(events.size(), maxVisible); ++i) {
                const auto& ev = events[i];
                QRect lineRect(cell.left() + 5, startY + i * lineH, cell.width() - 10, lineH);

                // 色点（稍小）
                p.setPen(Qt::NoPen);
                p.setBrush(Colors::categoryColor(static_cast<int>(ev.category)));
                p.drawEllipse(QPointF(lineRect.left() + 3.5, lineRect.center().y()), 3, 3);

                // 事件标题（截断）
                p.setPen(QColor(Colors::TextSecondary));
                p.setFont(Fonts::small());
                QFontMetrics fm(p.font());
                const QString elided = fm.elidedText(ev.title, Qt::ElideRight, lineRect.width() - 15);
                p.drawText(lineRect.adjusted(12, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, elided);
            }

            if (events.size() > maxVisible) {
                p.setPen(QColor(Colors::Primary));
                p.setFont(Fonts::small());
                p.drawText(cell.adjusted(5, startY + maxVisible * lineH, -5, lineH),
                           Qt::AlignLeft | Qt::AlignVCenter,
                           QString("+%1 项").arg(events.size() - maxVisible));
            }
        }
    }
}

// ── 交互 ──

void MonthView::mousePressEvent(QMouseEvent* event) {
    int row, col;
    if (rowColFromPos(event->pos(), row, col) == 0) {
        QDate date = cellDate(row, col);
        emit dayClicked(date);
    }

    // 翻页箭头
    const int topPad = 58;
    QRect prevBtn(width() - 140, 0, 50, topPad);
    QRect nextBtn(width() - 60, 0, 50, topPad);
    if (prevBtn.contains(event->pos())) {
        navigateMonth(-1);
    } else if (nextBtn.contains(event->pos())) {
        navigateMonth(1);
    }
}

void MonthView::mouseMoveEvent(QMouseEvent* event) {
    int row, col;
    if (rowColFromPos(event->pos(), row, col) == 0) {
        if (row != m_hoveredRow || col != m_hoveredCol) {
            m_hoveredRow = row;
            m_hoveredCol = col;
            update();
        }
    } else if (m_hoveredRow >= 0) {
        m_hoveredRow = -1;
        m_hoveredCol = -1;
        update();
    }
}

void MonthView::navigateMonth(int delta) {
    m_currentDate = m_currentDate.addMonths(delta);
    emit monthChanged(m_currentDate);
    update();
}
