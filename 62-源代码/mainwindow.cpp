#include "mainwindow.h"
#include "auth/logindialog.h"
#include "auth/registerdialog.h"
#include "calendar/monthview.h"
#include "calendar/weekview.h"
#include "calendar/dayview.h"
#include "calendar/eventeditor.h"
#include "core/storage.h"
#include "core/theme.h"
#include "dialogs.h"
#include "dialogs/dailycheckin.h"
#include "dialogs/goalsetup.h"
#include "social/socialpage.h"

#include <QApplication>
#include <QCalendarWidget>
#include <QCryptographicHash>
#include <QGraphicsOpacityEffect>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(const QString& username, QWidget* parent)
    : QMainWindow(parent), m_currentUser(username), m_date(QDate::currentDate()) {
    m_pomodoro = new PomodoroTimer(this);

    loadUserEvents();
    runOnboarding();
    buildUi();
    applyTheme();
    refreshAll();

    connect(m_pomodoro, &PomodoroTimer::tick, this, &MainWindow::updatePomodoro);
    connect(m_pomodoro, &PomodoroTimer::completed, this, [this]() {
        QMessageBox::information(this, "番茄钟完成", "25 分钟专注已完成！🎉");
    });
}

// ── 数据 ──

void MainWindow::loadUserEvents() {
    m_events = JsonStorage::instance().eventsForUser(m_currentUser);
    // 新用户：空白日程，不注入演示数据
}

void MainWindow::runOnboarding() {
    auto* profile = JsonStorage::instance().user(m_currentUser);
    if (!profile) return;

    const QDate today = QDate::currentDate();
    bool changed = false;

    // 1. 没有目标 → 弹出年度目标设定
    if (profile->goals.isEmpty()) {
        GoalSetupDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            profile->goals = dlg.selectedGoals();
            changed = true;
        }
    }

    // 2. 今天第一次登录 + 有目标 → 每日进度检查
    if (!profile->goals.isEmpty() && profile->lastLoginDate != today) {
        DailyCheckInDialog dlg(profile->goals, this);
        if (dlg.exec() == QDialog::Accepted) {
            profile->goals = dlg.updatedGoals();
            changed = true;
        }
    }

    // 3. 更新最后登录日期
    if (profile->lastLoginDate != today) {
        profile->lastLoginDate = today;
        changed = true;
    }

    if (changed) {
        JsonStorage::instance().updateUser(*profile);
    }
}

// ── UI 构建 ──

void MainWindow::buildUi() {
    setWindowTitle("ChronoScope · 时间透视镜");
    resize(1280, 840);

    // 工具栏
    auto* toolbar = addToolBar("主工具栏");
    auto* addAction = toolbar->addAction("➕ 新建事件");
    auto* schedAction = toolbar->addAction("🤖 自动排程");
    auto* aboutAction = toolbar->addAction("ℹ️ 关于");

    connect(addAction, &QAction::triggered, this, [this]() { openEventEditor(); });
    connect(schedAction, &QAction::triggered, this, &MainWindow::autoSchedule);
    connect(aboutAction, &QAction::triggered, this, [this]() { AboutDialog(this).exec(); });

    // 根布局：侧边栏 + 标签页
    auto* root = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(Spacing::LG, Spacing::LG, Spacing::LG, Spacing::LG);
    rootLayout->setSpacing(Spacing::LG);
    rootLayout->addWidget(buildSidebar(), 0);

    auto* tabs = new QTabWidget(root);
    tabs->addTab(buildCalendarPage(), "📅 日历");
    tabs->addTab(buildGoalPage(), "🎯 目标");
    tabs->addTab(buildSocialPage(), "👥 社交");
    rootLayout->addWidget(tabs, 1);
    setCentralWidget(root);

    statusBar()->showMessage("欢迎回来，" + m_currentUser + "！💪 点击空白时间格即可添加事件");
}

