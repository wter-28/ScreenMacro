#include "mainwindow.h"

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "pointpicker.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {
// 全局热键 ID，仅用于“停止执行”
constexpr int kStopHotkeyId = 0x5A01;

const char *kStyleSheet = R"QSS(
QWidget#root {
    background: #f4f5f7;
    font-family: "Microsoft YaHei UI", "Segoe UI", sans-serif;
    font-size: 13px;
    color: #1f2937;
}
QLabel#hint {
    color: #6b7280;
    font-size: 13px;
}
QLabel#fieldLabel {
    color: #4b5563;
}
QLabel#status {
    color: #6b7280;
    font-size: 12px;
}
QTableWidget {
    background: #ffffff;
    border: 1px solid #e5e7eb;
    border-radius: 10px;
    outline: none;
}
QTableWidget::item {
    padding: 0px 10px;
    border: none;
}
QTableWidget::item:selected {
    background: #e0edff;
    color: #111827;
}
QHeaderView::section {
    background: #fafbfc;
    border: none;
    border-bottom: 1px solid #e5e7eb;
    padding: 9px 10px;
    color: #6b7280;
    font-weight: 600;
}
QDoubleSpinBox {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 2px 6px;
    color: #1f2937;
}
QDoubleSpinBox:hover {
    border: 1px solid #cbd5e1;
    background: #ffffff;
}
QDoubleSpinBox:focus {
    border: 1px solid #2563eb;
    background: #ffffff;
}
QComboBox {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 2px 0px 2px 6px;
    color: #1f2937;
}
QComboBox:hover {
    border: 1px solid #cbd5e1;
    background: #ffffff;
}
QComboBox::drop-down {
    border: none;
    width: 16px;
}
QComboBox QAbstractItemView {
    background: #ffffff;
    border: 1px solid #e5e7eb;
    border-radius: 8px;
    padding: 4px;
    outline: none;
    selection-background-color: #e0edff;
    selection-color: #111827;
}
QSpinBox {
    background: #ffffff;
    border: 1px solid #e5e7eb;
    border-radius: 8px;
    padding: 5px 4px 5px 10px;
    color: #1f2937;
    min-width: 40px;
    min-height: 22px;
}
QSpinBox:focus {
    border: 1px solid #2563eb;
}
QSpinBox#cellSpin {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 2px 6px;
}
QSpinBox#cellSpin:hover {
    border: 1px solid #cbd5e1;
    background: #ffffff;
}
QSpinBox#cellSpin:focus {
    border: 1px solid #2563eb;
    background: #ffffff;
}
QPushButton {
    background: #ffffff;
    border: 1px solid #e5e7eb;
    border-radius: 8px;
    padding: 8px 14px;
    color: #374151;
}
QPushButton:hover {
    background: #f3f4f6;
}
QPushButton:pressed {
    background: #e5e7eb;
}
QPushButton:disabled {
    color: #9ca3af;
    background: #f9fafb;
}
QPushButton#primary {
    background: #2563eb;
    border: none;
    border-radius: 10px;
    padding: 14px;
    font-size: 15px;
    font-weight: 600;
    color: #ffffff;
}
QPushButton#primary:hover {
    background: #1d4ed8;
}
QPushButton#primary:pressed {
    background: #1e40af;
}
QPushButton#primary[running="true"] {
    background: #dc2626;
}
QPushButton#primary[running="true"]:hover {
    background: #b91c1c;
}
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 4px 2px 4px 0px;
}
QScrollBar::handle:vertical {
    background: #d1d5db;
    border-radius: 4px;
    min-height: 30px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: transparent;
}
)QSS";
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("屏幕点击助手"));
    setStyleSheet(QString::fromUtf8(kStyleSheet));
    setFixedSize(520, 640);

    buildUi();

    m_picker = new PointPicker(this);
    connect(m_picker, &PointPicker::picked, this, &MainWindow::onPicked);
    connect(m_picker, &PointPicker::canceled, this, &MainWindow::onPickerCanceled);

    qApp->installNativeEventFilter(this);
}

