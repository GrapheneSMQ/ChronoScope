#include "visualwidgets.h"
#include "core/theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QShowEvent>
#include <QtMath>

namespace {
QFont titleFont()
{
    QFont f;
    f.setPointSize(13);
    f.setBold(true);
    return f;
}

void drawRoundedTextBox(QPainter& p, const QRect& rect, const QColor& color,
                        const QString& title, const QString& subtitle)
{
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawRoundedRect(rect, 8, 8);
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect.adjusted(10, 6, -10, -rect.height() / 2), Qt::AlignLeft | Qt::AlignVCenter, title);
    f.setBold(false);
    f.setPointSize(std::max(9, f.pointSize() - 1));
    p.setFont(f);
    p.drawText(rect.adjusted(10, rect.height() / 2 - 4, -10, -6), Qt::AlignLeft | Qt::AlignVCenter, subtitle);
}
}

TimeBlockCanvas::TimeBlockCanvas(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(310);
    setMouseTracking(true);
}

void TimeBlockCanvas::setBlocks(const QVector<TimeBlock>& blocks)
{
    m_blocks = blocks;
    update();
}

QRect TimeBlockCanvas::blockRect(const TimeBlock& block) const
{
    const int leftMargin = 58;
    const int rightMargin = 24;
    const int top = 66;
    const int widthSpan = width() - leftMargin - rightMargin;
    const int dayStart = 8 * 60;
    const int dayMinutes = 14 * 60;
    const int x = leftMargin + (minutesFromDayStart(block.start) - dayStart) * widthSpan / dayMinutes;
    const int w = std::max(46, block.minutes() * widthSpan / dayMinutes);
    const int row = (block.task.id + block.start.hour()) % 3;
    return QRect(x, top + row * 72, std::min(w, width() - x - rightMargin), 54);
}

void TimeBlockCanvas::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f8fafc"));

    p.setFont(titleFont());
    p.setPen(QColor("#0f172a"));
    p.drawText(24, 30, "时间块画布");

    const int leftMargin = 58;
    const int rightMargin = 24;
    const int top = 58;
    const int bottom = height() - 34;
    const int span = width() - leftMargin - rightMargin;

    p.setPen(QPen(QColor("#cbd5e1"), 1));
    for (int hour = 8; hour <= 22; ++hour) {
        const int x = leftMargin + (hour - 8) * span / 14;
        p.drawLine(x, top, x, bottom);
        p.setPen(QColor("#64748b"));
        p.drawText(x - 14, 52, QString("%1:00").arg(hour, 2, 10, QChar('0')));
        p.setPen(QPen(QColor("#cbd5e1"), 1));
    }

    const int goldX = leftMargin;
    const int goldW = 3 * span / 14;
    p.fillRect(QRect(goldX, top, goldW, bottom - top), QColor(250, 204, 21, 35));
    p.setPen(QColor("#92400e"));
    p.drawText(goldX + 8, bottom - 8, "黄金时段");

    p.setPen(QPen(QColor("#94a3b8"), 1));
    p.drawLine(leftMargin, top, width() - rightMargin, top);
    p.drawLine(leftMargin, bottom, width() - rightMargin, bottom);

    for (const auto& block : m_blocks) {
        const QColor color = categoryColor(block.task.category);
        QRect r = blockRect(block);
        drawRoundedTextBox(p, r, color, block.task.title,
                           QString("%1-%2 · %3")
                               .arg(block.start.toString("HH:mm"),
                                    block.end.toString("HH:mm"),
                                    categoryName(block.task.category)));
    }
}

void TimeBlockCanvas::mouseDoubleClickEvent(QMouseEvent* event)
{
    for (const auto& block : m_blocks) {
        if (blockRect(block).contains(event->pos())) {
            emit blockActivated(block);
            return;
        }
    }
}

ForensicsDiffView::ForensicsDiffView(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(360);
}

void ForensicsDiffView::setReport(const ForensicsReport& report)
{
    m_report = report;
    update();
}

