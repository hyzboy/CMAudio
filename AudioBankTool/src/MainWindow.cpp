#include "MainWindow.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QToolBar>
#include <QAction>
#include <QFile>
#include <QFileInfo>

#include <hgl/audio/AudioFileType.h>
#include <hgl/audio/AudioBank.h>
#include <hgl/audio/AudioPlayer.h>
#include <hgl/audio/OpenAL.h>
#include <hgl/io/MemoryInputStream.h>
#include <hgl/CoreType.h>
#include <hgl/type/String.h>

#include <cstring>

using namespace hgl;
using hgl::audio::AudioFileType;
using hgl::audio::AudioBankWriter;
using hgl::audio::CheckAudioExtName;
using hgl::io::MemoryInputStream;

namespace
{
    const char *FileTypeName(AudioFileType type)
    {
        switch(type)
        {
            case AudioFileType::Wav:    return "WAV";
            case AudioFileType::Vorbis: return "OGG";
            case AudioFileType::Opus:   return "OPUS";
            case AudioFileType::MIDI:   return "MIDI";
            default:                    return "?";
        }
    }

    QString HumanSize(qint64 bytes)
    {
        if(bytes<1024)      return QString("%1 B").arg(bytes);
        if(bytes<1024*1024) return QString("%1 KB").arg(bytes/1024.0,0,'f',1);
        return QString("%1 MB").arg(bytes/(1024.0*1024.0),0,'f',2);
    }
}//namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , table(nullptr)
    , preview_player(nullptr)
{
    setWindowTitle(tr("AudioBank Authoring Tool"));
    resize(960, 600);

    // 条目表格：名字 | 分组 | 格式 | 大小 | 循环 | 增益 | 源文件
    table = new QTableWidget(0, 7, this);
    table->setHorizontalHeaderLabels({
        tr("名字"), tr("分组"), tr("格式"), tr("大小"), tr("循环"), tr("增益"), tr("源文件")
    });
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    connect(table, &QTableWidget::cellChanged, this, &MainWindow::OnCellChanged);

    setCentralWidget(table);

    CreateMenus();
    CreateToolbar();

    statusBar()->showMessage(tr("就绪"));
}

MainWindow::~MainWindow()
{
    StopPreview();
    delete preview_player;
}

void MainWindow::CreateMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));

    QAction *importAction = fileMenu->addAction(tr("导入音频(&I)..."));
    importAction->setShortcut(QKeySequence::Open);
    connect(importAction, &QAction::triggered, this, &MainWindow::OnImportFiles);

    QAction *exportAction = fileMenu->addAction(tr("导出 .bank(&E)..."));
    exportAction->setShortcut(QKeySequence::SaveAs);
    connect(exportAction, &QAction::triggered, this, &MainWindow::OnExport);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction(tr("退出(&X)"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::OnExit);
}

void MainWindow::CreateToolbar()
{
    QToolBar *bar = addToolBar(tr("主工具栏"));
    bar->setMovable(false);

    QAction *importAction = bar->addAction(tr("导入"));
    connect(importAction, &QAction::triggered, this, &MainWindow::OnImportFiles);

    QAction *removeAction = bar->addAction(tr("移除"));
    connect(removeAction, &QAction::triggered, this, &MainWindow::OnRemove);

    bar->addSeparator();

    QAction *previewAction = bar->addAction(tr("预览"));
    connect(previewAction, &QAction::triggered, this, &MainWindow::OnPreview);

    QAction *exportAction = bar->addAction(tr("导出"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::OnExport);
}

QString MainWindow::NameForPath(const QString &path) const
{
    QFileInfo fi(path);
    return fi.completeBaseName();
}

void MainWindow::AddEntry(const EntryItem &item)
{
    entries.append(item);

    const int row = entries.size() - 1;
    UpdateTableRow(row);
}

void MainWindow::UpdateTableRow(int row) const
{
    if(row < 0 || row >= entries.size())
        return;

    const EntryItem &e = entries[row];

    table->blockSignals(true);      // 避免回写触发

    QTableWidgetItem *nameItem  = new QTableWidgetItem(e.name);
    nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
    table->setItem(row, 0, nameItem);

    QTableWidgetItem *groupItem = new QTableWidgetItem(e.group);
    groupItem->setFlags(groupItem->flags() | Qt::ItemIsEditable);
    table->setItem(row, 1, groupItem);

    table->setItem(row, 2, new QTableWidgetItem(FileTypeName(e.file_type)));
    table->setItem(row, 3, new QTableWidgetItem(HumanSize(e.data.size())));

    QTableWidgetItem *loopItem = new QTableWidgetItem;
    loopItem->setFlags(loopItem->flags() | Qt::ItemIsUserCheckable);
    loopItem->setCheckState(e.loop ? Qt::Checked : Qt::Unchecked);
    table->setItem(row, 4, loopItem);

    QTableWidgetItem *gainItem = new QTableWidgetItem(QString::number(e.gain, 'f', 2));
    gainItem->setFlags(gainItem->flags() | Qt::ItemIsEditable);
    table->setItem(row, 5, gainItem);

    QTableWidgetItem *srcItem = new QTableWidgetItem(e.source_path);
    srcItem->setFlags(srcItem->flags() & ~Qt::ItemIsEditable);
    table->setItem(row, 6, srcItem);

    table->blockSignals(false);
}

void MainWindow::OnCellChanged(int row, int col)
{
    if(row < 0 || row >= entries.size())
        return;

    EntryItem &e = entries[row];
    QTableWidgetItem *item = table->item(row, col);

    if(!item)
        return;

    switch(col)
    {
        case 0: e.name = item->text(); break;
        case 1: e.group = item->text(); break;
        case 4: e.loop = (item->checkState() == Qt::Checked); break;
        case 5: e.gain = item->text().toFloat(); break;
        default: break;
    }
}

void MainWindow::OnImportFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("选择音频文件"), QString(),
        tr("音频文件 (*.wav *.ogg *.opus);;WAV (*.wav);;OGG (*.ogg *.opus);;所有文件 (*)"));

    if(files.isEmpty())
        return;

    int added = 0;

    for(const QString &path : files)
    {
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(this, tr("导入失败"), tr("无法读取文件:\n%1").arg(path));
            continue;
        }

        const QByteArray data = file.readAll();
        file.close();

        if(data.isEmpty())
            continue;

        // 按扩展名探测格式
        const QString ext = QFileInfo(path).suffix().toLower();
        const std::wstring ext_w = ext.toStdWString();
        const OSString os_ext = OSString(ext_w.c_str());
        const AudioFileType type = CheckAudioExtName(os_ext.c_str());

        if(type == AudioFileType::None)
        {
            QMessageBox::warning(this, tr("导入失败"), tr("不支持的音频格式:\n%1").arg(path));
            continue;
        }

        EntryItem item;
        item.name = NameForPath(path);
        item.group = QString();
        item.loop = false;
        item.gain = 1.0f;
        item.file_type = type;
        item.source_path = path;
        item.data = data;

        // 重名自动加后缀
        bool name_ok = true;
        for(const EntryItem &e : entries)
        {
            if(e.name == item.name)
            {
                name_ok = false;
                break;
            }
        }
        if(!name_ok)
            item.name += QString("_%1").arg(entries.size() + 1);

        AddEntry(item);
        added++;
    }

    if(added > 0)
        statusBar()->showMessage(tr("已导入 %1 个音频文件").arg(added), 5000);
}