MainWindow::~MainWindow()
{
    unregisterStopHotkey();
    qApp->removeNativeEventFilter(this);

    if (m_player) {
        m_player->requestStop();
        m_player->wait();
    }
}

void MainWindow::buildUi()
{
    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("root"));

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(root);

    auto *layout = new QVBoxLayout(root);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(14);

    auto *hint = new QLabel(QStringLiteral("① 添加点击点　② 填写等待秒数　③ 开始执行"));
    hint->setObjectName(QStringLiteral("hint"));
    layout->addWidget(hint);

    // ---- 步骤列表 ----
    m_table = new QTableWidget(0, 5, root);
    m_table->setHorizontalHeaderLabels({QStringLiteral("#"),
                                        QStringLiteral("位置"),
                                        QStringLiteral("操作"),
                                        QStringLiteral("次数"),
                                        QStringLiteral("等待(秒)")});
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setFixedHeight(36);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 42);
    m_table->setColumnWidth(2, 120);
    m_table->setColumnWidth(3, 62);
    m_table->setColumnWidth(4, 88);
    m_table->verticalHeader()->setDefaultSectionSize(42);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    layout->addWidget(m_table, 1);

    // ---- 列表操作与脚本文件按钮 ----
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    m_addBtn = new QPushButton(QStringLiteral("＋ 添加点击点"));
    m_delBtn = new QPushButton(QStringLiteral("删除"));
    m_clearBtn = new QPushButton(QStringLiteral("清空"));
    m_saveBtn = new QPushButton(QStringLiteral("保存脚本"));
    m_loadBtn = new QPushButton(QStringLiteral("读取脚本"));
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_delBtn);
    btnRow->addWidget(m_clearBtn);
    btnRow->addStretch(1);
    btnRow->addWidget(m_saveBtn);
    btnRow->addWidget(m_loadBtn);
    layout->addLayout(btnRow);

    // ---- 运行参数 ----
    auto *paramRow = new QHBoxLayout;
    paramRow->setSpacing(8);

    auto *loopLabel = new QLabel(QStringLiteral("重复"));
    loopLabel->setObjectName(QStringLiteral("fieldLabel"));
    m_loopSpin = new QSpinBox;
    m_loopSpin->setRange(1, 9999);
    m_loopSpin->setValue(1);

    auto *loopUnit = new QLabel(QStringLiteral("次"));
    loopUnit->setObjectName(QStringLiteral("fieldLabel"));

    auto *delayLabel = new QLabel(QStringLiteral("开始前等待"));
    delayLabel->setObjectName(QStringLiteral("fieldLabel"));
    m_delaySpin = new QSpinBox;
    m_delaySpin->setRange(0, 60);
    m_delaySpin->setValue(3);

    auto *delayUnit = new QLabel(QStringLiteral("秒"));
    delayUnit->setObjectName(QStringLiteral("fieldLabel"));

    paramRow->addWidget(loopLabel);
    paramRow->addWidget(m_loopSpin);
    paramRow->addWidget(loopUnit);
    paramRow->addStretch(1);
    paramRow->addWidget(delayLabel);
    paramRow->addWidget(m_delaySpin);
    paramRow->addWidget(delayUnit);
    layout->addLayout(paramRow);

    // ---- 主按钮与状态 ----
    m_runBtn = new QPushButton(QStringLiteral("▶  开始执行"));
    m_runBtn->setObjectName(QStringLiteral("primary"));
    m_runBtn->setCursor(Qt::PointingHandCursor);
    m_runBtn->setProperty("running", false);
    layout->addWidget(m_runBtn);

    m_status = new QLabel(QStringLiteral("点击「＋ 添加点击点」，然后在屏幕上点一下即可取点"));
    m_status->setObjectName(QStringLiteral("status"));
    m_status->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_status);

    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::addPoint);
    connect(m_delBtn, &QPushButton::clicked, this, &MainWindow::removeSelected);
    connect(m_clearBtn, &QPushButton::clicked, this, &MainWindow::clearAll);
    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::saveScript);
    connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::loadScript);
    connect(m_runBtn, &QPushButton::clicked, this, &MainWindow::toggleRun);

    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setInterval(1000);
    connect(m_countdownTimer, &QTimer::timeout, this, [this] {
        --m_countdown;
        if (m_countdown > 0) {
            updateCountdown();
        } else {
            m_countdownTimer->stop();
            beginPlayback();
        }
    });
}