void ForensicsDiffView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f8fafc"));
    p.setFont(titleFont());
    p.setPen(QColor("#0f172a"));
    p.drawText(24, 30, "时间取证：计划 vs 现实");

    p.setFont(QFont());
    p.setPen(QColor("#475569"));
    p.drawText(24, 56, QString("完成率 %1% · 延迟 %2 · 跳过 %3 · 计划外 %4")
                         .arg(int(m_report.completionRate * 100))
                         .arg(m_report.delayedCount)
                         .arg(m_report.skippedCount)
                         .arg(m_report.unplannedCount));

    const QRect left(24, 78, width() / 2 - 42, height() - 104);
    const QRect right(width() / 2 + 18, 78, width() / 2 - 42, height() - 104);
    p.setPen(QPen(QColor("#cbd5e1")));
    p.setBrush(Qt::white);
    p.drawRoundedRect(left, 10, 10);
    p.drawRoundedRect(right, 10, 10);
    p.setPen(QColor("#334155"));
    p.drawText(left.adjusted(14, 10, 0, 0), "计划");
    p.drawText(right.adjusted(14, 10, 0, 0), "现实");

    int y = 116;
    for (const auto& match : m_report.matches) {
        QColor color("#22c55e");
        QString badge = "吻合";
        if (match.kind == MatchKind::Delayed) { color = QColor("#f59e0b"); badge = "延迟"; }
        if (match.kind == MatchKind::Skipped) { color = QColor("#ef4444"); badge = "跳过"; }
        if (match.kind == MatchKind::Unplanned) { color = QColor("#db2777"); badge = "计划外"; }
        if (match.kind == MatchKind::Shifted) { color = QColor("#6366f1"); badge = "移动"; }

        p.setPen(color);
        p.setBrush(color.lighter(185));
        p.drawRoundedRect(QRect(width() / 2 - 24, y - 12, 48, 24), 12, 12);
        p.setPen(color.darker(150));
        p.drawText(QRect(width() / 2 - 24, y - 12, 48, 24), Qt::AlignCenter, badge);

        p.setPen(QColor("#334155"));
        if (!match.planned.task.title.isEmpty()) {
            p.drawText(left.adjusted(14, y - 88, -10, 0),
                       QString("%1  %2").arg(match.planned.start.toString("HH:mm"), match.planned.task.title));
        }
        if (!match.actual.task.title.isEmpty()) {
            p.drawText(right.adjusted(14, y - 88, -10, 0),
                       QString("%1  %2").arg(match.actual.start.toString("HH:mm"), match.actual.task.title));
        }
        y += 34;
        if (y > height() - 34) break;
    }
}

RadarChart::RadarChart(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(340, 300);
}

void RadarChart::setValues(const QMap<Category, double>& actual, const QMap<Category, double>& target)
{
    m_actual = actual;
    m_target = target;
    update();
}

void RadarChart::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f8fafc"));
    p.setFont(titleFont());
    p.setPen(QColor("#0f172a"));
    p.drawText(20, 30, "生活平衡雷达");

    const QVector<Category> cats = {Category::Study, Category::Work, Category::Health,
                                    Category::Social, Category::Sleep, Category::Leisure};
    const QPointF c(width() / 2.0, height() / 2.0 + 16);
    const double radius = std::min(width(), height()) * 0.32;

    p.setPen(QPen(QColor("#cbd5e1"), 1));
    for (int ring = 1; ring <= 4; ++ring) {
        QPolygonF poly;
        for (int i = 0; i < cats.size(); ++i) {
            const double a = -M_PI_2 + 2 * M_PI * i / cats.size();
            poly << c + QPointF(std::cos(a), std::sin(a)) * radius * ring / 4.0;
        }
        p.drawPolygon(poly);
    }

    auto polyFor = [&](const QMap<Category, double>& values) {
        QPolygonF poly;
        for (int i = 0; i < cats.size(); ++i) {
            const double a = -M_PI_2 + 2 * M_PI * i / cats.size();
            poly << c + QPointF(std::cos(a), std::sin(a)) * radius * qBound(0.0, values.value(cats[i]), 1.0);
        }
        return poly;
    };

    p.setBrush(QColor(47, 111, 237, 70));
    p.setPen(QPen(QColor("#2f6fed"), 2));
    p.drawPolygon(polyFor(m_actual));
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor("#f97316"), 2, Qt::DashLine));
    p.drawPolygon(polyFor(m_target));

    p.setPen(QColor("#334155"));
    for (int i = 0; i < cats.size(); ++i) {
        const double a = -M_PI_2 + 2 * M_PI * i / cats.size();
        QPointF label = c + QPointF(std::cos(a), std::sin(a)) * (radius + 28);
        p.drawText(QRectF(label.x() - 36, label.y() - 12, 72, 24), Qt::AlignCenter, categoryName(cats[i]));
    }
}

ChronotypeCurve::ChronotypeCurve(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(260);
}

void ChronotypeCurve::setCurve(const QVector<double>& values, const QString& label)
{
    m_values = values;
    m_label = label;
    update();
}

