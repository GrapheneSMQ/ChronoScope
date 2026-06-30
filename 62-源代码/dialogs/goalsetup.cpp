#include "goalsetup.h"
#include "core/theme.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

GoalSetupDialog::GoalSetupDialog(QWidget* parent) : QDialog(parent) {
    buildUi();
    setWindowTitle("设定年度目标");
}

QVector<GoalNode> GoalSetupDialog::selectedGoals() const {
    QVector<GoalNode> result;
    for (const auto& opt : m_options) {
        if (opt.check->isChecked())
            result.append(opt.goal);
    }
    // "其他" 自定义目标
    const QString custom = m_customEdit->text().trimmed();
    if (!custom.isEmpty()) {
        GoalNode g;
        g.title = custom;
        g.subtitle = "自定义目标";
        g.progress = 0.0;
        g.color = QColor("#8b5cf6");  // 紫色
        result.append(g);
    }
    return result;
}

void GoalSetupDialog::buildUi() {
    setMinimumSize(440, 460);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(Spacing::XXL, Spacing::XXL, Spacing::XXL, Spacing::XXL);
    root->setSpacing(Spacing::MD);

    auto* title = new QLabel("<h2 style='margin:0'>🎯 设定你的年度目标</h2>", this);
    root->addWidget(title);

    auto* hint = new QLabel("选择你想在 ChronoScope 中追踪的目标，可以多选。<br>"
                            "所有目标初始进度为 <b>0%</b>，每天登录时可更新进度。", this);
    hint->setStyleSheet("color:#64748b; font-size:13px;");
    hint->setWordWrap(true);
    root->addWidget(hint);

    root->addSpacing(Spacing::SM);

    // 推荐目标列表
    const QVector<QPair<QString, QColor>> recommended = {
        {"GPA 达到 3.8+",      QColor("#4f46e5")},
        {"拿到心仪实习 offer",  QColor("#d97706")},
        {"完成个人项目作品集",   QColor("#7c3aed")},
        {"跑完半程马拉松",      QColor("#10b981")},
        {"每周阅读一本书",      QColor("#0891b2")},
        {"学会一门新技能/语言",  QColor("#f59e0b")},
        {"保持每日运动习惯",    QColor("#ec4899")},
        {"攒下第一笔旅行基金",   QColor("#06b6d4")},
    };

    for (const auto& [text, color] : recommended) {
        auto* cb = new QCheckBox(text, this);
        cb->setStyleSheet(
            "QCheckBox { font-size:14px; color:#334155; padding:6px 0; spacing:10px; }"
            "QCheckBox::indicator { width:20px; height:20px; border-radius:5px;"
            "  border:2px solid #cbd5e1; background:white; }"
            "QCheckBox::indicator:checked { background:#4f46e5; border-color:#4f46e5; }");
        root->addWidget(cb);

        GoalNode g;
        g.title = text;
        g.subtitle = "年度目标";
        g.progress = 0.0;   // 初始 0%
        g.color = color;
        m_options.append({cb, g});
    }

    root->addSpacing(Spacing::SM);

    // "其他" 自定义
    auto* otherLabel = new QLabel("✏️ 自定义目标（选填）：", this);
    otherLabel->setStyleSheet("font-weight:bold; color:#334155;");
    root->addWidget(otherLabel);

    m_customEdit = new QLineEdit(this);
    m_customEdit->setPlaceholderText("输入你自己的年度目标（最多10字）...");
    m_customEdit->setMaxLength(10);
    m_customEdit->setMinimumHeight(38);
    m_customEdit->setStyleSheet(
        "QLineEdit { border:2px solid #e2e8f0; border-radius:8px; padding:8px 12px; font-size:13px; }"
        "QLineEdit:focus { border-color:#4f46e5; }");
    root->addWidget(m_customEdit);

    root->addSpacing(Spacing::MD);

    auto* buttons = new QDialogButtonBox(this);
    auto* okBtn = buttons->addButton("开始追踪 →", QDialogButtonBox::AcceptRole);
    okBtn->setStyleSheet(
        "QPushButton { background:#4f46e5; color:white; border:none; border-radius:8px;"
        "padding:12px 28px; font-weight:bold; font-size:15px; }"
        "QPushButton:hover { background:#4338ca; }");
    okBtn->setCursor(Qt::PointingHandCursor);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (selectedGoals().isEmpty()) {
            QMessageBox::information(this, "提示", "请至少选择一个目标，或填写自定义目标。");
            return;
        }
        accept();
    });
    root->addWidget(buttons);
}