// ---------------------------------------------------------------- 选点

void MainWindow::addPoint()
{
    // 先把自己藏起来，否则遮罩会把本窗口一起盖住
    hide();
    QTimer::singleShot(200, this, [this] {
        m_picker->start();
    });
}

void MainWindow::onPicked(const QPoint &pos)
{
    MacroStep step;
    step.pos = pos;
    addRow(step);
    m_status->setText(QStringLiteral("已添加 (%1, %2)").arg(pos.x()).arg(pos.y()));
    show();
    activateWindow();
    raise();
}

void MainWindow::onPickerCanceled()
{
    show();
    activateWindow();
    raise();
    m_status->setText(QStringLiteral("已取消选点"));
}

// ---------------------------------------------------------------- 列表维护

void MainWindow::addRow(const MacroStep &step)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);

    auto *indexItem = new QTableWidgetItem(QString::number(row + 1));
    indexItem->setTextAlignment(Qt::AlignCenter);
    indexItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_table->setItem(row, 0, indexItem);

    auto *posItem = new QTableWidgetItem(QStringLiteral("(%1, %2)")
                                             .arg(step.pos.x())
                                             .arg(step.pos.y()));
    posItem->setData(Qt::UserRole, step.pos);
    posItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_table->setItem(row, 1, posItem);

    auto *actionBox = new QComboBox;
    actionBox->addItem(stepActionLabel(StepAction::LeftClick), int(StepAction::LeftClick));
    actionBox->addItem(stepActionLabel(StepAction::RightClick), int(StepAction::RightClick));
    actionBox->addItem(stepActionLabel(StepAction::DoubleClick), int(StepAction::DoubleClick));
    actionBox->setCurrentIndex(actionBox->findData(int(step.action)));
    actionBox->setCursor(Qt::PointingHandCursor);
    m_table->setCellWidget(row, 2, actionBox);

    auto *timesSpin = new QSpinBox;
    timesSpin->setObjectName(QStringLiteral("cellSpin"));
    timesSpin->setRange(1, 9999);
    timesSpin->setValue(qMax(1, step.repeatCount));
    timesSpin->setAlignment(Qt::AlignCenter);
    timesSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_table->setCellWidget(row, 3, timesSpin);

    auto *waitSpin = new QDoubleSpinBox;
    waitSpin->setRange(0.0, 3600.0);
    waitSpin->setDecimals(1);
    waitSpin->setSingleStep(0.5);
    waitSpin->setValue(step.waitSec);
    waitSpin->setSuffix(QStringLiteral(" s"));
    waitSpin->setAlignment(Qt::AlignCenter);
    waitSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_table->setCellWidget(row, 4, waitSpin);

    m_table->setCurrentCell(row, 1);
}

void MainWindow::renumberRows()
{
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (QTableWidgetItem *item = m_table->item(row, 0)) {
            item->setText(QString::number(row + 1));
        }
    }
}

void MainWindow::removeSelected()
{
    const int row = m_table->currentRow();
    if (row < 0) {
        return;
    }
    m_table->removeRow(row);
    renumberRows();
    m_status->setText(QStringLiteral("已删除一步"));
}

