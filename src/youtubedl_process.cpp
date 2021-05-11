#include "youtubedl_process.h"

youtubedl_process::youtubedl_process(QWidget *parent, QString p) :
    QWidget(parent),
    exe_path(p)
{
    ytdl_process = new QProcess();
        connect(ytdl_process, SIGNAL(readyReadStandardOutput()), this, SLOT(ytdl_process_out()));
        connect(ytdl_process, SIGNAL(readyReadStandardError()), this, SLOT(ytdl_process_err()));
        connect(ytdl_process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(ytdl_state_changed(QProcess::ProcessState)));
        connect(ytdl_process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(ytdl_error_state(QProcess::ProcessError)));
        connect(ytdl_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ytdl_finished(int, QProcess::ExitStatus)));

    ytdl_process->setProgram("youtube-dl");
    ytdl_process->setArguments({"--version"});
    if(ytdl_process->startDetached()) use_portable = false;//linux test
    else if(QFile(exe_path).exists())
    {
        ytdl_process->setProgram(exe_path);
        use_portable = true;
        ytdl_process->start();
    }else emit process_ready();
}
youtubedl_process::~youtubedl_process(){}
void youtubedl_process::ytdl_process_out()
{
    QByteArray out_str = ytdl_process->readAllStandardOutput().simplified();
    qDebug()<<"ytdl_out"<<out_str;
    if(QString(out_str).contains(QRegularExpression("[12]\\d{3}.(0[1-9]|1[0-2]).(0[1-9]|[12]\\d|3[01])")))
    {version = out_str; ytdl_available = true; emit process_ready();}
}
void youtubedl_process::ytdl_process_err()
{
    QByteArray out_str = ytdl_process->readAllStandardError().simplified();
    qDebug()<<"ytdl_err"<<out_str;
    emit log(out_str.trimmed());
    if(out_str.contains("ERROR:")) QMessageBox::information(this, "ERROR", out_str, QMessageBox::Ok);
}
void youtubedl_process::ytdl_error_state(QProcess::ProcessError err){qDebug()<<"ytdl_err_state"<<err;}
void youtubedl_process::ytdl_state_changed(QProcess::ProcessState s){emit process_state(s);}
void youtubedl_process::do_ytdl()
{
}
void youtubedl_process::ytdl_finished(int code, QProcess::ExitStatus state)
{qDebug()<<"ytdl_FINISHED"<<code<<state;
}