void ChronotypeCurve::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f8fafc"));
    p.setFont(titleFont());
    p.setPen(QColor("#0f172a"));
    p.drawText(20, 30, "时间类型引擎");
    p.setFont(QFont());
    p.setPen(QColor("#475569"));
    p.drawText(20, 54, "画像：" + m_label);

    QRect plot = rect().adjusted(44, 78, -28, -36);
    p.setPen(QPen(QColor("#cbd5e1")));
    for (int i = 0; i <= 4; ++i) {
        const int y = plot.bottom() - i * plot.height() / 4;
        p.drawLine(plot.left(), y, plot.right(), y);
    }
    p.fillRect(QRect(plot.left(), plot.top(), plot.width() * 3 / 14, plot.height()), QColor(250, 204, 21, 35));

    if (m_values.size() >= 24) {
        QPainterPath path;
        for (int hour = 8; hour <= 22; ++hour) {
            const int x = plot.left() + (hour - 8) * plot.width() / 14;
            const int y = plot.bottom() - int(m_values[hour] * plot.height());
            if (hour == 8) path.moveTo(x, y);
            else path.lineTo(x, y);
        }
        p.setPen(QPen(QColor("#2f6fed"), 3));
        p.drawPath(path);
    }

    p.setPen(QColor("#64748b"));
    for (int hour = 8; hour <= 22; hour += 2) {
        const int x = plot.left() + (hour - 8) * plot.width() / 14;
        p.drawText(x - 10, plot.bottom() + 22, QString::number(hour));
    }
}

HeatmapWidget::HeatmapWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(320);
}

void HeatmapWidget::setData(const QVector<Event>& events, const QVector<GoalNode>& goals)
{
    m_events = events;
    m_goals = goals;
    update();
}

double HeatmapWidget::calcStress(const QDate& date) const {
    // 当日事件总分钟
    int totalMinutes = 0;
    for (const auto& ev : m_events) {
        if (ev.startTime.date() <= date && ev.endTime.date() >= date)
            totalMinutes += ev.durationMinutes();
    }
    double eventStress = qMin(1.0, totalMinutes / 600.0) * 50.0;

    double goalPressure = 0.0;
    if (!m_goals.isEmpty()) {
        for (const auto& g : m_goals)
            goalPressure += (1.0 - g.progress);
        goalPressure = goalPressure / m_goals.size() * 50.0;
    }
    return qBound(0.0, eventStress + goalPressure, 100.0);
}

void HeatmapWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f8fafc"));
    p.setFont(titleFont());
    p.setPen(QColor("#0f172a"));
    p.drawText(20, 30, "压力热力图");

    const QStringList names = {"一", "二", "三", "四", "五", "六", "日"};
    const QRect grid = rect().adjusted(32, 70, -32, -48);
    const int cellW = grid.width() / 7;
    const int cellH = grid.height() / 2;
    QDate start = QDate::currentDate();
    start = start.addDays(1 - start.dayOfWeek());
    for (int col = 0; col < 7; ++col) {
        p.setPen(QColor("#64748b"));
        p.drawText(grid.left() + col * cellW, 54, cellW, 18, Qt::AlignCenter, "周" + names[col]);
    }
    for (int i = 0; i < 14; ++i) {
        const int row = i / 7;
        const int col = i % 7;
        const QDate date = start.addDays(i);
        const double score = calcStress(date);

        // 颜色映射
        QColor cellColor = score >= 75 ? QColor("#dc2626") :
                           score >= 50 ? QColor("#f97316") :
                           score >= 25 ? QColor("#facc15") : QColor("#22c55e");

        QRect r(grid.left() + col * cellW + 6, grid.top() + row * cellH + 6, cellW - 12, cellH - 12);
        p.setPen(Qt::NoPen);
        p.setBrush(cellColor.lighter(110));
        p.drawRoundedRect(r, 10, 10);
        p.setPen(Qt::white);
        p.drawText(r.adjusted(6, 6, -6, -r.height() / 2), Qt::AlignLeft | Qt::AlignVCenter,
                   date.toString("M/d"));
        QFont f = p.font();
        f.setBold(true);
        p.setFont(f);

        QString label = score >= 75 ? "极限" : score >= 50 ? "忙碌" : score >= 25 ? "适中" : "轻松";
        p.drawText(r.adjusted(6, r.height() / 2 - 8, -6, -6), Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1 · %2").arg(int(score)).arg(label));
        f.setBold(false);
        p.setFont(f);
    }
}

GoalCascadeView::GoalCascadeView(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
}

int GoalCascadeView::contentHeight() const
{
    const int cardH = 82;
    const int spacing = 14;
    const int count = qMax(1, int(m_goals.size()));
    return 58 + count * (cardH + spacing) + 20;  // 标题 + 卡片们 + 底边距
}

QSize GoalCascadeView::sizeHint() const
{
    return QSize(580, contentHeight());
}

