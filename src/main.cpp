#include <QApplication>
#include <QStyle>
#include <QStyleFactory>

#include "mainwindow.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

int main(int argc, char *argv[])
{
    // 让进程按物理像素工作，这样取到的屏幕坐标能直接交给 SendInput
    if (!::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        ::SetProcessDPIAware();
    }
    // 关闭 Qt 的高 DPI 缩放，避免逻辑坐标与物理像素之间来回换算
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");

    QApplication app(argc, argv);
    // 固定使用 Fusion 样式，避免各版本 Windows 原生样式与自定义样式表互相干扰
    if (QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion"))) {
        QApplication::setStyle(fusion);
    }
    app.setApplicationName(QStringLiteral("ScreenMacro"));
    app.setApplicationDisplayName(QStringLiteral("屏幕点击助手"));

    MainWindow window;
    window.show();

    return app.exec();
}