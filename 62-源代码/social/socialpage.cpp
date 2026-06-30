#include "socialpage.h"
#include "core/storage.h"
#include "core/theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

// ── 通用卡片容器 ──
static QFrame* makeCard(QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setStyleSheet(
        "QFrame { background:#ffffff; border:1px solid #e2e8f0; border-radius:12px; }");
    return card;
}

// ── 带头部的卡片 ──
static QFrame* makeSectionCard(const QString& emoji, const QString& title, QWidget* parent) {
    auto* card = makeCard(parent);
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 14, 16, 14);
    lay->setSpacing(10);
    auto* hdr = new QLabel(QString("<span style='font-size:15px; font-weight:bold; color:#1e293b;'>%1  %2</span>").arg(emoji, title), card);
    lay->addWidget(hdr);
    return card;
}

// ── 列表样式 ──
static const char* LIST_STYLE = R"(
    QListWidget {
        background:#f8fafc;
        border:1px solid #e2e8f0;
        border-radius:10px;
        padding:6px;
        outline:none;
    }
    QListWidget::item {
        background:#ffffff;
        border:2px solid transparent;
        border-radius:8px;
        padding:12px 14px;
        margin:3px 0;
        color:#334155;
        font-size:13px;
        font-weight:500;
    }
    QListWidget::item:hover {
        background:#f1f5f9;
        border:2px solid #cbd5e1;
    }
    QListWidget::item:selected {
        background:#eef2ff;
        border:2px solid #4f46e5;
        color:#1e293b;
        font-weight:bold;
    }
)";

// ── 按钮样式 ──
static const char* PRIMARY_BTN = R"(
    QPushButton {
        background:#4f46e5; color:white; border:none;
        border-radius:8px; padding:9px 16px; font-weight:bold; font-size:13px;
    }
    QPushButton:hover { background:#4338ca; }
)";

static const char* OUTLINE_BTN = R"(
    QPushButton {
        background:transparent; color:#4f46e5;
        border:2px solid #cbd5e1; border-radius:8px;
        padding:8px 14px; font-weight:bold; font-size:13px;
    }
    QPushButton:hover {
        border-color:#4f46e5; background:#eef2ff;
    }
)";

static const char* WARM_BTN = R"(
    QPushButton {
        background:#f59e0b; color:white; border:none;
        border-radius:8px; padding:9px 16px; font-weight:bold; font-size:13px;
    }
    QPushButton:hover { background:#d97706; }
)";

static const char* DANGER_BTN = R"(
    QPushButton {
        background:transparent; color:#ef4444;
        border:2px solid #fecaca; border-radius:8px;
        padding:8px 14px; font-weight:bold; font-size:13px;
    }
    QPushButton:hover {
        background:#fef2f2; border-color:#ef4444;
    }
)";

SocialPage::SocialPage(const QString& currentUser, QWidget* parent)
    : QWidget(parent), m_currentUser(currentUser) {
    buildUi();
    refresh();
}