void MainWindow::clearAll()
{
    if (m_table->rowCount() == 0) {
        return;
    }
    m_table->setRowCount(0);
    m_status->setText(QStringLiteral("列表已清空"));
}

void MainWindow::saveScript()
{
    const QVector<MacroStep> steps = collectScript();
    if (steps.isEmpty()) {
        m_status->setText(QStringLiteral("列表是空的，没有可保存的步骤"));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存脚本"),
        m_lastScriptPath.isEmpty() ? QStringLiteral("点击脚本.json") : m_lastScriptPath,
        QStringLiteral("点击脚本 (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    if (!path.endsWith(QLatin1String(".json"), Qt::CaseInsensitive)) {
        path += QLatin1String(".json");
    }

    QJsonArray array;
    for (const MacroStep &step : steps) {
        QJsonObject item;
        item[QStringLiteral("x")] = step.pos.x();
        item[QStringLiteral("y")] = step.pos.y();
        item[QStringLiteral("action")] = stepActionKey(step.action);
        item[QStringLiteral("repeat")] = step.repeatCount;
        item[QStringLiteral("wait")] = step.waitSec;
        array.append(item);
    }

    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("loop")] = m_loopSpin->value();
    root[QStringLiteral("startupDelay")] = m_delaySpin->value();
    root[QStringLiteral("steps")] = array;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), file.errorString());
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    m_lastScriptPath = path;
    m_status->setText(QStringLiteral("已保存 %1 步到 %2")
                          .arg(steps.size())
                          .arg(QFileInfo(path).fileName()));
}

void MainWindow::loadScript()
{
    const QString path = QFileDialog::getOpenFileName(this,
                                                      QStringLiteral("读取脚本"),
                                                      m_lastScriptPath,
                                                      QStringLiteral("点击脚本 (*.json)"));
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("读取失败"), file.errorString());
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this,
                             QStringLiteral("读取失败"),
                             QStringLiteral("文件不是有效的脚本：%1")
                                 .arg(parseError.errorString()));
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonArray array = root.value(QStringLiteral("steps")).toArray();
    if (array.isEmpty()) {
        m_status->setText(QStringLiteral("这个脚本里没有任何步骤"));
        return;
    }

    m_table->setRowCount(0);
    m_loopSpin->setValue(qMax(1, root.value(QStringLiteral("loop")).toInt(1)));
    m_delaySpin->setValue(qBound(0, root.value(QStringLiteral("startupDelay")).toInt(3), 60));

    for (const QJsonValue &value : array) {
        const QJsonObject item = value.toObject();
        MacroStep step;
        step.pos = QPoint(item.value(QStringLiteral("x")).toInt(),
                          item.value(QStringLiteral("y")).toInt());
        step.action = stepActionFromKey(item.value(QStringLiteral("action")).toString());
        step.repeatCount = qMax(1, item.value(QStringLiteral("repeat")).toInt(1));
        step.waitSec = item.value(QStringLiteral("wait")).toDouble(1.0);
        addRow(step);
    }

    m_table->scrollToTop();
    m_lastScriptPath = path;
    m_status->setText(QStringLiteral("已读取 %1 步").arg(m_table->rowCount()));
}

QVector<MacroStep> MainWindow::collectScript() const
{
    QVector<MacroStep> steps;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const QTableWidgetItem *posItem = m_table->item(row, 1);
        if (!posItem) {
            continue;
        }

        MacroStep step;
        step.pos = posItem->data(Qt::UserRole).toPoint();

        if (auto *box = qobject_cast<QComboBox *>(m_table->cellWidget(row, 2))) {
            step.action = static_cast<StepAction>(box->currentData().toInt());
        }
        if (auto *spin = qobject_cast<QSpinBox *>(m_table->cellWidget(row, 3))) {
            step.repeatCount = spin->value();
        }
        if (auto *spin = qobject_cast<QDoubleSpinBox *>(m_table->cellWidget(row, 4))) {
            step.waitSec = spin->value();
        }
        steps.append(step);
    }
    return steps;
}

