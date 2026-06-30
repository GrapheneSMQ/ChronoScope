#pragma once

#include "domain.h"
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>

// ── 事件编辑器 ──
// 模态弹窗，最小 420×480
// 所有字段清晰排列，间距充足

class EventEditor : public QDialog {
    Q_OBJECT
public:
    explicit EventEditor(QWidget* parent = nullptr);
    explicit EventEditor(const Event& event, QWidget* parent = nullptr);

    Event event() const;

private:
    void buildUi();
    void applyEvent(const Event& event);

    QLineEdit* m_titleEdit;
    QPlainTextEdit* m_descEdit;
    QLineEdit* m_locationEdit;
    QDateTimeEdit* m_startEdit;
    QDateTimeEdit* m_endEdit;
    QComboBox* m_categoryCombo;
    QComboBox* m_priorityCombo;
    QLineEdit* m_goalEdit;
    QComboBox* m_privacyCombo;

    int m_eventId = -1;
    QString m_owner;
};
