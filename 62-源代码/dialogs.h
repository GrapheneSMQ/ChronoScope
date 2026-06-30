#pragma once

#include "domain.h"

#include <QDialog>

class QComboBox;
class QDateEdit;
class QLineEdit;
class QSpinBox;

class TaskEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit TaskEditDialog(QWidget* parent = nullptr);
    explicit TaskEditDialog(const Task& task, QWidget* parent = nullptr);
    Task task() const;

private:
    void buildUi(const Task& task);

    QLineEdit* m_title = nullptr;
    QComboBox* m_category = nullptr;
    QComboBox* m_priority = nullptr;
    QSpinBox* m_minutes = nullptr;
    QDateEdit* m_deadline = nullptr;
    QLineEdit* m_goal = nullptr;
    int m_id = 0;
};

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);
};

class InsightDialog : public QDialog {
    Q_OBJECT
public:
    explicit InsightDialog(const QString& title, const QStringList& insights, QWidget* parent = nullptr);
};
