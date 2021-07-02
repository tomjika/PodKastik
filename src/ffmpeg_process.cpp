#include "ffmpeg_process.h"

ffmpeg_process::ffmpeg_process(QWidget *parent, QString p) :
    QWidget(parent),
    exe_path(p)
{
    process = new QProcess();
        connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(process_out()));
        connect(process, SIGNAL(readyReadStandardError()), this, SLOT(process_err()));
        connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(process_state_changed(QProcess::ProcessState)));
        connect(process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(process_error_state(QProcess::ProcessError)));
        connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(process_finished(int, QProcess::ExitStatus)));

    init_timer = new QTimer(this);
    init_timer->setSingleShot(true);
    connect(init_timer, &QTimer::timeout, this, &ffmpeg_process::initialize_process);
}
ffmpeg_process::~ffmpeg_process()
{}
void ffmpeg_process::initialize_process()
{
    process->setProgram(QFile(exe_path).exists() ? exe_path : "ffmpeg");
    process->setArguments({"-version"});

    use_portable = QFile(exe_path).exists();
    process->start(QIODevice::ReadWrite);
}
void ffmpeg_process::exe_process(QString file_name)
{qDebug()<<"ffmpeg exe process"<<file_name;
    QStringList args;
    args<<"-progress"<<"pipe:1";
    args<<"-loglevel"<<"32";
    args<<"-y";//overwrite
    args<<"-i";
    args<<file_name;
    args<<"-ac"<<(stereo_to_mono ? "1" : "2");
    args<<"-ab"<<QString::number(to_kbit, 'f', 0).append("k");
    args<<"-acodec"<<"mp3";
    args<<"-af"<<QString("dynaudnorm=f=150:g=15,atempo=").append(QString::number(speed_tempo, 'f', 2));
    args<<output_file_name;

    process->setArguments(args);
    qDebug()<<process->arguments()<<process->program();
    process->start(QIODevice::ReadWrite);
}
void ffmpeg_process::process_out()
{bool ok;
    QRegularExpression sep("[:.]");
    QByteArray out_str = process->readAllStandardOutput().simplified();
    qDebug()<<"ffmpeg_out"<<out_str;

    if(QString(out_str).contains("version"))
    {
        version = out_str.remove(0, QString("ffmpeg version ").size());
        version.truncate(10);
        available = true;
        emit process_ready(available);
    }
    else if(out_str.contains("out_time="))
    {
        QString dur_str = out_str.right(out_str.count()-out_str.indexOf("out_time=")-9);
        QStringList dur_list = dur_str.left(dur_str.indexOf(" dup_frames")).split(sep);
        QTime duration(dur_list[0].toInt(), dur_list[1].toInt(), dur_list[2].toInt(), dur_list[3].toInt()/1000);
        current_ms_ffmpeg = QTime(0,0,0,0).msecsTo(duration);
        conv_progress = 100.0*(double)current_ms_ffmpeg;
        conv_progress = conv_progress/(total_target_ms!=0 ? (double)total_target_ms : 1.0);//not working on linux in one line

        QString speed_factor = out_str.right(out_str.count()-out_str.indexOf("speed=")-6).simplified();
        speed_factor.truncate(speed_factor.indexOf("x"));

        QTime eta = QTime(0,0,0,0).addMSecs((total_target_ms-current_ms_ffmpeg)/speed_factor.toDouble(&ok));
        //qDebug()<<conv_progress<<eta<<"speed_factor"<<speed_factor<<QTime(0,0,0,0).msecsTo(eta);
        advance_status = "Conversion ETA: "+eta.toString("hh:mm:ss");
        if(out_str.contains("progress=end") || QTime(0,0,0,0).msecsTo(eta)<1000)// || ui->l_output->text()=="Conversion ETA: 00:00:00" || ui->l_output->text()=="Conversion ETA: 00:00:01" || ui->l_output->text()=="Conversion ETA: 00:00:02")
        {
            process->waitForFinished(5);
            conv_progress = 100;
            advance_status = "Conversion finished!";
        }
        emit process_out_update();
    }
}
void ffmpeg_process::process_err()
{
    QByteArray out_str;
    if(process->waitForReadyRead(300))
        out_str = process->readAllStandardError().simplified();
    //QByteArray out_str = process->readAllStandardError().simplified();
    //qDebug()<<"FFmpeg_ERR:"<<out_str;
    QRegularExpression sep("[:.]");
    if(out_str.contains("Duration"))
    {
        QString dur_str = out_str.right(out_str.count()-out_str.indexOf("Duration: ")-10);
        QStringList dur_list = dur_str.left(dur_str.indexOf(",")).split(sep); if(dur_list.size()<4)return;
        current_file_duration = QTime(dur_list[0].toInt(), dur_list[1].toInt(), dur_list[2].toInt(), dur_list[3].toInt()*10);
        total_target_ms = QTime(0,0,0,0).msecsTo(current_file_duration)/speed_tempo;
    }
    else if(out_str.contains("size=")){/*size=     806kB time=00:01:43.08 bitrate=  64.1kbits/s speed=41.1x*/}
    else emit log(out_str.trimmed());
}
void ffmpeg_process::process_error_state(QProcess::ProcessError err)
{qDebug()<<"ffmpeg_err_state"<<err<<process->errorString();
    switch(err){
        case QProcess::FailedToStart:
            available = false;
            emit process_ready(available);
            init_timer->start(1000);
        break;
    }
}
void ffmpeg_process::process_state_changed(QProcess::ProcessState s)
{qDebug()<<"ffmpeg state: "<<s;
    switch(s){
        case QProcess::NotRunning: running = false; break;
        case QProcess::Starting: running = true; break;
        case QProcess::Running: running = true; break;
    }
    if(available) emit process_running(running);
}
void ffmpeg_process::process_finished(int code, QProcess::ExitStatus state)
{qDebug()<<"ffmpeg_FINISHED"<<code<<state;
    if(conv_progress >= 100)
        emit conversion_finished();
}