void SocialPage::buildUi() {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(Spacing::XL, Spacing::XL, Spacing::XL, Spacing::XL);
    root->setSpacing(Spacing::XL);

    // ══════════════════════════════════════
    // 左栏：好友列表
    // ══════════════════════════════════════
    auto* leftCol = new QVBoxLayout;
    leftCol->setSpacing(8);

    // 头部：标题 + 助力值（无白框）
    auto* headerRow = new QHBoxLayout;
    auto* headerTitle = new QLabel("<span style='font-size:16px; font-weight:bold; color:#1e293b;'>👥 好友列表</span>", this);
    headerRow->addWidget(headerTitle);
    headerRow->addStretch();
    m_boostLabel = new QLabel(this);
    m_boostLabel->setStyleSheet(
        "background:#eef2ff; color:#4f46e5; border-radius:20px;"
        "padding:6px 16px; font-size:13px; font-weight:bold;");
    headerRow->addWidget(m_boostLabel);
    leftCol->addLayout(headerRow);

    // 好友列表（主要视觉元素）
    m_friendList = new QListWidget(this);
    m_friendList->setMinimumWidth(200);
    m_friendList->setStyleSheet(LIST_STYLE);
    m_friendList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_friendList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftCol->addWidget(m_friendList, 1);

    // 操作按钮
    auto* btnLay = new QHBoxLayout;
    btnLay->setSpacing(10);

    auto* boostBtn = new QPushButton("⚡ 助力", this);
    boostBtn->setStyleSheet(WARM_BTN);
    boostBtn->setCursor(Qt::PointingHandCursor);
    connect(boostBtn, &QPushButton::clicked, this, [this]() {
        auto* item = m_friendList->currentItem();
        if (!item) { QMessageBox::information(this, "提示", "请先选择一个好友。"); return; }
        sendBoost(item->data(Qt::UserRole).toString());
    });
    btnLay->addWidget(boostBtn);
    btnLay->addStretch();
    leftCol->addLayout(btnLay);

    root->addLayout(leftCol, 1);

    // ── 右栏：发现用户 + 好友请求 ──
    auto* rightCol = new QVBoxLayout;
    rightCol->setSpacing(Spacing::MD);

    // 搜索卡片
    auto* searchCard = makeSectionCard("🔍", "发现用户", this);
    auto* searchInner = qobject_cast<QVBoxLayout*>(searchCard->layout());

    auto* searchRow = new QHBoxLayout;
    searchRow->setSpacing(8);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("输入用户名搜索...");
    m_searchEdit->setMinimumHeight(40);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background:#f8fafc; border:2px solid #e2e8f0; border-radius:8px;"
        "padding:8px 12px; font-size:13px; color:#334155; }"
        "QLineEdit:focus { border-color:#4f46e5; background:#ffffff; }");
    searchRow->addWidget(m_searchEdit, 1);
    auto* searchBtn = new QPushButton("🔍 搜索", this);
    searchBtn->setMinimumHeight(40);
    searchBtn->setStyleSheet(PRIMARY_BTN);
    searchBtn->setCursor(Qt::PointingHandCursor);
    connect(searchBtn, &QPushButton::clicked, this, &SocialPage::searchUsers);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SocialPage::searchUsers);
    searchRow->addWidget(searchBtn);
    searchInner->addLayout(searchRow);

    // 搜索结果列表
    m_searchResults = new QListWidget(this);
    m_searchResults->setMaximumHeight(120);
    m_searchResults->setStyleSheet(LIST_STYLE);
    searchInner->addWidget(m_searchResults);

    // 添加好友按钮
    auto* addFriendBtn = new QPushButton("➕ 添加为好友", this);
    addFriendBtn->setStyleSheet(OUTLINE_BTN);
    addFriendBtn->setCursor(Qt::PointingHandCursor);
    connect(addFriendBtn, &QPushButton::clicked, this, [this]() {
        auto* item = m_searchResults->currentItem();
        if (!item) {
            QMessageBox::information(this, "提示", "请先在搜索结果中点击选中一个用户。");
            return;
        }
        const QString target = item->text();
        // 二次确认弹窗
        QMessageBox confirm(this);
        confirm.setWindowTitle("添加好友");
        confirm.setText(QString("<b>发送好友请求？</b>"));
        confirm.setInformativeText(QString("将向 <span style='color:#4f46e5;font-weight:bold;'>@%1</span> 发送好友请求。\n"
                                          "对方同意后你们即可互相助力。").arg(target));
        confirm.setIcon(QMessageBox::Question);
        auto* yesBtn = confirm.addButton("是的，发送请求", QMessageBox::YesRole);
        yesBtn->setStyleSheet(PRIMARY_BTN);
        auto* noBtn = confirm.addButton("取消", QMessageBox::NoRole);
        noBtn->setStyleSheet(OUTLINE_BTN);
        confirm.setDefaultButton(noBtn);
        confirm.exec();

        if (confirm.clickedButton() == yesBtn) {
            JsonStorage::instance().sendFriendRequest(m_currentUser, target);
            QMessageBox::information(this, "已发送",
                                     QString("✅ 好友请求已发送给 %1！\n等待对方确认。").arg(target));
            refresh();
        }
    });
    searchInner->addWidget(addFriendBtn);
    rightCol->addWidget(searchCard);

    // 好友请求卡片
    auto* reqCard = makeSectionCard("📨", "好友请求", this);
    auto* reqInner = qobject_cast<QVBoxLayout*>(reqCard->layout());

    m_requestList = new QListWidget(this);
    m_requestList->setStyleSheet(LIST_STYLE);
    reqInner->addWidget(m_requestList, 1);

    // 接受/拒绝按钮行
    auto* reqBtnRow = new QHBoxLayout;
    reqBtnRow->setSpacing(10);
    auto* acceptBtn = new QPushButton("✅ 接受", this);
    acceptBtn->setStyleSheet(PRIMARY_BTN);
    acceptBtn->setCursor(Qt::PointingHandCursor);
    connect(acceptBtn, &QPushButton::clicked, this, [this]() {
        auto* item = m_requestList->currentItem();
        if (!item) { QMessageBox::information(this, "提示", "请先选择一个请求。"); return; }
        acceptRequest(item->text());
    });
    reqBtnRow->addWidget(acceptBtn);

    auto* rejectBtn = new QPushButton("❌ 拒绝", this);
    rejectBtn->setStyleSheet(DANGER_BTN);
    rejectBtn->setCursor(Qt::PointingHandCursor);
    connect(rejectBtn, &QPushButton::clicked, this, [this]() {
        auto* item = m_requestList->currentItem();
        if (!item) return;
        const QString from = item->text();
        // 简单拒绝：从好友请求列表移除
        if (auto* profile = JsonStorage::instance().user(m_currentUser)) {
            profile->friendRequests.removeAll(from);
            JsonStorage::instance().updateUser(*profile);
        }
        refresh();
    });
    reqBtnRow->addWidget(rejectBtn);
    reqBtnRow->addStretch();
    reqInner->addLayout(reqBtnRow);

    rightCol->addWidget(reqCard, 1);
    root->addLayout(rightCol, 1);
}

