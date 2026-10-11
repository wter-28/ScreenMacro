#pragma once

#include <QPoint>
#include <QString>
#include <QThread>
#include <QVector>

#include <atomic>

// 单步的鼠标动作
enum class StepAction
{
    LeftClick,
    RightClick,
    DoubleClick
};

// 一个宏步骤：把鼠标移到 pos（物理像素坐标）执行动作，然后等待 waitSec 秒
struct MacroStep
{
    QPoint pos;
    double waitSec = 1.0;
    StepAction action = StepAction::LeftClick;
    int repeatCount = 1; // 该步骤单独重复的次数
};

// 界面显示用的中文名
inline QString stepActionLabel(StepAction action)
{
    switch (action) {
    case StepAction::RightClick:
        return QStringLiteral("右键单击");
    case StepAction::DoubleClick:
        return QStringLiteral("左键双击");
    case StepAction::LeftClick:
        break;
    }
    return QStringLiteral("左键单击");
}

// 存档用的稳定标识
inline QString stepActionKey(StepAction action)
{
    switch (action) {
    case StepAction::RightClick:
        return QStringLiteral("right");
    case StepAction::DoubleClick:
        return QStringLiteral("double");
    case StepAction::LeftClick:
        break;
    }
    return QStringLiteral("left");
}

inline StepAction stepActionFromKey(const QString &key)
{
    if (key == QLatin1String("right")) {
        return StepAction::RightClick;
    }
    if (key == QLatin1String("double")) {
        return StepAction::DoubleClick;
    }
    return StepAction::LeftClick;
}

// 在独立线程里顺序回放点击脚本，可随时中断
class MacroPlayer : public QThread
{
    Q_OBJECT

public:
    explicit MacroPlayer(QObject *parent = nullptr);

    void setScript(const QVector<MacroStep> &steps, int loops, int startupDelaySec);
    void requestStop();

signals:
    void statusChanged(const QString &text);
    // aborted 为 true 表示被用户中途停止，false 表示正常跑完
    void runFinished(bool aborted);

protected:
    void run() override;

private:
    // 睡眠 ms 毫秒，但每 20ms 检查一次停止标志；返回 false 表示被中断
    bool interruptibleSleep(int ms);
    static void performAction(const MacroStep &step);

    QVector<MacroStep> m_steps;
    int m_loops = 1;
    int m_startupDelaySec = 3;
    std::atomic<bool> m_stopRequested{false};
};