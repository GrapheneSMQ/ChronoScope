#include "dailycheckin.h"
#include "core/theme.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DailyCheckInDialog::DailyCheckInDialog(const QVector<GoalNode>& goals, QWidget* parent)
    : QDialog(parent) {
    buildUi(goals);
    setWindowTitle("每日进度检查");
}

QVector<GoalNode> DailyCheckInDialog::updatedGoals() const {
    QVector<GoalNode> result;
    for (const auto& s : m_sliders) {
        GoalNode g = s.goal;
        const QString text = s.combo->currentText();
        int inc = 0;
        if (text.startsWith("+")) {
            inc = text.mid(1).chopped(1).toInt(); // "+5%" → 5
        }
        g.progress = qMin(1.0, g.progress + inc / 100.0);
        result.append(g);
    }
    return result;
}

void DailyCheckInDialog::buildUi(const QVector<GoalNode>& goals) {
    setMinimumSize(420, 120 + goals.size() * 56);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(Spacing::XXL, Spacing::XXL, Spacing::XXL, Spacing::XXL);
    root->setSpacing(Spacing::MD);

    auto* title = new QLabel("<h2 style='margin:0'>🌅 早安！</h2>", this);
    root->addWidget(title);

    auto* hint = new QLabel("新的一天开始了！你的年度目标有进展吗？", this);
    hint->setStyleSheet("color:#64748b; font-size:13px;");
    hint->setWordWrap(true);
    root->addWidget(hint);

    root->addSpacing(Spacing::SM);

    if (goals.isEmpty()) {
        auto* none = new QLabel("暂无年度目标，去「目标」页面添加吧。", this);
        none->setStyleSheet("color:#94a3b8; font-style:italic;");
        root->addWidget(none);
    }

    for (const auto& g : goals) {
        auto* row = new QHBoxLayout;
        row->setSpacing(12);

        // 目标名 + 色条
        auto* dot = new QLabel(this);
        dot->setFixedSize(12, 12);
        dot->setStyleSheet(QString("background:%1; border-radius:6px;").arg(g.color.name()));
        row->addWidget(dot);

        auto* name = new QLabel(QString("%1").arg(g.title), this);
        name->setStyleSheet("font-size:14px; font-weight:bold; color:#334155;");
        row->addWidget(name, 1);

        // 当前进度
        auto* cur = new QLabel(QString("<span style='color:#64748b;'>当前 </span>"
                                       "<span style='color:#4f46e5;font-weight:bold;'>%1%</span>")
                               .arg(int(g.progress * 100)), this);
        cur->setStyleSheet("font-size:13px;");
        row->addWidget(cur);

        // 进度增量选择（5% 步长，上限 = 剩余进度）
        auto* combo = new QComboBox(this);
        const int remaining = qMax(0, int((1.0 - g.progress) * 100));
        combo->addItem("不变");
        const int step5 = 5;
        for (int s = step5; s <= 100; s += step5) {
            if (s <= remaining)
                combo->addItem(QString("+%1%").arg(s));
            else break;
        }
        // 如果剩余不是 5 的倍数，追加精确剩余值
        if (remaining > 0 && remaining % step5 != 0 && remaining < 20) {
            combo->addItem(QString("+%1%").arg(remaining));
        }
        combo->setStyleSheet(
            "QComboBox { border:2px solid #e2e8f0; border-radius:6px; padding:6px 10px;"
            "font-size:13px; color:#334155; min-width:80px; }"
            "QComboBox:hover { border-color:#4f46e5; }"
            "QComboBox::drop-down { border:none; }");
        row->addWidget(combo);

        root->addLayout(row);
        m_sliders.append({g, combo});
    }

    root->addSpacing(Spacing::MD);

    auto* buttons = new QDialogButtonBox(this);
    auto* okBtn = buttons->addButton("✅ 确认更新", QDialogButtonBox::AcceptRole);
    okBtn->setStyleSheet(
        "QPushButton { background:#4f46e5; color:white; border:none; border-radius:8px;"
        "padding:12px 24px; font-weight:bold; font-size:14px; }"
        "QPushButton:hover { background:#4338ca; }");
    okBtn->setCursor(Qt::PointingHandCursor);

    auto* skipBtn = buttons->addButton("今天跳过", QDialogButtonBox::RejectRole);
    skipBtn->setStyleSheet(
        "QPushButton { background:transparent; color:#64748b; border:2px solid #e2e8f0;"
        "border-radius:8px; padding:12px 20px; font-size:13px; }"
        "QPushButton:hover { border-color:#94a3b8; }");

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}
