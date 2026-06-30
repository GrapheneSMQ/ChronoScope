#pragma once

#include "domain.h"
#include <QComboBox>
#include <QDialog>

// ── 每日签到 / 进度更新 ──
// 每天第一次登录时弹出，询问是否推进年度目标

class DailyCheckInDialog : public QDialog {
    Q_OBJECT
public:
    explicit DailyCheckInDialog(const QVector<GoalNode>& goals, QWidget* parent = nullptr);
    QVector<GoalNode> updatedGoals() const;

private:
    void buildUi(const QVector<GoalNode>& goals);

    struct GoalSlider {
        GoalNode goal;
        QComboBox* combo;
    };
    QVector<GoalSlider> m_sliders;
};
