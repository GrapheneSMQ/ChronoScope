#include "dialogs.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QVBoxLayout>

TaskEditDialog::TaskEditDialog(QWidget* parent) : QDialog(parent)
{
    buildUi({});
}

TaskEditDialog::TaskEditDialog(const Task& task, QWidget* parent) : QDialog(parent)
{
    buildUi(task);
}

void TaskEditDialog::buildUi(const Task& task)
{
    m_id = task.id;
    setWindowTitle(task.title.isEmpty() ? "添加任务" : "编辑任务");
    resize(420, 280);

    m_title = new QLineEdit(task.title, this);
    m_category = new QComboBox(this);
    const QVector<Category> cats = {Category::Study, Category::Work, Category::Health, Category::Social,
                                    Category::Sleep, Category::Leisure, Category::Creative, Category::Life};
    for (Category cat : cats) {
        m_category->addItem(categoryName(cat), int(cat));
    }
    m_category->setCurrentIndex(std::max(0, m_category->findData(int(task.category))));

    m_priority = new QComboBox(this);
    m_priority->addItem("低", int(Priority::Low));
    m_priority->addItem("普通", int(Priority::Normal));
    m_priority->addItem("高", int(Priority::High));
    m_priority->addItem("紧急", int(Priority::Urgent));
    m_priority->setCurrentIndex(std::max(0, m_priority->findData(int(task.priority))));

    m_minutes = new QSpinBox(this);
    m_minutes->setRange(15, 360);
    m_minutes->setSingleStep(15);
    m_minutes->setValue(task.estimatedMinutes ? task.estimatedMinutes : 60);

    m_deadline = new QDateEdit(task.deadline.isValid() ? task.deadline : QDate::currentDate().addDays(3), this);
    m_deadline->setCalendarPopup(true);

    m_goal = new QLineEdit(task.goal, this);

    auto* form = new QFormLayout;
    form->addRow("标题", m_title);
    form->addRow("类别", m_category);
    form->addRow("优先级", m_priority);
    form->addRow("预计时长", m_minutes);
    form->addRow("截止日期", m_deadline);
    form->addRow("连接目标", m_goal);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

Task TaskEditDialog::task() const
{
    Task t;
    t.id = m_id;
    t.title = m_title->text().trimmed();
    t.category = Category(m_category->currentData().toInt());
    t.priority = Priority(m_priority->currentData().toInt());
    t.estimatedMinutes = m_minutes->value();
    t.deadline = m_deadline->date();
    t.goal = m_goal->text().trimmed();
    return t;
}

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("关于 ChronoScope");
    resize(520, 360);
    auto* layout = new QVBoxLayout(this);
    auto* title = new QLabel("<h2>ChronoScope 时间透视镜</h2>", this);
    auto* body = new QLabel(
        "一个面向大学生日常规划的时间智能工作站。<br><br>"
        "创新点：LCS 时间取证、贪心自动排程、时间类型曲线、生活平衡雷达、压力热力图、目标瀑布。<br><br>"
        "技术栈：C++20 / Qt 6 Widgets / QPainter / 信号槽 / CMake。", this);
    body->setWordWrap(true);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(title);
    layout->addWidget(body);
    layout->addStretch();
    layout->addWidget(buttons);
}

InsightDialog::InsightDialog(const QString& title, const QStringList& insights, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    resize(520, 330);
    auto* layout = new QVBoxLayout(this);
    auto* text = new QPlainTextEdit(this);
    text->setReadOnly(true);
    QString content;
    for (int i = 0; i < insights.size(); ++i) {
        content += QString("%1. %2\n").arg(i + 1).arg(insights[i]);
    }
    text->setPlainText(content);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(text);
    layout->addWidget(buttons);
}
