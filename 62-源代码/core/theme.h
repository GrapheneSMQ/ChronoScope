#pragma once

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QString>

// ── 统一色彩系统 ──
namespace Colors {
    // 背景
    inline constexpr auto Bg        = "#f0f2f5";
    inline constexpr auto Surface   = "#ffffff";
    inline constexpr auto Card      = "#ffffff";
    inline constexpr auto Sidebar   = "#f8f9fb";

    // 文字
    inline constexpr auto TextPrimary   = "#1e293b";
    inline constexpr auto TextSecondary = "#64748b";
    inline constexpr auto TextMuted     = "#94a3b8";

    // 边框
    inline constexpr auto Border      = "#e2e8f0";
    inline constexpr auto BorderLight = "#f1f5f9";

    // 强调色
    inline constexpr auto Primary    = "#4f46e5";  // indigo
    inline constexpr auto PrimaryBg  = "#eef2ff";
    inline constexpr auto Accent     = "#f59e0b";  // amber
    inline constexpr auto Success    = "#10b981";
    inline constexpr auto Danger     = "#ef4444";

    // 类别色
    inline QColor categoryColor(int cat) {
        static const QColor map[] = {
            QColor("#4f46e5"), QColor("#0891b2"), QColor("#10b981"),
            QColor("#f59e0b"), QColor("#64748b"), QColor("#ec4899"),
            QColor("#8b5cf6"), QColor("#06b6d4")
        };
        return (cat >= 0 && cat < 8) ? map[cat] : QColor("#64748b");
    }

    inline QString categoryName(int cat) {
        static const char* map[] = {
            "学习","工作","运动","社交","睡眠","娱乐","创意","生活"
        };
        return (cat >= 0 && cat < 8) ? map[cat] : "其他";
    }
}

// ── 间距系统 (px) ──
namespace Spacing {
    inline constexpr int XS   = 4;
    inline constexpr int SM   = 8;
    inline constexpr int MD   = 12;
    inline constexpr int LG   = 16;
    inline constexpr int XL   = 20;
    inline constexpr int XXL  = 28;
    inline constexpr int XXXL = 36;
}

// ── 圆角 ──
namespace Radius {
    inline constexpr int SM  = 6;
    inline constexpr int MD  = 10;
    inline constexpr int LG  = 14;
    inline constexpr int XL  = 20;
}

// ── 字体工具 ──
namespace Fonts {
    inline QFont title() {
        QFont f; f.setPointSize(14); f.setBold(true); return f;
    }
    inline QFont section() {
        QFont f; f.setPointSize(12); f.setBold(true); return f;
    }
    inline QFont body() {
        QFont f; f.setPointSize(10); return f;
    }
    inline QFont caption() {
        QFont f; f.setPointSize(9); return f;
    }
    inline QFont small() {
        QFont f; f.setPointSize(8); return f;
    }
    inline QFont mono() {
        QFont f("Consolas", 10); return f;
    }
}

// ── 动画时长 (ms) ──
namespace Duration {
    inline constexpr int Fast   = 150;
    inline constexpr int Normal = 300;
    inline constexpr int Slow   = 500;
}
