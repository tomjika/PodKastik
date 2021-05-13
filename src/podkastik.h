#ifndef PODKASTIK_H
#define PODKASTIK_H

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QSettings>
//#include <QSslSocket >
#include <QTcpSocket>
#include <QRegularExpression>
#include <QDesktopServices>
#include "youtubedl_process.h"
#include "ffmpeg_process.h"

namespace Ui {
class PodKastik;
}

class PodKastik : public QWidget
{
    Q_OBJECT

public:
    explicit PodKastik(QWidget *parent = nullptr);
    ~PodKastik();

private:
    Ui::PodKastik *ui;

    //QProcess *youtube_dl;
    youtubedl_process *youtube_dl;
    ffmpeg_process *ffmpeg;
    QString output_path;

    QClipboard *clipboard;
    QString dl_link;

    QString default_folder;
    QString ytdl_folder;
    QString ffmpeg_folder;
    QString current_file_name;
    QString output_file_name;
    QTime current_file_duration;
    int total_target_ms = 1;
    int current_ms_ffmpeg = 0;

    //double speed_tempo;
    bool stereo_to_mono;
    double to_kbit;

private slots:
    void loadSettings();
    void saveSettings();

    void ytdl_process_out();
    void ytdl_state_changed(QProcess::ProcessState);
    void ytdl_finished();

    void ffmpeg_process_out();
    void ffmpeg_state_changed(QProcess::ProcessState);
    void ffmpeg_error_state(QProcess::ProcessError);
    void ffmpeg_finished();

    void on_pb_download_clicked();
    void on_pb_browse_clicked();
    void on_pb_settings_clicked();
    void clip_paste();
    void on_pb_browse_default_clicked();
    void on_pb_browse_ytdl_clicked();
    bool urlExists(QString);
    void on_pb_browse_pressed();
    void on_pb_browse_released();
    void do_ffmpeg(QString);
    void on_pb_browse_ffmpeg_clicked();
    void do_ytdl();
    void on_dsb_tempo_valueChanged(double arg1);
    void on_cb_to_mono_stateChanged(int arg1);
    void on_sb_kbits_valueChanged(double arg1);
    void on_rb_video_toggled(bool checked);
    void on_pb_select_file_to_convert_clicked();
    void on_pb_open_output_path_clicked();
    void logging(QString);
};

#endif // PODKASTIK_H
