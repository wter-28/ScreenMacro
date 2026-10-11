#pragma once

#include <QPoint>
#include <QRect>
#include <QWidget>

// 全屏半透明遮罩，让用户直接点屏幕上的任意位置取坐标
class PointPicker : public QWidget
{
    Q_OBJECT

public:
    explicit PointPicker(QWidget *parent = nullptr);

    // 覆盖整个虚拟桌面（支持多显示器）并开始等待用户点击
    void start();

signals:
    void picked(const QPoint &physicalPos);
    void canceled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPoint m_hover;      // 当前光标位置（本窗口逻辑坐标）
    QRect m_virtualRect; // 虚拟桌面矩形，用于换算全局坐标
};