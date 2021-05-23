#ifndef YOUTUBEDL_PROCESS_H
#define YOUTUBEDL_PROCESS_H

#include <QWidget>
#include <QProcess>
#include <QObject>
#include <QFileDialog>
#include <QRegularExpression>
#include <QMessageBox>
#include <QTcpSocket>

class youtubedl_process : public QWidget
{
    Q_OBJECT

public:
    explicit youtubedl_process(QWidget *parent = nullptr, QString p = "");
    ~youtubedl_process();
    void exe_process(QString);

private slots:
    void process_out();
    void process_err();
    void process_state_changed(QProcess::ProcessState);
    void process_error_state(QProcess::ProcessError);
    void process_finished(int, QProcess::ExitStatus);
    bool urlExists(QString);

public:
    QProcess *process;

  //youtube-dl app
    QString version = "";
    bool use_portable = false;
    QString exe_path;
    bool available = false;

  //process execution
    QString current_file_path_name;
    QString current_file_name;
    QString advance_status;
    double dl_progress;

  //arguments
    bool audio_only = true;
    bool is_playlist = false;
    QString output_folder;

signals:
    void process_ready();
    void process_state(QProcess::ProcessState);
    void download_finished();
    void log(QString);
    void process_out_update();
};

#endif // YOUTUBEDL_PROCESS_H
