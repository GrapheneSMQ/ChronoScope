# ChronoScope

ChronoScope（时间透视镜）是一个 C++20 / Qt 6 实现的智能时间分析与生活规划工作站。它不是普通日历，而是把计划、现实、目标、压力和个人效率模式放在同一个可视化工作台中。

## 功能亮点

- 时间块画布：用积木式时间块展示当天计划，支持快速添加任务。
- 时间取证：用 LCS 动态规划匹配“计划 vs 现实”，生成完成率、延迟和偏离洞察。
- 时间类型引擎：根据任务类别和时段估算效率曲线，标出黄金时段和低谷时段。
- 自动排程：用加权贪心算法把待办任务安排到更合适的时间槽。
- 生活平衡雷达：用 QPainter 自绘雷达图展示学习、工作、运动、社交、睡眠、娱乐分配。
- 压力热力图：使用非线性叠加公式预测未来两周的压力指数。
- 目标瀑布：把长期目标、里程碑和今日任务连接起来。
- 番茄钟：内置轻量专注计时器，可用于演示“实际记录”的来源。

## 构建方式

如果 Qt 工具没有加入 PATH，可以直接使用安装器中的 `qt-cmake`：

```bash
QTFRAMEWORK_BYPASS_LICENSE_CHECK=1 \
PATH="$HOME/Qt/Tools/CMake/CMake.app/Contents/bin:$HOME/Qt/Tools/Ninja:$HOME/Qt/6.11.1/macos/bin:$PATH" \
  "$HOME/Qt/6.11.1/macos/bin/qt-cmake" -S . -B build -G Ninja

QTFRAMEWORK_BYPASS_LICENSE_CHECK=1 \
PATH="$HOME/Qt/Tools/CMake/CMake.app/Contents/bin:$HOME/Qt/Tools/Ninja:$HOME/Qt/6.11.1/macos/bin:$PATH" \
  cmake --build build
```

也可以直接用 Qt Creator 打开本目录的 `CMakeLists.txt`。

## 作业说明建议

报告中可以重点讲三个算法：

1. LCS 计划匹配：把时间块序列当作两个序列，用动态规划找最佳对应关系。
2. 贪心自动调度：按优先级排序，为每个任务选择效率、紧迫度、休息间隔综合得分最高的时间段。
3. 压力预测：任务压力不是线性相加，而用 `1 - Π(1 - p_i)` 表示多任务叠加的心理负荷。
