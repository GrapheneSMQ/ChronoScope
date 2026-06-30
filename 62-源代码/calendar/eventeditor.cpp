#include "eventeditor.h"
#include "core/theme.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

EventEditor::EventEditor(QWidget* parent) : QDialog(parent) {
    buildUi();
    setWindowTitle("新建事件");
}

EventEditor::EventEditor(const Event& event, QWidget* parent) : QDialog(parent) {
    buildUi();
    setWindowTitle("编辑事件");
    applyEvent(event);
}

void EventEditor::buildUi() {
    setMinimumSize(440, 540);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(Spacing::XL, Spacing::XL, Spacing::XL, Spacing::XL);
    root->setSpacing(Spacing::MD);

    auto* title = new QLabel("<h3 style='margin:0'>📅 事件详情</h3>", this);
    root->addWidget(title);

    auto* form = new QFormLayout;
    form->setSpacing(Spacing::LG);
    form->setContentsMargins(0, Spacing::MD, 0, 0);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("事件标题（如：团队周会、复习高数）");
    m_titleEdit->setMinimumHeight(34);
    form->addRow("标题", m_titleEdit);

    m_descEdit = new QPlainTextEdit(this);
    m_descEdit->setPlaceholderText("描述、备注...");
    m_descEdit->setMaximumHeight(80);
    form->addRow("描述", m_descEdit);

    m_locationEdit = new QLineEdit(this);
    m_locationEdit->setPlaceholderText("地点（可选）");
    form->addRow("地点", m_locationEdit);

    m_startEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_startEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_startEdit->setCalendarPopup(true);
    m_startEdit->setMinimumHeight(34);
    form->addRow("开始", m_startEdit);

    m_endEdit = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(3600), this);
    m_endEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_endEdit->setCalendarPopup(true);
    m_endEdit->setMinimumHeight(34);
    form->addRow("结束", m_endEdit);

    m_categoryCombo = new QComboBox(this);
    for (int i = 0; i < 8; ++i)
        m_categoryCombo->addItem(Colors::categoryName(i));
    m_categoryCombo->setMinimumHeight(34);
    form->addRow("类别", m_categoryCombo);

    m_priorityCombo = new QComboBox(this);
    m_priorityCombo->addItems({"低", "普通", "高", "紧急"});
    m_priorityCombo->setCurrentIndex(1);
    m_priorityCombo->setMinimumHeight(34);
    form->addRow("优先级", m_priorityCombo);

    m_goalEdit = new QLineEdit(this);
    m_goalEdit->setPlaceholderText("关联目标（最多10字）");
    m_goalEdit->setMaxLength(10);
    form->addRow("目标", m_goalEdit);

    m_privacyCombo = new QComboBox(this);
    m_privacyCombo->addItems({"公开", "好友可见", "仅显忙碌", "私密"});
    m_privacyCombo->setMinimumHeight(34);
    form->addRow("隐私", m_privacyCombo);

    root->addLayout(form);
    root->addSpacing(Spacing::MD);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText("保存");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

void EventEditor::applyEvent(const Event& event) {
    m_eventId = event.id;
    m_owner = event.owner;
    m_titleEdit->setText(event.title);
    m_descEdit->setPlainText(event.description);
    m_locationEdit->setText(event.location);
    m_startEdit->setDateTime(event.startTime);
    m_endEdit->setDateTime(event.endTime);
    m_categoryCombo->setCurrentIndex(static_cast<int>(event.category));
    m_priorityCombo->setCurrentIndex(static_cast<int>(event.priority) - 1);
    m_goalEdit->setText(event.goal);
    m_privacyCombo->setCurrentIndex(static_cast<int>(event.privacy));
}

Event EventEditor::event() const {
    Event ev;
    ev.id = m_eventId;
    ev.title = m_titleEdit->text().trimmed();
    if (ev.title.isEmpty()) ev.title = "未命名事件";
    ev.description = m_descEdit->toPlainText().trimmed();
    ev.location = m_locationEdit->text().trimmed();
    ev.startTime = m_startEdit->dateTime();
    ev.endTime = m_endEdit->dateTime();
    ev.category = static_cast<Category>(m_categoryCombo->currentIndex());
    ev.priority = static_cast<Priority>(m_priorityCombo->currentIndex() + 1);
    ev.goal = m_goalEdit->text().trimmed();
    ev.privacy = static_cast<PrivacyLevel>(m_privacyCombo->currentIndex());
    ev.owner = m_owner;
    return ev;
}