void MainWindow::OnRemove()
{
    QList<int> rows;

    const auto selected = table->selectionModel()->selectedRows();
    for(const auto &index : selected)
        rows.append(index.row());

    if(rows.isEmpty())
        return;

    StopPreview();

    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for(int row : rows)
    {
        entries.removeAt(row);
        table->removeRow(row);
    }

    statusBar()->showMessage(tr("已移除 %1 个条目").arg(rows.size()), 3000);
}

bool MainWindow::EnsureOpenAL()
{
    if(openal_ready)
        return true;

    if(!openal::InitOpenAL())
    {
        QMessageBox::warning(this, tr("预览失败"),
            tr("OpenAL 初始化失败。\n请确认 OpenAL32.dll 位于程序目录。"));
        return false;
    }

    openal_ready = true;
    return true;
}

void MainWindow::StopPreview()
{
    if(preview_player)
        preview_player->Stop();
}

void MainWindow::OnPreview()
{
    const auto selected = table->selectionModel()->selectedRows();
    if(selected.isEmpty())
    {
        statusBar()->showMessage(tr("请先选中一个条目"), 3000);
        return;
    }

    const int row = selected.first().row();
    if(row < 0 || row >= entries.size())
        return;

    const EntryItem &e = entries[row];

    if(!EnsureOpenAL())
        return;

    if(!preview_player)
        preview_player = new hgl::audio::AudioPlayer();

    StopPreview();

    // bank 数据在内存：用 MemoryInputStream 走解码插件（不落盘）
    MemoryInputStream mis(const_cast<uint8 *>(reinterpret_cast<const uint8 *>(e.data.constData())),
                          static_cast<int64>(e.data.size()));

    if(!preview_player->Load(&mis, static_cast<int>(e.data.size()), e.file_type))
    {
        statusBar()->showMessage(tr("预览失败：无法解码 %1").arg(e.name), 5000);
        return;
    }

    preview_player->SetLoop(e.loop);
    preview_player->SetGain(e.gain);
    preview_player->Play();

    statusBar()->showMessage(tr("预览：%1").arg(e.name), 5000);
}

void MainWindow::OnExport()
{
    if(entries.isEmpty())
    {
        QMessageBox::information(this, tr("导出"), tr("没有可导出的条目。"));
        return;
    }

    const QString filename = QFileDialog::getSaveFileName(
        this, tr("导出 AudioBank"), QString("audio.bank"), tr("AudioBank (*.bank)"));

    if(filename.isEmpty())
        return;

    StopPreview();

    AudioBankWriter writer;

    for(const EntryItem &e : entries)
    {
        const std::wstring name_w  = e.name.toStdWString();
        const std::wstring group_w = e.group.toStdWString();

        if(!writer.AddEntry(OSString(name_w.c_str()),
                            OSString(group_w.c_str()),
                            e.file_type,
                            e.loop,
                            e.gain,
                            e.data.constData(),
                            static_cast<uint64>(e.data.size())))
        {
            QMessageBox::warning(this, tr("导出失败"),
                tr("条目 \"%1\" 添加失败（空数据/重名/未知格式）。").arg(e.name));
            return;
        }
    }

    const std::wstring path_w = filename.toStdWString();
    const OSString out_path(path_w.c_str());

    if(!writer.Write(out_path))
    {
        QMessageBox::warning(this, tr("导出失败"), tr("无法写入文件:\n%1").arg(filename));
        return;
    }

    statusBar()->showMessage(tr("已导出 %1 个条目 → %2").arg(entries.size()).arg(filename), 8000);
    QMessageBox::information(this, tr("导出成功"),
        tr("已导出 %1 个条目到:\n%2").arg(entries.size()).arg(filename));
}

void MainWindow::OnExit()
{
    close();
}