// ---------------------------------------------------------------- 执行控制

void MainWindow::toggleRun()
{
    if (m_running) {
        onStopRequested();
        return;
    }

    const QVector<MacroStep> script = collectScript();
    if (script.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("还没有点击点"),
                                 QStringLiteral("请先点击「＋ 添加点击点」在屏幕上取点。"));
        return;
    }

    m_pendingScript = script;
    setRunning(true);
    registerStopHotkey();

    m_countdown = m_delaySpin->value();
    if (m_countdown > 0) {
        updateCountdown();
        m_countdownTimer->start();
    } else {
        beginPlayback();
    }
}

void MainWindow::updateCountdown()
{
    m_status->setText(QStringLiteral("%1 秒后开始，按 F9 可取消").arg(m_countdown));
}

void MainWindow::beginPlayback()
{
    m_player = new MacroPlayer(this);
    connect(m_player, &MacroPlayer::statusChanged, this, &MainWindow::onStatusChanged);
    connect(m_player, &MacroPlayer::runFinished, this, &MainWindow::onRunFinished);
    m_player->setScript(m_pendingScript, m_loopSpin->value(), 0);

    hide();
    // 给系统一点时间把焦点交还给用户的目标窗口
    QTimer::singleShot(300, this, [this] {
        if (m_player) {
            m_player->start();
        }
    });
}

void MainWindow::onStatusChanged(const QString &text)
{
    m_status->setText(text);
}

void MainWindow::onRunFinished(bool aborted)
{
    Q_UNUSED(aborted);

    unregisterStopHotkey();
    m_countdownTimer->stop();
    m_countdown = 0;

    if (m_player) {
        m_player->wait();
        m_player->deleteLater();
        m_player = nullptr;
    }

    setRunning(false);
    show();
    activateWindow();
    raise();
}

void MainWindow::onStopRequested()
{
    if (m_countdownTimer->isActive()) {
        m_countdownTimer->stop();
        m_countdown = 0;
        unregisterStopHotkey();
        setRunning(false);
        m_status->setText(QStringLiteral("已取消"));
        return;
    }

    if (m_player) {
        m_player->requestStop();
        m_status->setText(QStringLiteral("正在停止…"));
    }
}

void MainWindow::setRunning(bool running)
{
    m_running = running;

    m_runBtn->setText(running ? QStringLiteral("■  停止执行  (F9)")
                              : QStringLiteral("▶  开始执行"));
    m_runBtn->setProperty("running", running);
    m_runBtn->style()->unpolish(m_runBtn);
    m_runBtn->style()->polish(m_runBtn);

    m_addBtn->setEnabled(!running);
    m_delBtn->setEnabled(!running);
    m_clearBtn->setEnabled(!running);
    m_saveBtn->setEnabled(!running);
    m_loadBtn->setEnabled(!running);
    m_table->setEnabled(!running);
    m_loopSpin->setEnabled(!running);
    m_delaySpin->setEnabled(!running);
}

void MainWindow::registerStopHotkey()
{
    if (m_hotkeyRegistered) {
        return;
    }
    // 只在执行期间占用 F9，平时不干扰系统
    m_hotkeyRegistered = ::RegisterHotKey(nullptr, kStopHotkeyId, 0, VK_F9);
}

void MainWindow::unregisterStopHotkey()
{
    if (!m_hotkeyRegistered) {
        return;
    }
    ::UnregisterHotKey(nullptr, kStopHotkeyId);
    m_hotkeyRegistered = false;
}

bool MainWindow::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);

    if (eventType != "windows_generic_MSG") {
        return false;
    }

    const MSG *msg = static_cast<const MSG *>(message);
    if (msg->message == WM_HOTKEY && static_cast<int>(msg->wParam) == kStopHotkeyId) {
        onStopRequested();
        return true;
    }
    return false;
}