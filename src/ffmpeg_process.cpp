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

    process->setProgram(QFile(exe_path).exists() ? exe_path : "ffmpeg");
    process->setArguments({"-version"});

    use_portable = QFile(exe_path).exists();
    qDebug()<<process->arguments()<<process->program()<<use_portable;
    process->start(QIODevice::ReadWrite);
}
ffmpeg_process::~ffmpeg_process(){}
void ffmpeg_process::process_out()
{bool ok;
    QRegularExpression sep("[:.]");
    QByteArray out_str = process->readAllStandardOutput().simplified();
    qDebug()<<"ffmpeg_out"<<out_str;

    if(QString(out_str).contains("version"))
    {
        version = out_str.remove(0, QString("ffmpeg version ").size());
        version.truncate(10);
        ffmpeg_available = true;
        emit process_ready();
    }
    else if(out_str.contains("out_time="))
    {
        QString dur_str = out_str.right(out_str.count()-out_str.indexOf("out_time=")-9);
        QStringList dur_list = dur_str.left(dur_str.indexOf(" dup_frames")).split(sep);
        QTime duration(dur_list[0].toInt(), dur_list[1].toInt(), dur_list[2].toInt(), dur_list[3].toInt()/1000);
        current_ms_ffmpeg = QTime(0,0,0,0).msecsTo(duration);
        conv_progress = 100.0*(double)current_ms_ffmpeg/(total_target_ms!=0 ? (double)total_target_ms : 1.0);

        QString speed_factor = out_str.right(out_str.count()-out_str.indexOf("speed=")-6).simplified();
        speed_factor.truncate(speed_factor.indexOf("x"));

        QTime eta = QTime(0,0,0,0).addMSecs((total_target_ms-current_ms_ffmpeg)/speed_factor.toDouble(&ok));
        qDebug()<<conv_progress<<eta<<"speed_factor"<<speed_factor<<QTime(0,0,0,0).msecsTo(eta);
        advance_status = "Conversion ETA: "+eta.toString("hh:mm:ss");
        if(out_str.contains("progress=end") || QTime(0,0,0,0).msecsTo(eta)<1000)// || ui->l_output->text()=="Conversion ETA: 00:00:00" || ui->l_output->text()=="Conversion ETA: 00:00:01" || ui->l_output->text()=="Conversion ETA: 00:00:02")
        {
            process->waitForFinished(5);
            //qDebug()<<"REMOVE"<<current_file_name<<QFile::remove(current_file_name);
            //QFile::rename(output_file_name, current_file_name);
            advance_status = "Conversion finished!";
        }
        emit process_out_update();
    }
}
void ffmpeg_process::process_err()
{
    QByteArray out_str = process->readAllStandardError().simplified();
    qDebug()<<"FFmpeg_ERR:"<<out_str;
    QRegularExpression sep("[:.]");
    if(out_str.contains("Duration"))
    {
        QString dur_str = out_str.right(out_str.count()-out_str.indexOf("Duration: ")-10);
        QStringList dur_list = dur_str.left(dur_str.indexOf(",")).split(sep); if(dur_list.size()<4)return;
        current_file_duration = QTime(dur_list[0].toInt(), dur_list[1].toInt(), dur_list[2].toInt(), dur_list[3].toInt()*10);
        total_target_ms = QTime(0,0,0,0).msecsTo(current_file_duration)/speed_tempo;
        qDebug()<<current_file_duration<<total_target_ms<<speed_tempo;
    }
    else if(out_str.contains("size="))
    {//size=     806kB time=00:01:43.08 bitrate=  64.1kbits/s speed=41.1x
    }else emit log(out_str.trimmed());
}
void ffmpeg_process::process_error_state(QProcess::ProcessError err){qDebug()<<"ffmpeg_err_state"<<err;}
void ffmpeg_process::process_state_changed(QProcess::ProcessState s){emit process_state(s);}
void ffmpeg_process::exe_process(QStringList args)
{
    args<<"-af"<<QString("dynaudnorm=f=150:g=15,atempo=").append(QString::number(speed_tempo, 'f', 2));
    process->setArguments(args);
    qDebug()<<process->arguments()<<process->program();
    process->start(QIODevice::ReadWrite);
}
void ffmpeg_process::process_finished(int code, QProcess::ExitStatus state)
{qDebug()<<"ffmpeg_FINISHED"<<code<<state;
    /*qDebug()<<"REMOVE"<<youtube_dl->current_file_path_name<<QFile::remove(youtube_dl->current_file_path_name);
    QString new_name = output_file_name;
    int start_name = new_name.lastIndexOf("\\")==-1 ? new_name.lastIndexOf("/") : new_name.lastIndexOf("\\");
    new_name.insert(start_name+1, "("+QTime(0,0,0,0).addMSecs(total_target_ms).toString("hh_mm_ss")+") ");
    qDebug()<<"output_file_name EXISTS"<<QFile::exists(current_file_name);
    qDebug()<<"RENAME"<<new_name<<QFile::rename(output_file_name, new_name);*/
}
