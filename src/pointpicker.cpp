#include "pointpicker.h"

#include <QCursor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

PointPicker::PointPicker(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

void PointPicker::start()
{
    // 覆盖所有显示器拼成的整个虚拟桌面
    QRect virtualRect;
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        virtualRect = virtualRect.united(screen->geometry());
    }

    m_virtualRect = virtualRect;
    m_hover = QCursor::pos() - virtualRect.topLeft();
    setGeometry(virtualRect);
    show();
    raise();
    activateWindow();
    setFocus();
    grabKeyboard(); // 保证 Esc 一定能收到
    update();
}

void PointPicker::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 半透明遮罩，既压暗背景又保留画面可辨识
    p.fillRect(rect(), QColor(15, 23, 42, 120));

    QFont f = font();
    f.setFamily(QStringLiteral("Microsoft YaHei UI"));
    f.setPointSize(11);
    p.setFont(f);

    // 顶部操作提示
    const QString tip = QStringLiteral("点击屏幕任意位置记录坐标　·　Esc 取消");
    const QFontMetrics fm(f);
    QRect tipRect = fm.boundingRect(tip).adjusted(-20, -12, 20, 12);
    tipRect.moveCenter(QPoint(width() / 2, 64));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(17, 24, 39, 225));
    p.drawRoundedRect(tipRect, 10, 10);
    p.setPen(QColor(255, 255, 255));
    p.drawText(tipRect, Qt::AlignCenter, tip);

    const QPoint pos = m_hover;

    // 贯穿全屏的十字辅助线
    p.setPen(QPen(QColor(255, 255, 255, 190), 1, Qt::DashLine));
    p.drawLine(0, pos.y(), width(), pos.y());
    p.drawLine(pos.x(), 0, pos.x(), height());

    // 光标处的瞄准圈
    p.setPen(QPen(QColor(56, 189, 248), 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(pos, 15, 15);
    p.setPen(QPen(QColor(56, 189, 248), 3));
    p.drawPoint(pos);

    // 实时坐标气泡
    POINT cursorPos = {};
    ::GetCursorPos(&cursorPos);
    const QString coordText = QStringLiteral("%1, %2").arg(cursorPos.x).arg(cursorPos.y);

    QRect coordRect = fm.boundingRect(coordText).adjusted(-14, -8, 14, 8);
    QPoint anchor = pos + QPoint(24, 24);
    if (anchor.x() + coordRect.width() > width()) {
        anchor.setX(pos.x() - 24 - coordRect.width());
    }
    if (anchor.y() + coordRect.height() > height()) {
        anchor.setY(pos.y() - 24 - coordRect.height());
    }
    coordRect.moveTopLeft(anchor);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(37, 99, 235, 235));
    p.drawRoundedRect(coordRect, 8, 8);
    p.setPen(QColor(255, 255, 255));
    p.drawText(coordRect, Qt::AlignCenter, coordText);
}

void PointPicker::mouseMoveEvent(QMouseEvent *event)
{
    m_hover = event->position().toPoint();
    update();
}

void PointPicker::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    // 直接用系统光标位置，拿到的就是物理像素坐标，无需做 DPI 换算
    POINT cursorPos = {};
    ::GetCursorPos(&cursorPos);

    releaseKeyboard();
    hide();
    emit picked(QPoint(cursorPos.x, cursorPos.y));
}

void PointPicker::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        releaseKeyboard();
        hide();
        emit canceled();
        return;
    }
    QWidget::keyPressEvent(event);
}