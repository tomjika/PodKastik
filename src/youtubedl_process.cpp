#include "youtubedl_process.h"

youtubedl_process::youtubedl_process(QWidget *parent, QString p) :
    QWidget(parent),
    exe_path(p)
{
    process = new QProcess();
        connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(process_out()));
        connect(process, SIGNAL(readyReadStandardError()), this, SLOT(process_err()));
        connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(process_state_changed(QProcess::ProcessState)));
        connect(process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(process_error_state(QProcess::ProcessError)));
        connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(process_finished(int, QProcess::ExitStatus)));

    process->setProgram(QFile(exe_path).exists() ? exe_path : "youtube-dl");
    process->setArguments({"--version"});

    use_portable = QFile(exe_path).exists();
    process->start(QIODevice::ReadWrite);
}
youtubedl_process::~youtubedl_process(){}
void youtubedl_process::process_out()
{bool ok;
    QByteArray out_str = process->readAllStandardOutput().simplified();
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
void youtubedl_process::process_err()
{
    QByteArray out_str = process->readAllStandardError().simplified();
    qDebug()<<"ytdl_err"<<out_str;
    emit log(out_str.trimmed());
    if(out_str.contains("ERROR:")) QMessageBox::information(this, "ERROR", out_str, QMessageBox::Ok);
}
void youtubedl_process::process_error_state(QProcess::ProcessError err){qDebug()<<"ytdl_err_state"<<err;}
void youtubedl_process::process_state_changed(QProcess::ProcessState s){emit process_state(s);}
void youtubedl_process::exe_process(QStringList args)
{
    process->setArguments(args);
    process->start(QIODevice::ReadWrite);
}
void youtubedl_process::process_finished(int code, QProcess::ExitStatus state)
{
    qDebug()<<"ytdl_FINISHED"<<code<<state;
    //emit process_ended();
}
