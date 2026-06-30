#pragma once

#include "domain.h"
#include "engines.h"

#include <QWidget>

class TimeBlockCanvas : public QWidget {
    Q_OBJECT
public:
    explicit TimeBlockCanvas(QWidget* parent = nullptr);
    void setBlocks(const QVector<TimeBlock>& blocks);
    QVector<TimeBlock> blocks() const { return m_blocks; }

signals:
    void blockActivated(const TimeBlock& block);

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QRect blockRect(const TimeBlock& block) const;
    QVector<TimeBlock> m_blocks;
};

class ForensicsDiffView : public QWidget {
    Q_OBJECT
public:
    explicit ForensicsDiffView(QWidget* parent = nullptr);
    void setReport(const ForensicsReport& report);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    ForensicsReport m_report;
};

class RadarChart : public QWidget {
    Q_OBJECT
public:
    explicit RadarChart(QWidget* parent = nullptr);
    void setValues(const QMap<Category, double>& actual, const QMap<Category, double>& target);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QMap<Category, double> m_actual;
    QMap<Category, double> m_target;
};

class ChronotypeCurve : public QWidget {
    Q_OBJECT
public:
    explicit ChronotypeCurve(QWidget* parent = nullptr);
    void setCurve(const QVector<double>& values, const QString& label);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QVector<double> m_values;
    QString m_label;
};

class HeatmapWidget : public QWidget {
    Q_OBJECT
public:
    explicit HeatmapWidget(QWidget* parent = nullptr);
    void setData(const QVector<Event>& events, const QVector<GoalNode>& goals);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QVector<Event> m_events;
    QVector<GoalNode> m_goals;
    double calcStress(const QDate& date) const;
};

class GoalCascadeView : public QWidget {
    Q_OBJECT
public:
    explicit GoalCascadeView(QWidget* parent = nullptr);
    void setGoals(const QVector<GoalNode>& goals);
    QVector<GoalNode> goals() const { return m_goals; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void goalClicked(int index);  // 点击目标，弹出编辑

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void showEvent(QShowEvent*) override;

private:
    int goalIndexAtPos(const QPoint& pos) const;
    int contentHeight() const;
    QRect goalRect(int index) const;
    QVector<GoalNode> m_goals;
    int m_hoveredGoal = -1;
};
