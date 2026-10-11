#pragma once

#include <QAbstractNativeEventFilter>
#include <QVector>
#include <QWidget>

#include "macroplayer.h"

class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTimer;
class PointPicker;

class MainWindow : public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private slots:
    void addPoint();
    void removeSelected();
    void clearAll();
    void saveScript();
    void loadScript();
    void toggleRun();
    void onPicked(const QPoint &pos);
    void onPickerCanceled();
    void onStatusChanged(const QString &text);
    void onRunFinished(bool aborted);
    void onStopRequested();

private:
    void buildUi();
    void addRow(const MacroStep &step);
    void renumberRows();
    QVector<MacroStep> collectScript() const;
    void setRunning(bool running);
    void updateCountdown();
    void beginPlayback();
    void registerStopHotkey();
    void unregisterStopHotkey();

    QTableWidget *m_table = nullptr;
    QSpinBox *m_loopSpin = nullptr;
    QSpinBox *m_delaySpin = nullptr;
    QPushButton *m_addBtn = nullptr;
    QPushButton *m_delBtn = nullptr;
    QPushButton *m_clearBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_loadBtn = nullptr;
    QPushButton *m_runBtn = nullptr;
    QLabel *m_status = nullptr;
    PointPicker *m_picker = nullptr;
    MacroPlayer *m_player = nullptr;
    QTimer *m_countdownTimer = nullptr;

    QVector<MacroStep> m_pendingScript;
    QString m_lastScriptPath;
    int m_countdown = 0;
    bool m_running = false;
    bool m_hotkeyRegistered = false;
};