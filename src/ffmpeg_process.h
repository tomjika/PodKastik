#ifndef FFMPEG_PROCESS_H
#define FFMPEG_PROCESS_H

#include <QWidget>
#include <QProcess>
#include <QObject>
#include <QFileDialog>
#include <QRegularExpression>

class ffmpeg_process : public QWidget
{
    Q_OBJECT

public:
    explicit ffmpeg_process(QWidget *parent = nullptr, QString p = "");
    ~ffmpeg_process();
    void exe_process(QString);

private slots:
    void process_out();
    void process_err();
    void process_state_changed(QProcess::ProcessState);
    void process_error_state(QProcess::ProcessError);
    void process_finished(int, QProcess::ExitStatus);

public:
    QProcess *process;

  //ffmpeg app
    QString version = "";
    bool use_portable = false;
    QString exe_path;
    bool available = false;

  //process execution
    QString advance_status;
    double conv_progress;
    QTime current_file_duration;
    int total_target_ms = 1;
    int current_ms_ffmpeg = 0;

  //arguments
    double speed_tempo;
    bool stereo_to_mono;
    double to_kbit;
    QString output_file_name;

signals:
    void process_ready();
    void process_state(QProcess::ProcessState);
    void conversion_finished();
    void log(QString);
    void process_out_update();
};

#endif // FFMPEG_PROCESS_H
