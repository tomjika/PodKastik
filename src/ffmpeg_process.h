#ifndef FFMPEG_PROCESS_H
#define FFMPEG_PROCESS_H

#include <QWidget>
#include <QProcess>
#include <QObject>
#include <QFileDialog>
#include <QRegularExpression>
#include <QTimer>

class ffmpeg_process : public QWidget
{
    Q_OBJECT

public:
    explicit ffmpeg_process(QWidget *parent = nullptr, QString p = "");
    ~ffmpeg_process();
    void exe_process(QString);
    void initialize_process();

private slots:
    void process_out();
    void process_err();
    void process_state_changed(QProcess::ProcessState);
    void process_error_state(QProcess::ProcessError);
    void process_finished(int, QProcess::ExitStatus);

public:
    QProcess *process;
    QTimer* init_timer;

  //ffmpeg app
    QString exe_path;
    QString version = "";
    bool use_portable = false;
    bool available = false;
    bool running = false;
    bool initializing = false;

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
    void process_ready(bool);
    void process_running(bool);
    void conversion_finished();
    void log(QString);
    void process_out_update();
};

#endif // FFMPEG_PROCESS_H