QWidget* MainWindow::buildSidebar() {
    auto* side = new QFrame(this);
    side->setObjectName("Sidebar");
    side->setFixedWidth(260);
    auto* layout = new QVBoxLayout(side);
    layout->setContentsMargins(Spacing::LG, Spacing::LG, Spacing::LG, Spacing::LG);
    layout->setSpacing(Spacing::MD);

    // 用户信息
    auto* profile = JsonStorage::instance().user(m_currentUser);
    m_userLabel = new QLabel(side);
    m_userLabel->setText(QString("<h3 style='margin:0'>👤 %1</h3><p style='color:#64748b;margin:4px 0'>@%2</p>")
                         .arg(profile ? profile->displayName : m_currentUser, m_currentUser));
    layout->addWidget(m_userLabel);

    m_boostLabel = new QLabel(side);
    m_boostLabel->setText(QString("⚡ 助力值：%1").arg(profile ? profile->boostPoints : 0));
    m_boostLabel->setStyleSheet("background:#eef2ff; color:#4f46e5; border-radius:8px; padding:8px; font-weight:bold;");
    layout->addWidget(m_boostLabel);

    // 切换账号按钮
    auto* logoutBtn = new QPushButton("🔄 切换账号", side);
    logoutBtn->setStyleSheet("QPushButton { background:transparent; color:#64748b; border:1px solid #e2e8f0; border-radius:6px; padding:4px 8px; font-size:11px; } QPushButton:hover { background:#fee2e2; color:#dc2626; border-color:#fecaca; }");
    connect(logoutBtn, &QPushButton::clicked, this, [this]() {
        auto reply = QMessageBox::question(this, "切换账号",
            "清除登录凭据并返回登录界面？\n当前账号数据已保存。");
        if (reply == QMessageBox::Yes) {
            LoginDialog::clearCredentials();
            hide();
            // 重新走登录流程
            QString newUser;
            while (newUser.isEmpty()) {
                LoginDialog loginDlg;
                if (loginDlg.exec() == QDialog::Accepted) {
                    newUser = loginDlg.username();
                    break;
                }
                RegisterDialog regDlg;
                if (regDlg.exec() != QDialog::Accepted) {
                    // 用户强制关闭 → 退出
                    QApplication::quit();
                    return;
                }
            }
            // 用新用户重建一切
            m_currentUser = newUser;
            m_events.clear();
            m_tasks.clear();
            m_planned.clear();
            m_actual.clear();
            loadUserEvents();
            runOnboarding();
            refreshAll();
            // 更新侧边栏用户信息
            auto* profile = JsonStorage::instance().user(m_currentUser);
            if (m_userLabel && profile) {
                m_userLabel->setText(QString("<h3 style='margin:0'>👤 %1</h3><p style='color:#64748b;margin:4px 0'>@%2</p>")
                    .arg(profile->displayName, m_currentUser));
            }
            if (m_socialPage) m_socialPage->setUser(m_currentUser);
            show();
            statusBar()->showMessage("已切换至账号：" + m_currentUser, 4000);
        }
    });
    layout->addWidget(logoutBtn);

    // 日历小部件
    auto* calendar = new QCalendarWidget(side);
    calendar->setSelectedDate(m_date);
    calendar->setMaximumHeight(200);
    calendar->setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);
    calendar->setStyleSheet("QCalendarWidget { background:white; border:1px solid #e2e8f0; border-radius:8px; }");
    connect(calendar, &QCalendarWidget::clicked, this, [this](const QDate& date) {
        showWeekView(date);
    });
    layout->addWidget(calendar);

    // 压力预测
    m_pressure = new QLabel(side);
    m_pressure->setStyleSheet("background:white; border:1px solid #e2e8f0; border-radius:8px; padding:10px;");
    layout->addWidget(m_pressure);

    // 目标进度（可滚动区域，夹在压力和番茄钟之间）
    if (profile && !profile->goals.isEmpty()) {
        auto* goalScroll = new QScrollArea(side);
        goalScroll->setFrameShape(QFrame::NoFrame);
        goalScroll->setWidgetResizable(true);
        goalScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        goalScroll->setStyleSheet("QScrollArea { background:transparent; border:none; }");

        auto* goalWidget = new QWidget(goalScroll);
        auto* goalLay = new QVBoxLayout(goalWidget);
        goalLay->setContentsMargins(0, 0, 0, 0);
        goalLay->setSpacing(4);

        for (const auto& goal : profile->goals) {
            auto* row = new QFrame(goalWidget);
            row->setStyleSheet("QFrame { background:white; border:1px solid #e2e8f0; border-radius:6px; }");
            auto* rowLay = new QVBoxLayout(row);
            rowLay->setContentsMargins(10, 6, 10, 6);
            rowLay->setSpacing(3);

            // 第一行：标题 + 百分比
            auto* topRow = new QHBoxLayout;
            auto* name = new QLabel(goal.title, row);
            name->setStyleSheet("font-size:12px; color:#334155; font-weight:bold; border:none;");
            topRow->addWidget(name, 1);
            auto* pct = new QLabel(QString("<b>%1%</b>").arg(int(goal.progress * 100)), row);
            pct->setStyleSheet("font-size:13px; color:#4f46e5; border:none;");
            pct->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            topRow->addWidget(pct);
            rowLay->addLayout(topRow);

            // 第二行：进度条
            auto* bar = new QProgressBar(row);
            bar->setValue(int(goal.progress * 100));
            bar->setTextVisible(false);
            bar->setFixedHeight(6);
            bar->setStyleSheet(
                "QProgressBar { border:none; border-radius:3px; background:#f1f5f9; }"
                "QProgressBar::chunk { background:#4f46e5; border-radius:3px; }");
            rowLay->addWidget(bar);

            goalLay->addWidget(row);
        }
        goalLay->addStretch();
        goalScroll->setWidget(goalWidget);
        layout->addWidget(goalScroll, 1);  // stretch=1, 占满番茄钟以上的剩余空间
    }

    // 番茄钟
    auto* pomoBox = new QFrame(side);
    pomoBox->setObjectName("Panel");
    auto* pLayout = new QVBoxLayout(pomoBox);
    pLayout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);
    pLayout->setSpacing(Spacing::SM);

    auto* pomoTitle = new QLabel("🍅 番茄钟", pomoBox);
    pomoTitle->setStyleSheet("font-weight:bold; color:#334155;");
    pLayout->addWidget(pomoTitle);

    m_pomodoroLabel = new QLabel("25:00", pomoBox);
    QFont pf = m_pomodoroLabel->font();
    pf.setPointSize(24);
    pf.setBold(true);
    m_pomodoroLabel->setFont(pf);
    m_pomodoroLabel->setAlignment(Qt::AlignCenter);
    m_pomodoroLabel->setStyleSheet("color:#4f46e5;");
    m_pomodoroLabel->setCursor(Qt::PointingHandCursor);
    m_pomodoroLabel->setToolTip("点击设置番茄钟时长");
    pLayout->addWidget(m_pomodoroLabel);

    // 点击时间 → 自定义时长
    connect(m_pomodoroLabel, &QLabel::linkActivated, this, [this](const QString&) {}); // dummy
    // QLabel doesn't have clicked signal. Override with event filter or use mousePress.
    // Install event filter on the label for click detection.
    m_pomodoroLabel->installEventFilter(this);

    auto* pBtnRow = new QHBoxLayout;
    auto* startBtn = new QPushButton("▶ 开始", pomoBox);
    auto* resetBtn = new QPushButton("↺ 重置", pomoBox);
    startBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setCursor(Qt::PointingHandCursor);
    startBtn->setStyleSheet("QPushButton { background:#4f46e5; color:white; border:none; border-radius:6px; padding:8px 14px; font-weight:bold; } QPushButton:hover { background:#4338ca; } QPushButton:pressed { background:#3730a3; }");
    resetBtn->setStyleSheet("QPushButton { background:transparent; color:#64748b; border:2px solid #e2e8f0; border-radius:6px; padding:6px 12px; } QPushButton:hover { border-color:#94a3b8; background:#f8fafc; }");
    connect(startBtn, &QPushButton::clicked, this, [this]() {
        m_pomodoro->startPause();
    });
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        m_pomodoro->reset();
    });
    pBtnRow->addWidget(startBtn);
    pBtnRow->addWidget(resetBtn);
    pLayout->addLayout(pBtnRow);
    layout->addWidget(pomoBox);

    layout->addStretch();
    return side;
}