QSize GoalCascadeView::minimumSizeHint() const
{
    return QSize(420, contentHeight());
}

void GoalCascadeView::setGoals(const QVector<GoalNode>& goals)
{
    m_goals = goals;
    setMinimumHeight(contentHeight());
    update();
}

QRect GoalCascadeView::goalRect(int index) const
{
    const int left = 32;
    const int w = width() - 64;
    const int cardH = 82;
    const int spacing = 14;
    const int topY = 56;
    const int y = topY + index * (cardH + spacing);
    return QRect(left, y, w, cardH);
}

int GoalCascadeView::goalIndexAtPos(const QPoint& pos) const
{
    for (int i = 0; i < m_goals.size(); ++i) {
        if (goalRect(i).contains(pos)) return i;
    }
    return -1;
}

void GoalCascadeView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f8fafc"));

    // ── 标题 ──
    p.setFont(titleFont());
    p.setPen(QColor("#0f172a"));
    p.drawText(24, 32, "年度目标");

    if (m_goals.isEmpty()) {
        p.setFont(Fonts::body());
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect().adjusted(0, 80, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                   "还没有设定年度目标");
        return;
    }

    // ── 每个目标一张卡片 ──
    for (int i = 0; i < m_goals.size(); ++i) {
        const auto& goal = m_goals[i];
        QRect card = goalRect(i);
        const bool hovered = (i == m_hoveredGoal);

        // 卡片背景
        p.setPen(QPen(hovered ? QColor("#4f46e5") : QColor("#e2e8f0"),
                      hovered ? 2.5 : 1.5));
        p.setBrush(Qt::white);
        p.drawRoundedRect(card, 12, 12);

        // 左侧色条
        QRect accent(card.left() + 5, card.top() + 12, 6, card.height() - 24);
        p.setPen(Qt::NoPen);
        p.setBrush(goal.color);
        p.drawRoundedRect(accent, 3, 3);

        // 目标标题
        p.setPen(QColor("#1e293b"));
        QFont titleF = p.font();
        titleF.setBold(true);
        titleF.setPointSize(12);
        p.setFont(titleF);
        p.drawText(card.adjusted(22, 10, -120, -card.height()/2),
                   Qt::AlignLeft | Qt::AlignVCenter, goal.title);

        // 副标题
        p.setPen(QColor("#94a3b8"));
        p.setFont(Fonts::caption());
        p.drawText(card.adjusted(22, card.height()/2 - 2, -120, -10),
                   Qt::AlignLeft | Qt::AlignVCenter, goal.subtitle);

        // ── 右侧：百分比 + 进度条 ──
        const int rightW = 100;
        const int rightX = card.right() - rightW - 14;

        // 大百分比数字
        p.setPen(goal.color);
        QFont pctFont = p.font();
        pctFont.setBold(true);
        pctFont.setPointSize(22);
        p.setFont(pctFont);
        const QString pctText = QString("%1%").arg(int(goal.progress * 100));
        p.drawText(QRect(rightX, card.top() + 8, rightW, 30), Qt::AlignRight | Qt::AlignVCenter, pctText);

        // 进度条
        const int barY = card.top() + 44;
        const int barH = 8;
        QRect track(rightX, barY, rightW, barH);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f1f5f9"));
        p.drawRoundedRect(track, 4, 4);

        p.setBrush(goal.color);
        const int fillW = int(track.width() * goal.progress);
        if (fillW > 4) {
            p.drawRoundedRect(QRect(track.left(), track.top(), fillW, track.height()), 4, 4);
        }

        // hover 提示
        if (hovered) {
            p.setPen(QColor("#4f46e5"));
            p.setFont(Fonts::small());
            p.drawText(card.adjusted(0, card.height() - 20, -14, 0),
                       Qt::AlignRight | Qt::AlignVCenter, "点击编辑进度 →");
        }
    }
}

void GoalCascadeView::mousePressEvent(QMouseEvent* event) {
    int idx = goalIndexAtPos(event->pos());
    if (idx >= 0) emit goalClicked(idx);
}

void GoalCascadeView::mouseMoveEvent(QMouseEvent* event) {
    int idx = goalIndexAtPos(event->pos());
    if (idx != m_hoveredGoal) {
        m_hoveredGoal = idx;
        update();
    }
}

void GoalCascadeView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // 向上查找 QScrollArea
    int vpW = 520;
    QWidget* w = parentWidget();
    while (w) {
        if (auto* sa = qobject_cast<QScrollArea*>(w)) {
            if (sa->viewport()) vpW = sa->viewport()->width();
            break;
        }
        w = w->parentWidget();
    }
    resize(qMax(vpW, 420), contentHeight());
}
