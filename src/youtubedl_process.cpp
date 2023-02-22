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

    use_portable = QFile(exe_path).exists();

    //why is the use of this timer ?
    init_timer = new QTimer(this);
    init_timer->setSingleShot(true);
    //connect(init_timer, &QTimer::timeout, this, &youtubedl_process::initialize_process);
}

youtubedl_process::~youtubedl_process()
{
    process->kill();
}

void youtubedl_process::initialize_process(bool use_patched)
{
    QString ytdl_to_use = use_patched ? "ytdl-patched" : "youtube-dl";

    process->setProgram(QFile(exe_path).exists() ? exe_path : ytdl_to_use);
    process->setArguments({"--version"});
    qDebug()<<process->program();
    process->start(QIODevice::ReadWrite);
}

void youtubedl_process::exe_process(QString dl_link, QString outputFolder, bool isPlaylist, bool audioOnly)
{
    if(dl_link.isEmpty())
    {
        QMessageBox::information(this, "Error", "Clipboard empty", QMessageBox::Ok);
        return;
    }
    //dl_link = "https://www.youtube.com/watch?v=dQw4w9WgXcQ";

    if(!dl_link.contains("http")) dl_link.prepend("https://");

    if(!urlExists(dl_link))
    {
        QMessageBox::information(this, "Error", "Invalid url: "+dl_link, QMessageBox::Ok);
        return;
    }

    dl_progress = 0;
    advance_status = "";

    QStringList args;
    if(audioOnly) args<<"-x";//<<"--audio-format"<<"mp3"<<"--ffmpeg-location"<<"F:/Dropbox/A_Podcast/ffmpeg.exe"; not use because stucking the program
    if(!isPlaylist) args<<"--playlist-items"<<"1";
    args<<dl_link;
    args<<"--restrict-filenames";
    args<<"-o";
    args<<outputFolder+"/%(title)s.%(ext)s";

    process->setArguments(args);
    process->start(QIODevice::ReadWrite);
}

void youtubedl_process::process_out()
{
    QByteArray out_str = process->readAllStandardOutput().simplified(); qDebug()<<"ytdl_out"<<out_str;

    if(QString(out_str).contains(QRegularExpression("[12]\\d{3}.(0[1-9]|1[0-2]).(0[1-9]|[12]\\d|3[01])")))
    {
        version = out_str;
        available = true;
        emit process_ready(available);
    }
    else if(out_str.startsWith("[download]"))//[download]  37.4% of 2.67MiB at 844.71KiB/s ETA 00:02
    {
        if(out_str.contains("Destination"))
        {
            if(out_str.contains("100%"))
                return;// skip last ytdl output

            out_str.replace("[download] Destination: ", "");
            if(out_str.indexOf("[download]") != -1)
                out_str.truncate(out_str.indexOf("[download]"));

            current_file_path_name = QString::fromLocal8Bit(out_str.trimmed());
            current_file_path_name.replace(".webm", ".opus");

            current_file_name = current_file_path_name.right(current_file_path_name.size() - current_file_path_name.lastIndexOf('/') - 1);
        }
        else if(out_str.contains(" has already been downloaded"))
        {
            current_file_path_name = QString::fromLocal8Bit(out_str.replace("[download] ", 0).replace(" has already been downloaded", 0).trimmed());
            current_file_name = current_file_path_name.right(current_file_path_name.size()-current_file_path_name.lastIndexOf('\\')-1);
        }
        else
        {
            advance_status = out_str.replace("[download] ", 0).trimmed();
            dl_progress = out_str.left(out_str.indexOf("%")).toDouble();
        }
        emit process_out_update();
        //if(dl_progress == 100 && process->waitForFinished()) emit download_finished();
    }
    else
    {
        emit log(out_str.trimmed());
        emit process_out_update();
    }
}

void youtubedl_process::process_err()
{
    QByteArray out_str = process->readAllStandardError().simplified(); qDebug()<<"ytdl_err"<<out_str;

    emit log(out_str.trimmed());

    if(out_str.contains("ERROR:"))
        QMessageBox::information(this, "ERROR", out_str, QMessageBox::Ok);
}

void youtubedl_process::process_error_state(QProcess::ProcessError err)
{qDebug()<<"ytdl_err_state: "<<err;

    switch(err){
        case QProcess::FailedToStart:
            available = false;
            emit process_ready(available);
            init_timer->start(1000);
        break;
    }
}

void youtubedl_process::process_state_changed(QProcess::ProcessState s)
{
    switch(s){
        case QProcess::NotRunning: running = false; break;
        case QProcess::Starting: running = true; break;
        case QProcess::Running: running = true; break;
    }

    if(available) emit process_running(running);
}

void youtubedl_process::process_finished(int code, QProcess::ExitStatus state)
{
    qDebug()<<"ytdl_FINISHED"<<code<<state;
    emit download_finished();
}

bool youtubedl_process::urlExists(QString url_string)
{
    QUrl url(url_string);
    //QTextStream out(stdout);
    QTcpSocket socket;
    QByteArray buffer;
    socket.connectToHost(url.host(), 80);
    if(socket.waitForConnected()) {
        socket.write("GET / HTTP/1.1\r\n""host: " + url.host().toUtf8() + "\r\n\r\n");
        if(socket.waitForReadyRead()){
            while(socket.bytesAvailable()){
                buffer.append(socket.readAll());
                int packetSize=buffer.size();
                while(packetSize>0){
                    if(buffer.contains("200 OK") || buffer.contains("302 Found") || buffer.contains("301 Moved")) return true;
                    buffer.remove(0,packetSize);
                    packetSize=buffer.size();
                }
            }
        }
    }return false;
}