// ── 日历页面（Month → Week → Day 三级导航） ──

QWidget* MainWindow::buildCalendarPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 面包屑导航
    m_calendarBreadcrumb = new QLabel(page);
    m_calendarBreadcrumb->setStyleSheet("color:#64748b; font-size:13px; padding:8px 0;");
    m_calendarBreadcrumb->setText("📅 月视图");
    layout->addWidget(m_calendarBreadcrumb);

    // 日历栈
    m_calendarStack = new QStackedWidget(page);
    m_monthView = new MonthView(m_calendarStack);
    m_weekView = new WeekView(m_calendarStack);
    m_dayView = new DayView(m_calendarStack);

    // MonthView 包裹滚动区域
    auto* monthScroll = new QScrollArea(m_calendarStack);
    monthScroll->setWidgetResizable(true);
    monthScroll->setFrameShape(QFrame::NoFrame);
    monthScroll->setWidget(m_monthView);

    // WeekView 包裹滚动区域
    auto* weekScroll = new QScrollArea(m_calendarStack);
    weekScroll->setWidgetResizable(true);
    weekScroll->setFrameShape(QFrame::NoFrame);
    weekScroll->setWidget(m_weekView);

    // DayView 包裹滚动区域
    auto* dayScroll = new QScrollArea(m_calendarStack);
    dayScroll->setWidgetResizable(true);
    dayScroll->setFrameShape(QFrame::NoFrame);
    dayScroll->setWidget(m_dayView);

    m_calendarStack->addWidget(monthScroll);   // index 0
    m_calendarStack->addWidget(weekScroll);    // index 1
    m_calendarStack->addWidget(dayScroll);     // index 2
    m_calendarStack->setCurrentIndex(0);

    layout->addWidget(m_calendarStack, 1);

    // 信号连接
    connect(m_monthView, &MonthView::dayClicked, this, [this](const QDate& date) {
        showWeekView(date);
    });

    connect(m_weekView, &WeekView::slotClicked, this, &MainWindow::onSlotClicked);
    connect(m_weekView, &WeekView::eventClicked, this, [this](const Event& ev) {
        showDayView(ev.startTime.date());
    });
    connect(m_weekView, &WeekView::dayHeaderClicked, this, &MainWindow::showMonthView);

    connect(m_dayView, &DayView::slotClicked, this, &MainWindow::onSlotClicked);
    connect(m_dayView, &DayView::eventDoubleClicked, this, &MainWindow::onEventDoubleClicked);
    connect(m_dayView, &DayView::backRequested, this, &MainWindow::navigateToWeekFromDay);

    return page;
}