void SocialPage::setUser(const QString& user) {
    m_currentUser = user;
    refresh();
}

void SocialPage::refresh() {
    loadFriends();
    loadRequests();
    auto* profile = JsonStorage::instance().user(m_currentUser);
    if (profile && m_boostLabel) {
        m_boostLabel->setText(QString("⚡  %1 助力值").arg(profile->boostPoints));
    }
}

void SocialPage::searchUsers() {
    const QString query = m_searchEdit->text().trimmed();
    m_searchResults->clear();
    if (query.isEmpty() || query == m_currentUser) return;

    for (const auto& u : JsonStorage::instance().allUsers()) {
        if (u.username.contains(query, Qt::CaseInsensitive) && u.username != m_currentUser)
            m_searchResults->addItem(u.username);
    }
    if (m_searchResults->count() == 0) {
        m_searchResults->addItem("（未找到匹配用户）");
    }
}

void SocialPage::loadFriends() {
    m_friendList->clear();
    auto* profile = JsonStorage::instance().user(m_currentUser);
    if (!profile) return;

    const int listW = m_friendList->viewport()->width() - 20;
    const int rowH = 46;

    for (const auto& f : profile->friends) {
        // 统计该好友给我的助力总数
        int totalBoosts = 0;
        const auto& allBoosts = JsonStorage::instance().boostsForUser(m_currentUser);
        for (const auto& b : allBoosts) {
            if (b.fromUser == f) ++totalBoosts;
        }
        auto* item = new QListWidgetItem();
        item->setData(Qt::UserRole, f);
        m_friendList->addItem(item);

        // 行容器：全透明，融入 item 的白色卡片背景
        auto* row = new QWidget();
        row->setStyleSheet("background:transparent;");
        auto* rowLay = new QHBoxLayout(row);
        rowLay->setContentsMargins(0, 0, 0, 0);
        rowLay->setSpacing(8);
        auto* name = new QLabel(f);
        name->setStyleSheet("font-size:14px; font-weight:bold; color:#1e293b; background:transparent;");
        rowLay->addWidget(name, 1);
        auto* boostLabel = new QLabel(totalBoosts > 0 ? QString("⚡ %1").arg(totalBoosts * 10) : "");
        boostLabel->setStyleSheet("font-size:13px; color:#f59e0b; font-weight:bold; background:transparent;");
        rowLay->addWidget(boostLabel);

        item->setSizeHint(QSize(qMax(listW, 160), rowH));
        m_friendList->setItemWidget(item, row);
    }
    if (profile->friends.isEmpty()) {
        m_friendList->addItem("（还没有好友，去搜索添加吧）");
    }
}

void SocialPage::loadRequests() {
    m_requestList->clear();
    auto* profile = JsonStorage::instance().user(m_currentUser);
    if (!profile) return;
    for (const auto& r : profile->friendRequests)
        m_requestList->addItem(r);
    if (m_requestList->count() == 0) {
        m_requestList->addItem("（暂无待处理请求）");
    }
}

void SocialPage::acceptRequest(const QString& from) {
    JsonStorage::instance().acceptFriendRequest(from, m_currentUser);
    QMessageBox::information(this, "已添加",
                             QString("<b>🎉 你们现在是好友了！</b><br><br>"
                                     "你和 <span style='color:#4f46e5;'>@%1</span> "
                                     "现在可以互相发送助力了。").arg(from));
    refresh();
}

void SocialPage::sendBoost(const QString& to) {
    int count = JsonStorage::instance().todayBoostCount(m_currentUser, to);
    if (count >= 3) {
        QMessageBox::information(this, "已达上限",
                                 QString("今天已经给 %1 助力 3 次啦 ⚡<br>明天再来吧！").arg(to));
        return;
    }

    BoostLog boost;
    boost.fromUser = m_currentUser;
    boost.toUser = to;
    boost.timestamp = QDateTime::currentDateTime();
    boost.message = QString("加油！%1 为你助力 ⚡").arg(m_currentUser);
    JsonStorage::instance().addBoost(boost);

    QMessageBox::information(this, "助力成功",
                             QString("<b>⚡ 助力 +10</b><br><br>"
                                     "已为 <span style='color:#4f46e5;'>@%1</span> 助力！"
                                     "<br>对方获得 10 助力值。").arg(to));
    refresh();
    emit boostSent(to);
}
