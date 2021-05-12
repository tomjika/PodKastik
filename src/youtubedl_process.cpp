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

    ytdl_process->setProgram(QFile(exe_path).exists() ? exe_path : "youtube-dl");
    ytdl_process->setArguments({"--version"});

    use_portable = QFile(exe_path).exists();
    ytdl_process->start(QIODevice::ReadWrite);
}
youtubedl_process::~youtubedl_process(){}
void youtubedl_process::ytdl_process_out()
{bool ok;
    QByteArray out_str = ytdl_process->readAllStandardOutput().simplified();
    qDebug()<<"ytdl_out"<<out_str;

    if(QString(out_str).contains(QRegularExpression("[12]\\d{3}.(0[1-9]|1[0-2]).(0[1-9]|[12]\\d|3[01])")))
    {
        version = out_str;
        ytdl_available = true;
        emit process_ready();
    }
    else if(out_str.contains("[download]"))//[download]  37.4% of 2.67MiB at 844.71KiB/s ETA 00:02
    {
        if(out_str.contains("Destination")){
            current_file_path_name = QString::fromLocal8Bit(out_str.replace("[download] Destination: ", 0).trimmed());
            current_file_name = current_file_name.right(current_file_name.size()-current_file_name.lastIndexOf('\\')-1);
        }
        else if(out_str.contains(" has already been downloaded")){
            current_file_path_name = QString::fromLocal8Bit(out_str.replace("[download] ", 0).replace(" has already been downloaded", 0).trimmed());
            current_file_name = current_file_name.right(current_file_name.size()-current_file_name.lastIndexOf('\\')-1);
        }else{
            advance_status = out_str.replace("[download] ", 0).trimmed();
            dl_progress = out_str.left(out_str.indexOf("%")).toDouble(&ok);
        }
        emit process_out_update();
    }else emit log(out_str.trimmed());
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
void youtubedl_process::exe_process(QStringList args)
{
    ytdl_process->setArguments(args);
    ytdl_process->start(QIODevice::ReadWrite);
}
void youtubedl_process::ytdl_finished(int code, QProcess::ExitStatus state)
{
    qDebug()<<"ytdl_FINISHED"<<code<<state;
    //emit ytdl_process_ended();
}
