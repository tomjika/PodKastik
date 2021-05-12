#ifndef YOUTUBEDL_PROCESS_H
#define YOUTUBEDL_PROCESS_H

#include <QWidget>
#include <QProcess>
#include <QObject>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>

class youtubedl_process : public QWidget
{
    Q_OBJECT

public:
    explicit youtubedl_process(QWidget *parent = nullptr, QString p = "");
    ~youtubedl_process();
    void exe_process(QStringList);

private slots:
    void ytdl_process_out();
    void ytdl_process_err();
    void ytdl_state_changed(QProcess::ProcessState);
    void ytdl_error_state(QProcess::ProcessError);
    void ytdl_finished(int, QProcess::ExitStatus);

public:
    QProcess *ytdl_process;
    QString version = "";
    bool use_portable = false;
    QString exe_path;
    bool ytdl_available = false;
    QString current_file_path_name;
    QString current_file_name;
    QString advance_status;
    double dl_progress;

signals:
    void process_ready();
    void process_state(QProcess::ProcessState);
    void ytdl_process_ended();
    void log(QString);
    void process_out_update();
};

#endif // YOUTUBEDL_PROCESS_H
