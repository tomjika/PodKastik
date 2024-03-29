#ifndef PODKASTIK_H
#define PODKASTIK_H

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QSettings>
#include <QTcpSocket>
#include <QDesktopServices>
#include "youtubedl_process.h"
#include "ffmpeg_process.h"
#include <QMimeData>

namespace Ui {
class PodKastik;
}

class PodKastik : public QWidget
{
    Q_OBJECT

public:
    explicit PodKastik(QWidget *parent = nullptr, QString outputPath = "");
    ~PodKastik();
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
    void loadSettings();

private:
    Ui::PodKastik *ui;

    std::unique_ptr<youtubedl_process> youtube_dl;
    ffmpeg_process *ffmpeg;

    QString output_path;

    QClipboard *clipboard;
    //QString dl_link;

    QString default_folder;
    QString ytdl_folder;
    QString ffmpeg_folder;
    QString default_prefix;
    QString current_file_name;
    QString output_file_name;
    QTime current_file_duration;
    int total_target_ms = 1;
    int current_ms_ffmpeg = 0;

    bool ytdl_AudioOnly;
    bool ytdl_IsPlaylist;
    bool ytdl_use_patched;

    QStringList filesToConvert;

public slots:
    void addToConvertList(QString);

private slots:
    void saveSettings();

    void CreateYtdlProcess(QString exe_path);
    void ytdl_process_out();
    void ytdl_process_ready(bool);
    void ytdl_process_running(bool);
    void ytdl_finished();

    void ffmpeg_process_out();
    void ffmpeg_process_ready(bool);
    void ffmpeg_process_running(bool);
    void ffmpeg_finished();
    void do_ffmpeg(QString);

    void on_pb_download_clicked();
    void on_pb_browse_output_clicked();
    void on_pb_settings_clicked();
    void on_pb_browse_default_clicked();
    void on_pb_browse_ytdl_clicked();
    void on_pb_browse_ffmpeg_clicked();
    void on_dsb_tempo_valueChanged(double arg1);
    void on_cb_to_mono_stateChanged(int arg1);
    void on_sb_kbits_valueChanged(double arg1);
    void on_rb_video_toggled(bool checked);
    void on_pb_select_file_to_convert_clicked();
    void on_pb_open_output_path_clicked();
    void on_rb_audio_toggled(bool checked);
    void on_cb_playlist_stateChanged(int arg1);
    void on_cb_ytdl_patched_stateChanged(int arg1);

    void clip_paste();
    void logging(QString);
    void tag_and_del();
    bool removeOldFile();
    bool renameNewFile();
    void on_pb_about_clicked();

    void SetTextToLabel(QLabel*, QString, Qt::TextElideMode);
    void SetTextToButton(QPushButton*, QString, Qt::TextElideMode);
    void on_le_prefix_format_textChanged(const QString &arg1);

};

#endif // PODKASTIK_H