// ── 目标页面 ──

QWidget* MainWindow::buildGoalPage() {
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* page = new QWidget(scroll);
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(Spacing::LG);
    layout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);

    m_goalView = new GoalCascadeView(page);
    connect(m_goalView, &GoalCascadeView::goalClicked, this, [this](int index) {
        auto* profile = JsonStorage::instance().user(m_currentUser);
        if (!profile || index < 0 || index >= profile->goals.size()) return;
        GoalNode& g = profile->goals[index];

        // 编辑弹窗：标题 + 进度
        QDialog dlg(this);
        dlg.setWindowTitle("编辑年度目标");
        auto* dlgLay = new QVBoxLayout(&dlg);
        dlgLay->setContentsMargins(24, 20, 24, 20);
        dlgLay->setSpacing(14);

        auto* nameEdit = new QLineEdit(&dlg);
        nameEdit->setText(g.title);
        nameEdit->setMaxLength(10);
        nameEdit->setPlaceholderText("目标名称（最多10字）");
        nameEdit->setMinimumHeight(36);
        nameEdit->setStyleSheet("QLineEdit { border:2px solid #e2e8f0; border-radius:8px; padding:6px 10px; font-size:14px; } QLineEdit:focus { border-color:#4f46e5; }");
        dlgLay->addWidget(new QLabel("目标名称（≤10字）：", &dlg));
        dlgLay->addWidget(nameEdit);

        auto* pctSpin = new QSpinBox(&dlg);
        pctSpin->setRange(0, 100);
        pctSpin->setValue(int(g.progress * 100));
        pctSpin->setSuffix(" %");
        pctSpin->setMinimumHeight(36);
        pctSpin->setStyleSheet("QSpinBox { border:2px solid #e2e8f0; border-radius:8px; padding:6px 10px; font-size:14px; }");
        dlgLay->addWidget(new QLabel("完成进度：", &dlg));
        dlgLay->addWidget(pctSpin);

        auto* btnRow = new QHBoxLayout;
        auto* saveBtn = new QPushButton("保存", &dlg);
        saveBtn->setStyleSheet("QPushButton { background:#4f46e5; color:white; border:none; border-radius:8px; padding:10px 20px; font-weight:bold; }");
        auto* delBtn = new QPushButton("删除此目标", &dlg);
        delBtn->setStyleSheet("QPushButton { background:transparent; color:#ef4444; border:2px solid #fecaca; border-radius:8px; padding:10px 16px; }");
        auto* cancelBtn = new QPushButton("取消", &dlg);
        cancelBtn->setStyleSheet("QPushButton { background:transparent; color:#64748b; border:2px solid #e2e8f0; border-radius:8px; padding:10px 16px; }");
        btnRow->addWidget(saveBtn);
        btnRow->addWidget(delBtn);
        btnRow->addWidget(cancelBtn);
        dlgLay->addLayout(btnRow);

        connect(saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        connect(delBtn, &QPushButton::clicked, this, [&]() {
            auto reply = QMessageBox::question(&dlg, "确认删除",
                QString("确定要删除目标「%1」吗？").arg(g.title));
            if (reply == QMessageBox::Yes) {
                profile->goals.removeAt(index);
                JsonStorage::instance().updateUser(*profile);
                refreshAll();
                dlg.reject();
            }
        });
        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

        if (dlg.exec() == QDialog::Accepted) {
            QString newTitle = nameEdit->text().trimmed();
            if (newTitle.isEmpty()) {
                QMessageBox::warning(&dlg, "提示", "目标名称不能为空。");
                return;
            }
            if (newTitle.length() > 10) {
                QMessageBox::warning(&dlg, "超出限制",
                    QString("目标名称不能超过10个字符（当前 %1 字）。").arg(newTitle.length()));
                return;
            }
            g.title = newTitle;
            g.progress = pctSpin->value() / 100.0;
            JsonStorage::instance().updateUser(*profile);
            refreshAll();
        }
    });

    // 添加新目标按钮
    auto* addGoalBtn = new QPushButton("➕ 添加年度目标", page);
    addGoalBtn->setStyleSheet("QPushButton { background:transparent; color:#4f46e5; border:2px dashed #cbd5e1; border-radius:10px; padding:10px; font-weight:bold; } QPushButton:hover { border-color:#4f46e5; background:#eef2ff; }");
    addGoalBtn->setCursor(Qt::PointingHandCursor);
    connect(addGoalBtn, &QPushButton::clicked, this, [this]() {
        auto* profile = JsonStorage::instance().user(m_currentUser);
        if (!profile) return;
        QString title;
        while (true) {
            bool ok = false;
            title = QInputDialog::getText(this, "添加年度目标",
                "输入目标名称（最多10字）：", QLineEdit::Normal, "", &ok);
            if (!ok) return; // 用户取消
            title = title.trimmed();
            if (title.isEmpty()) continue;
            if (title.length() > 10) {
                QMessageBox::warning(this, "超出限制",
                    QString("目标名称不能超过10个字符（当前 %1 字）。\n请重新输入。").arg(title.length()));
                continue;
            }
            break;
        }
        GoalNode g;
        g.title = title;
        g.subtitle = "年度目标";
        g.progress = 0.0;
        static const QColor colors[] = {
            QColor("#4f46e5"), QColor("#d97706"), QColor("#7c3aed"),
            QColor("#10b981"), QColor("#0891b2"), QColor("#f59e0b"),
            QColor("#ec4899"), QColor("#06b6d4")
        };
        g.color = colors[profile->goals.size() % 8];
        profile->goals.append(g);
        JsonStorage::instance().updateUser(*profile);
        refreshAll();
    });
    layout->addWidget(addGoalBtn);

    m_heatmap = new HeatmapWidget(page);
    layout->addWidget(m_goalView, 1);
    layout->addWidget(m_heatmap, 1);
    scroll->setWidget(page);
    return scroll;
}

