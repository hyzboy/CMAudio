#pragma once

#include <QMainWindow>
#include <QVector>
#include <QString>

#include <hgl/audio/AudioFileType.h>

class QTableWidget;
class QTableWidgetItem;
class QPushButton;

namespace hgl::audio { class AudioPlayer; }

/**
* AudioBank Authoring 工具主窗口
*
* P1：骨架
* P2：条目列表（多文件导入 / 表格编辑：名字/分组/循环/音量）
* P3：打包导出 .bank + 预览播放（AudioPlayer）
*/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    struct EntryItem
    {
        QString         name;           ///< 条目名（bank 内唯一）
        QString         group;          ///< 分组
        bool            loop=false;     ///< 循环
        float           gain=1.0f;      ///< 增益
        hgl::audio::AudioFileType file_type=hgl::audio::AudioFileType::None;
        QString         source_path;    ///< 源文件路径（显示用）
        QByteArray      data;           ///< 原始文件字节（入包数据）
    };

    QTableWidget *table;

    QVector<EntryItem> entries;

    hgl::audio::AudioPlayer *preview_player;    ///< 预览播放器（P3，懒创建）
    bool openal_ready=false;                    ///< OpenAL 是否已初始化

    void CreateMenus();
    void CreateToolbar();

    void AddEntry(const EntryItem &item);       ///< 加入模型 + 表格行
    void RemoveSelected();                      ///< 删除选中行
    void UpdateTableRow(int row) const;         ///< 模型 → 表格行

    QString NameForPath(const QString &path) const;     ///< 文件名去扩展名做默认条目名

    bool EnsureOpenAL();                        ///< 初始化 OpenAL（预览用），成功返回 true
    void StopPreview();                         ///< 停止当前预览

    // 表格编辑回写模型
    void OnCellChanged(int row,int col);

private slots:
    void OnImportFiles();       ///< 导入音频文件（P2）
    void OnRemove();            ///< 删除选中
    void OnExport();            ///< 导出 .bank（P3）
    void OnPreview();           ///< 预览播放选中条目（P3）
    void OnExit();
};
