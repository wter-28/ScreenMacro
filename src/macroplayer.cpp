#include "macroplayer.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

MacroPlayer::MacroPlayer(QObject *parent)
    : QThread(parent)
{
}

void MacroPlayer::setScript(const QVector<MacroStep> &steps, int loops, int startupDelaySec)
{
    m_steps = steps;
    m_loops = qMax(1, loops);
    m_startupDelaySec = qMax(0, startupDelaySec);
    m_stopRequested = false;
}

void MacroPlayer::requestStop()
{
    m_stopRequested = true;
}

bool MacroPlayer::interruptibleSleep(int ms)
{
    while (ms > 0) {
        if (m_stopRequested) {
            return false;
        }
        const int chunk = qMin(ms, 20);
        QThread::msleep(static_cast<unsigned long>(chunk));
        ms -= chunk;
    }
    return !m_stopRequested;
}

namespace {
// 发送一次「按下 + 抬起」
void sendMousePair(DWORD downFlag, DWORD upFlag)
{
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = downFlag;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = upFlag;
    SendInput(2, inputs, sizeof(INPUT));
}
} // namespace

void MacroPlayer::performAction(const MacroStep &step)
{
    // 目标坐标本身就是物理像素，这里直接设置光标位置
    SetCursorPos(step.pos.x(), step.pos.y());
    QThread::msleep(15);

    switch (step.action) {
    case StepAction::RightClick:
        sendMousePair(MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP);
        break;
    case StepAction::DoubleClick: {
        sendMousePair(MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP);
        // 间隔取系统双击判定时间的一半，保证被识别成双击
        const int gap = qBound(30, static_cast<int>(::GetDoubleClickTime()) / 2, 250);
        QThread::msleep(static_cast<unsigned long>(gap));
        sendMousePair(MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP);
        break;
    }
    case StepAction::LeftClick:
        sendMousePair(MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP);
        break;
    }
}

void MacroPlayer::run()
{
    // 开始前的倒计时，给用户时间把目标窗口切到前台
    for (int s = m_startupDelaySec; s > 0; --s) {
        emit statusChanged(QStringLiteral("准备中… %1 秒后开始").arg(s));
        if (!interruptibleSleep(1000)) {
            emit runFinished(true);
            return;
        }
    }

    for (int loop = 0; loop < m_loops; ++loop) {
        for (int i = 0; i < m_steps.size(); ++i) {
            const MacroStep &step = m_steps.at(i);
            const int times = qMax(1, step.repeatCount);

            for (int t = 0; t < times; ++t) {
                if (m_stopRequested) {
                    emit runFinished(true);
                    return;
                }

                QString text = QStringLiteral("执行中 · 第 %1/%2 轮 · 第 %3/%4 步")
                                   .arg(loop + 1)
                                   .arg(m_loops)
                                   .arg(i + 1)
                                   .arg(m_steps.size());
                if (times > 1) {
                    text += QStringLiteral("（重复 %1/%2）").arg(t + 1).arg(times);
                }
                text += QStringLiteral(" → (%1, %2)").arg(step.pos.x()).arg(step.pos.y());
                emit statusChanged(text);

                performAction(step);

                if (!interruptibleSleep(static_cast<int>(step.waitSec * 1000.0))) {
                    emit runFinished(true);
                    return;
                }
            }
        }
    }

    emit statusChanged(QStringLiteral("已全部执行完成"));
    emit runFinished(false);
}