// ── 社交页面 ──

QWidget* MainWindow::buildSocialPage() {
    m_socialPage = new SocialPage(m_currentUser, this);
    connect(m_socialPage, &SocialPage::boostSent, this, [this](const QString&) {
        refreshAll();
    });
    return m_socialPage;
}

// ── 智能分析页面 ──

QWidget* MainWindow::buildAnalyticsPage() {
    auto* page = new QWidget(this);
    auto* layout = new QGridLayout(page);
    layout->setSpacing(Spacing::LG);
    layout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);

    m_curve = new ChronotypeCurve(page);
    m_radar = new RadarChart(page);

    auto* insightBtn = new QPushButton("查看调度建议 💡", page);
    connect(insightBtn, &QPushButton::clicked, this, [this]() {
        InsightDialog("调度建议", m_chronotype.suggestions(m_planned), this).exec();
    });

    layout->addWidget(m_curve, 0, 0, 1, 2);
    layout->addWidget(m_radar, 1, 0);
    layout->addWidget(insightBtn, 1, 1, Qt::AlignTop | Qt::AlignRight);
    return page;
}

// ── 日历导航 ──

void MainWindow::showMonthView() {
    m_calendarBreadcrumb->setText("📅 月视图");
    animateSwitch(m_calendarStack->currentWidget(), m_calendarStack->widget(0), false);
    m_calendarStack->setCurrentIndex(0);
    m_monthView->setEvents(m_events);
}

