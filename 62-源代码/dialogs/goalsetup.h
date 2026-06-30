#pragma once

#include "domain.h"
#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QVector>

// ── 年度目标设定（首次登录） ──
// 推荐目标可勾选 + "其他" 自定义

class GoalSetupDialog : public QDialog {
    Q_OBJECT
public:
    explicit GoalSetupDialog(QWidget* parent = nullptr);
    QVector<GoalNode> selectedGoals() const;

private:
    void buildUi();

    struct GoalOption {
        QCheckBox* check;
        GoalNode goal;
    };
    QVector<GoalOption> m_options;
    QLineEdit* m_customEdit;
};
