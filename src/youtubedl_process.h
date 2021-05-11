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

private slots:
    void ytdl_process_out();
    void ytdl_process_err();
    void ytdl_state_changed(QProcess::ProcessState);
    void ytdl_error_state(QProcess::ProcessError);
    void ytdl_finished(int, QProcess::ExitStatus);
    void do_ytdl();

public:
    QProcess *ytdl_process;
    QString version = "";
    bool use_portable = false;
    QString exe_path;
    bool ytdl_available = false;

signals:
    process_ready();
    process_state(QProcess::ProcessState);
    log(QString);
};

#endif // YOUTUBEDL_PROCESS_H