void MainWindow::showWeekView(const QDate& startDate) {
    m_calendarBreadcrumb->setText(QString("📅 周视图 · %1/%2 起  ∘ 点击日期头返回月视图")
                                  .arg(startDate.month()).arg(startDate.day()));
    animateSwitch(m_calendarStack->currentWidget(), m_calendarStack->widget(1), true);
    m_calendarStack->setCurrentIndex(1);
    m_weekView->setWeekStart(startDate.addDays(1 - startDate.dayOfWeek()));
    m_weekView->setEvents(m_events);
}

void MainWindow::showDayView(const QDate& date) {
    m_calendarBreadcrumb->setText(QString("📅 日视图 · %1/%2  ∘ ← 返回回到周视图")
                                  .arg(date.month()).arg(date.day()));
    animateSwitch(m_calendarStack->currentWidget(), m_calendarStack->widget(2), true);
    m_calendarStack->setCurrentIndex(2);
    m_dayView->setDate(date);
    m_dayView->setEvents(m_events);
}

void MainWindow::navigateToWeekFromDay() {
    showWeekView(m_dayView->date());
}

void MainWindow::animateSwitch(QWidget* from, QWidget* to, bool forward) {
    Q_UNUSED(from); Q_UNUSED(to); Q_UNUSED(forward);
    // 清除旧动画，安全过渡
    if (m_fadeAnim) {
        m_fadeAnim->stop();
        disconnect(m_fadeAnim, nullptr, this, nullptr);
        m_fadeAnim->deleteLater();
        m_fadeAnim = nullptr;
    }
    // QStackedWidget::setCurrentIndex 本身有瞬时切换
    // 复杂度暂简化为无动画切换，确保稳定性
}

// ── 事件处理 ──

void MainWindow::onSlotClicked(const QDateTime& start) {
    openEventEditor(start);
}

void MainWindow::onEventClicked(const Event& event) {
    openEventEditor({}, event);
}

void MainWindow::onEventDoubleClicked(const Event& event) {
    openEventEditor({}, event);
}

void MainWindow::openEventEditor(const QDateTime& slotTime, const Event& existing) {
    EventEditor* editor;
    if (existing.isValid()) {
        editor = new EventEditor(existing, this);
    } else {
        editor = new EventEditor(this);
        if (slotTime.isValid()) {
            Event prefill;
            prefill.startTime = slotTime;
            prefill.endTime = slotTime.addSecs(3600);
            prefill.owner = m_currentUser;
            editor = new EventEditor(prefill, this);
        }
    }

    if (editor->exec() == QDialog::Accepted) {
        Event ev = editor->event();
        if (ev.owner.isEmpty()) ev.owner = m_currentUser;
        if (ev.id <= 0) ev.id = JsonStorage::instance().nextEventId();
        JsonStorage::instance().saveEvent(ev);

        // 也更新到本地的 Task 模型（用于引擎）
        Task task;
        task.id = ev.id;
        task.title = ev.title;
        task.category = ev.category;
        task.priority = ev.priority;
        task.estimatedMinutes = ev.durationMinutes();
        task.deadline = ev.endTime.date();
        task.goal = ev.goal;
        m_tasks.push_back(task);

        loadUserEvents();
        refreshAll();
        statusBar()->showMessage("事件已保存： " + ev.title, 3000);
    }
    delete editor;
}

// ── 自动排程 ──

void MainWindow::autoSchedule() {
    QVector<Task> unscheduled;
    for (const auto& task : m_tasks) {
        bool found = false;
        for (const auto& ev : m_events) {
            if (ev.id == task.id) { found = true; break; }
        }
        if (!found && !task.completed) unscheduled.push_back(task);
    }
    if (unscheduled.isEmpty()) {
        unscheduled = {
            {m_nextTaskId++, "自动：整理报告", Category::Creative, Priority::High, 60, m_date.addDays(1), "完成个人项目"},
            {m_nextTaskId++, "自动：复盘偏离", Category::Life, Priority::Normal, 30, m_date.addDays(1), "保持输入"},
        };
        m_tasks += unscheduled;
    }
    m_planned = m_scheduler.schedule(unscheduled, m_date, m_planned, m_chronotype);

    // 同步到 Event 模型
    for (const auto& block : m_planned) {
        Event ev;
        ev.id = block.task.id;
        ev.title = block.task.title;
        ev.category = block.task.category;
        ev.priority = block.task.priority;
        ev.startTime = QDateTime(block.date, block.start);
        ev.endTime = QDateTime(block.date, block.end);
        ev.owner = m_currentUser;
        JsonStorage::instance().saveEvent(ev);
    }
    loadUserEvents();
    refreshAll();
    statusBar()->showMessage("已根据时间类型画像自动安排 🤖", 3000);
}

// ── 全局刷新 ──

void MainWindow::refreshAll() {
    // 当前视图的事件
    if (m_monthView) m_monthView->setEvents(m_events);
    if (m_weekView) m_weekView->setEvents(m_events);
    if (m_dayView) m_dayView->setEvents(m_events);

    // 智能分析
    if (m_curve) m_curve->setCurve(m_chronotype.efficiencyCurve(Category::Study), m_chronotype.profileLabel());
    if (m_radar) {
        m_radar->setValues({
            {Category::Study, 0.88}, {Category::Work, 0.45}, {Category::Health, 0.24},
            {Category::Social, 0.62}, {Category::Sleep, 0.76}, {Category::Leisure, 0.68}},
            {{Category::Study, 0.70}, {Category::Work, 0.55}, {Category::Health, 0.48},
             {Category::Social, 0.50}, {Category::Sleep, 0.82}, {Category::Leisure, 0.42}});
    }
    auto* profile = JsonStorage::instance().user(m_currentUser);
    if (m_goalView && profile) m_goalView->setGoals(profile->goals);
    if (m_heatmap && profile) m_heatmap->setData(m_events, profile->goals);

    // 压力（基于日程总时长 + 目标完成度）
    double stress = 0.0;
    int totalMinutes = 0;
    const QDate today = QDate::currentDate();
    for (const auto& ev : m_events) {
        if (ev.startTime.date() <= today && ev.endTime.date() >= today)
            totalMinutes += ev.durationMinutes();
    }
    // 日程压力：10小时=50分
    double eventStress = qMin(1.0, totalMinutes / 600.0) * 50.0;
    // 目标压力：未完成比例均值 × 50
    double goalPressure = 0.0;
    if (profile && !profile->goals.isEmpty()) {
        for (const auto& g : profile->goals)
            goalPressure += (1.0 - g.progress);
        goalPressure = goalPressure / profile->goals.size() * 50.0;
    }
    stress = qBound(0.0, eventStress + goalPressure, 100.0);
    QString stressLabel = stress >= 75 ? "极限" : stress >= 50 ? "忙碌" : stress >= 25 ? "适中" : "轻松";
    if (m_pressure) {
        m_pressure->setText(QString("<b>今日压力</b><br>%1 · %2/100")
                            .arg(stressLabel).arg(int(stress)));
    }

    // 助力值
    if (m_boostLabel && profile) {
        m_boostLabel->setText(QString("⚡ 助力值：%1").arg(profile->boostPoints));
    }

    // 社交页
    if (m_socialPage) m_socialPage->refresh();
}

void MainWindow::updatePomodoro(int seconds) {
    const int m = seconds / 60;
    const int s = seconds % 60;
    m_pomodoroLabel->setText(QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')));
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_pomodoroLabel && event->type() == QEvent::MouseButtonPress) {
        QDialog dlg(this);
        dlg.setWindowTitle("设置番茄钟时长");
        auto* lay = new QVBoxLayout(&dlg);
        lay->setContentsMargins(24, 20, 24, 20);
        lay->setSpacing(12);

        auto* hint = new QLabel("自定义番茄钟时长：", &dlg);
        hint->setStyleSheet("font-weight:bold; font-size:14px; color:#1e293b;");
        lay->addWidget(hint);

        auto* spinRow = new QHBoxLayout;
        auto* minSpin = new QSpinBox(&dlg);
        minSpin->setRange(1, 120);
        minSpin->setValue(25);
        minSpin->setSuffix(" 分钟");
        minSpin->setMinimumHeight(36);
        minSpin->setStyleSheet("QSpinBox { border:2px solid #e2e8f0; border-radius:8px; padding:6px 10px; font-size:14px; }");
        spinRow->addWidget(minSpin);

        auto* secSpin = new QSpinBox(&dlg);
        secSpin->setRange(0, 59);
        secSpin->setValue(0);
        secSpin->setSuffix(" 秒");
        secSpin->setMinimumHeight(36);
        secSpin->setStyleSheet("QSpinBox { border:2px solid #e2e8f0; border-radius:8px; padding:6px 10px; font-size:14px; }");
        spinRow->addWidget(secSpin);
        lay->addLayout(spinRow);

        auto* btnRow = new QHBoxLayout;
        auto* okBtn = new QPushButton("确定", &dlg);
        okBtn->setStyleSheet("QPushButton { background:#4f46e5; color:white; border:none; border-radius:8px; padding:10px 20px; font-weight:bold; }");
        auto* cancelBtn = new QPushButton("取消", &dlg);
        cancelBtn->setStyleSheet("QPushButton { background:transparent; color:#64748b; border:2px solid #e2e8f0; border-radius:8px; padding:10px 18px; }");
        btnRow->addWidget(okBtn);
        btnRow->addWidget(cancelBtn);
        lay->addLayout(btnRow);

        connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

        if (dlg.exec() == QDialog::Accepted) {
            int totalSec = minSpin->value() * 60 + secSpin->value();
            totalSec = qBound(10, totalSec, 7200);
            m_pomodoro->setDuration(totalSec);
        }
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

// ── 主题 ──

void MainWindow::applyTheme() {
    setStyleSheet(R"(
        QMainWindow, QWidget {
            background: #f0f2f5; color: #1e293b; font-size: 13px;
        }
        QFrame#Sidebar, QFrame#Panel {
            background: white; border: 1px solid #e2e8f0; border-radius: 10px;
        }
        QTabWidget::pane {
            border: 1px solid #e2e8f0; border-radius: 10px; background: white;
        }
        QTabBar::tab {
            padding: 10px 18px; margin-right: 3px;
            border-top-left-radius: 8px; border-top-right-radius: 8px;
            background: #eef2ff; color: #4f46e5; font-weight: bold;
        }
        QTabBar::tab:selected {
            background: white; color: #1e293b;
        }
        QPushButton {
            background: #4f46e5; color: white; border: 0;
            border-radius: 7px; padding: 8px 14px; font-weight: bold;
        }
        QPushButton:hover { background: #4338ca; }
        QLineEdit, QComboBox, QSpinBox, QDateEdit, QPlainTextEdit {
            background: white; border: 1px solid #cbd5e1;
            border-radius: 6px; padding: 7px;
        }
        QScrollArea { background: transparent; border: none; }
        QToolBar {
            background: white; border-bottom: 1px solid #e2e8f0;
            padding: 4px; spacing: 6px;
        }
        QStatusBar {
            background: white; border-top: 1px solid #e2e8f0;
            color: #64748b;
        }
    )");
}
