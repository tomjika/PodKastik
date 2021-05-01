#include "podkastik.h"
#include "ui_podkastik.h"

PodKastik::PodKastik(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PodKastik)
{
    ui->setupUi(this);
    this->setFixedHeight(165);

    clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()), this, SLOT(clip_paste()));
    dl_link = clipboard->text();
    clip_paste();

    youtube_dl = new QProcess();
        connect(youtube_dl, SIGNAL(readyReadStandardOutput()), this, SLOT(ytdl_process_out()));
        connect(youtube_dl, SIGNAL(readyReadStandardError()), this, SLOT(ytdl_process_err()));
        connect(youtube_dl, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(ytdl_state_changed(QProcess::ProcessState)));
        connect(youtube_dl, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(ytdl_error_state(QProcess::ProcessError)));
        connect(youtube_dl, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ytdl_finished(int, QProcess::ExitStatus)));

    ffmpeg = new QProcess();
        connect(ffmpeg, SIGNAL(readyReadStandardOutput()), this, SLOT(ffmpeg_process_out()));
        connect(ffmpeg, SIGNAL(readyReadStandardError()), this, SLOT(ffmpeg_process_err()));
        connect(ffmpeg, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(ffmpeg_state_changed(QProcess::ProcessState)));
        connect(ffmpeg, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(ffmpeg_error_state(QProcess::ProcessError)));
        connect(ffmpeg, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(ffmpeg_finished(int, QProcess::ExitStatus)));

    ui->sb_kbits->blockSignals(true);
    ui->cb_to_mono->blockSignals(true);
    ui->dsb_tempo->blockSignals(true);

    loadSettings();
}
PodKastik::~PodKastik(){delete ui;}
void PodKastik::loadSettings()
{bool ok;
    QSettings my_settings("Studio1656", "PodKastik", this);

    default_folder = my_settings.value("default_folder").toString();
    output_path = default_folder;
    ui->pb_browse_default->setText("Default: "+default_folder);
    ui->pb_browse->setText("Output path: "+default_folder);

    ytdl_folder = my_settings.value("ytdl_folder").toString();
    ui->pb_browse_ytdl->setText("YT-dl Exe: "+ytdl_folder);
    youtube_dl->setProgram(ytdl_folder);

    ffmpeg_folder = my_settings.value("ffmpeg_folder").toString();
    ui->pb_browse_ffmpeg->setText("FFMPEG Exe: "+ffmpeg_folder);
    ffmpeg->setProgram(ffmpeg_folder);

    speed_tempo = my_settings.value("speed_tempo").toDouble(&ok);
    ui->dsb_tempo->setValue(speed_tempo);

    stereo_to_mono = my_settings.value("stereo_to_mono").toBool();
    ui->cb_to_mono->setChecked(stereo_to_mono);

    to_kbit = my_settings.value("kbit").toDouble(&ok);
    ui->sb_kbits->setValue(to_kbit);

    ui->sb_kbits->blockSignals(false);
    ui->cb_to_mono->blockSignals(false);
    ui->dsb_tempo->blockSignals(false);
}
void PodKastik::saveSettings()
{qDebug()<<"saving?";
    QSettings my_settings("Studio1656", "PodKastik", this);
    my_settings.setValue("default_folder", default_folder);
    my_settings.setValue("ytdl_folder", ytdl_folder);
    my_settings.setValue("ffmpeg_folder", ffmpeg_folder);
    my_settings.setValue("speed_tempo", speed_tempo);
    my_settings.setValue("stereo_to_mono", stereo_to_mono);
    my_settings.setValue("kbit", to_kbit);
}

/******************************************************************************************/
/*********************** YOUTUBE-DL********************************************************/
/******************************************************************************************/
void PodKastik::ytdl_process_out()
{bool ok;
    QByteArray out_str;
    //if(youtube_dl->waitForReadyRead(30000))
        out_str = youtube_dl->readAllStandardOutput();
    out_str = out_str.simplified();
    qDebug()<<"ytdl_out"<<out_str;
    if(out_str.contains("[download]"))
    {//[download]  37.4% of 2.67MiB at 844.71KiB/s ETA 00:02
        if(out_str.contains("Destination")){
            current_file_name = QString::fromLocal8Bit(out_str.replace("[download] Destination: ", 0).trimmed());
            ui->l_current_file_name->setText(current_file_name.right(current_file_name.size()-current_file_name.lastIndexOf('\\')-1));
        qDebug()<<"current_file_name"<<current_file_name;}
        if(out_str.contains(" has already been downloaded")){
            current_file_name = QString::fromLocal8Bit(out_str.replace("[download] ", 0).replace(" has already been downloaded", 0).trimmed());
            ui->l_current_file_name->setText(current_file_name.right(current_file_name.size()-current_file_name.lastIndexOf('\\')-1));
            return;
        }
        ui->l_output->setText(out_str.replace("[download] ", 0).trimmed());
        ui->pb_progress->setValue(out_str.left(out_str.indexOf("%")).toDouble(&ok));
        this->setWindowTitle("PodKastik | DL: "+out_str.left(out_str.indexOf("%"))+"%");
        ui->pb_progress->setFormat("Downloading...");
    }else ui->te_log->appendPlainText(out_str.trimmed());
}
void PodKastik::ytdl_process_err()
{
    QByteArray out_str;
    //if(youtube_dl->waitForReadyRead(30000))
        out_str = youtube_dl->readAllStandardError();
    out_str = out_str.simplified();
    qDebug()<<"ytdl_err"<<out_str;
    ui->te_log->appendPlainText(out_str.trimmed());
    if(out_str.contains("ERROR:")) QMessageBox::information(this, "ERROR", out_str, QMessageBox::Ok);
}
void PodKastik::ytdl_error_state(QProcess::ProcessError err){qDebug()<<"ytdl_err_state"<<err;}
void PodKastik::ytdl_state_changed(QProcess::ProcessState s)
{
    switch(s){
        case QProcess::NotRunning: ui->pb_download->setText("Paste and download"); ui->pb_convert->setEnabled(true); break;
        case QProcess::Starting: ui->pb_download->setText("Stop download"); ui->pb_convert->setEnabled(false); break;
        case QProcess::Running: ui->pb_download->setText("Stop download"); ui->pb_convert->setEnabled(false); break;
    }
}
void PodKastik::do_ytdl()
{
    dl_link = clipboard->text();
    //dl_link = "https://www.youtube.com/watch?v=n4CjtoNoQ1Q";
    if(!dl_link.contains("http"))dl_link.prepend("https://");
    if(!urlExists(dl_link)){QMessageBox::information(this, "Error", "Invalid url: "+dl_link, QMessageBox::Ok); return;}

    QStringList args;
    if(ui->rb_audio->isChecked()) args<<"-x"/*<<"--audio-format"<<"mp3"<<"--ffmpeg-location"<<ffmpeg_folder*/;
    if(!ui->cb_playlist->isChecked())args<<"--playlist-items"<<"1";
    args<<dl_link;
    args<<"--restrict-filenames";
    args<<"-o";
    args<<output_path+"/%(title)s.%(ext)s";
    youtube_dl->setArguments(args);
    youtube_dl->start(QIODevice::ReadWrite);
}
void PodKastik::ytdl_finished(int code, QProcess::ExitStatus state)
{qDebug()<<"ytdl_FINISHED"<<code<<state;

    //qDebug()<<"RENAME"<<QFile(current_file_name).isOpen()<<QFile::rename(current_file_name, current_file_name.insert(current_file_name.lastIndexOf('.'), 'X'));
    if(ui->cb_then->isEnabled() && ui->cb_then->isChecked())ui->pb_convert->click();
}

/******************************************************************************************/
/*********************** FFMPEG ***********************************************************/
/******************************************************************************************/
void PodKastik::ffmpeg_process_out()
{bool ok;
    QRegularExpression sep("[:.]");
    QByteArray out_str;
    if(ffmpeg->waitForReadyRead(30000))
        out_str = ffmpeg->readAllStandardOutput();
    out_str = out_str.simplified();
    qDebug()<<"ffmpeg_out"<<out_str;

    if(out_str.contains("out_time="))
    {
        ui->pb_progress->setFormat("Converting...");
        QString dur_str = out_str.right(out_str.count()-out_str.indexOf("out_time=")-9);
        QStringList dur_list = dur_str.left(dur_str.indexOf(" dup_frames")).split(sep);
        QTime duration(dur_list[0].toInt(), dur_list[1].toInt(), dur_list[2].toInt(), dur_list[3].toInt()/1000);
        current_ms_ffmpeg = QTime(0,0,0,0).msecsTo(duration);
        ui->pb_progress->setValue(100.0*(double)current_ms_ffmpeg/(total_target_ms!=0 ? (double)total_target_ms : 1.0));
        this->setWindowTitle("PodKastik | C: "+QString::number(100.0*(double)current_ms_ffmpeg/(total_target_ms!=0 ? (double)total_target_ms : 1.0),'f',2)+"%");

        QString speed_factor = out_str.right(out_str.count()-out_str.indexOf("speed=")-6).simplified();
        speed_factor.truncate(speed_factor.indexOf("x"));

        QTime eta = QTime(0,0,0,0).addMSecs((total_target_ms-current_ms_ffmpeg)/speed_factor.toDouble(&ok));
        qDebug()<<eta<<"speed_factor"<<speed_factor<<QTime(0,0,0,0).msecsTo(eta);
        ui->l_output->setText("Conversion ETA: "+eta.toString("hh:mm:ss"));
        if(out_str.contains("progress=end") || QTime(0,0,0,0).msecsTo(eta)<2000)// || ui->l_output->text()=="Conversion ETA: 00:00:00" || ui->l_output->text()=="Conversion ETA: 00:00:01" || ui->l_output->text()=="Conversion ETA: 00:00:02")
        {
            ffmpeg->waitForFinished(5);
            //qDebug()<<"REMOVE"<<current_file_name<<QFile::remove(current_file_name);
            //QFile::rename(output_file_name, current_file_name);
            ui->pb_progress->setValue(ui->pb_progress->maximum());
            ui->l_output->setText("Conversion finished!"); this->setWindowTitle("PodKastik | done!");
        }
    }
}
void PodKastik::ffmpeg_process_err()
{
    QByteArray out_str;
    if(ffmpeg->waitForReadyRead(300))
        out_str = ffmpeg->readAllStandardError();
    out_str = out_str.simplified();
    qDebug()<<"FFMPEG_ERR:"<<out_str;
    QRegularExpression sep("[:.]");
    if(out_str.contains("Duration"))
    {
        QString dur_str = out_str.right(out_str.count()-out_str.indexOf("Duration: ")-10);
        QStringList dur_list = dur_str.left(dur_str.indexOf(",")).split(sep); if(dur_list.size()<4)return;
        current_file_duration = QTime(dur_list[0].toInt(), dur_list[1].toInt(), dur_list[2].toInt(), dur_list[3].toInt()*10);
        total_target_ms = QTime(0,0,0,0).msecsTo(current_file_duration)/speed_tempo;
        //qDebug()<<current_file_duration<<total_target_ms;
    }
    else if(out_str.contains("size="))
    {//size=     806kB time=00:01:43.08 bitrate=  64.1kbits/s speed=41.1x
    }else ui->te_log->appendPlainText(out_str.trimmed());
}
void PodKastik::ffmpeg_error_state(QProcess::ProcessError err){qDebug()<<"ffmpeg_err_state"<<err;}
void PodKastik::ffmpeg_state_changed(QProcess::ProcessState s)
{qDebug()<<"ffmpeg state: "<<s;
    switch(s){
        case QProcess::NotRunning: ui->pb_convert->setText("Convert"); ui->pb_download->setEnabled(true); break;
        case QProcess::Starting: ui->pb_convert->setText("Stop convert"); ui->pb_download->setEnabled(false); break;
        case QProcess::Running: ui->pb_convert->setText("Stop convert"); ui->pb_download->setEnabled(false); break;
    }
}
void PodKastik::do_ffmpeg()
{//sox -S -t mp3 -b 64 "$i" ./$fn2"X.mp3" channels 1 tempo $tmpo (S: show progress, t: filetype, -b: bitrate,
    //ffmpeg->setProgram(ffmpeg_folder);
    output_file_name = current_file_name;
    output_file_name = output_file_name.left(output_file_name.lastIndexOf(".")).append("_eaready.mp3");
    qDebug()<<"current_file_name EXISTS"<<QFile::exists(current_file_name);
    QStringList args;
    args<<"-progress"<<"pipe:1";
    args<<"-loglevel"<<"32";
    args<<"-y";//overwrite
    args<<"-i";
    args<<current_file_name;
    args<<"-ac"<<(stereo_to_mono ? "1" : "2");
    args<<"-ab"<<QString::number(to_kbit, 'f', 0).append("k");
    args<<"-acodec"<<"mp3";
    args<<"-af"<<QString("dynaudnorm=f=150:g=15,atempo=").append(QString::number(speed_tempo, 'f', 2));
    args<<output_file_name;
    ffmpeg->setArguments(args);
    qDebug()<<ffmpeg->arguments()<<ffmpeg->program();
    ffmpeg->start(QIODevice::ReadWrite);
}
void PodKastik::ffmpeg_finished(int code, QProcess::ExitStatus state)
{qDebug()<<"ffmpeg_FINISHED"<<code<<state;
    qDebug()<<"REMOVE"<<current_file_name<<QFile::remove(current_file_name);
    QString new_name = output_file_name;
    int start_name = new_name.lastIndexOf("\\")==-1 ? new_name.lastIndexOf("/") : new_name.lastIndexOf("\\");
    new_name.insert(start_name+1, "("+QTime(0,0,0,0).addMSecs(total_target_ms).toString("hh_mm_ss")+") ");
    qDebug()<<"output_file_name EXISTS"<<QFile::exists(current_file_name);
    qDebug()<<"RENAME"<<new_name<<QFile::rename(output_file_name, new_name);
}

/******************************************************************************************/
/*********************** VIEW *************************************************************/
/******************************************************************************************/
void PodKastik::on_pb_download_clicked()
{
    ui->pb_progress->setValue(0);
    ui->pb_progress->setFormat("...");
    ui->l_output->setText("--"); this->setWindowTitle("PodKastik | "+ui->l_output->text());
    if(youtube_dl->state() == QProcess::Running){youtube_dl->kill(); return;}
    do_ytdl();
}
void PodKastik::on_pb_convert_clicked()
{
    ui->pb_progress->setValue(0);
    ui->pb_progress->setFormat("...");
    ui->l_output->setText("--"); this->setWindowTitle("PodKastik | "+ui->l_output->text());
    if(ffmpeg->state() == QProcess::Running){ffmpeg->kill(); return;}
    do_ffmpeg();
}
void PodKastik::on_pb_browse_clicked()
{
    QString p = QFileDialog::getExistingDirectory(this, "Output directory", output_path, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    output_path = p.isEmpty() ? output_path : p;
    ui->pb_browse->setText("Output path: "+output_path);
}
void PodKastik::on_pb_browse_pressed(){ui->l_path->setFrameShadow(QFrame::Sunken);}
void PodKastik::on_pb_browse_released(){ui->l_path->setFrameShadow(QFrame::Raised);}
void PodKastik::on_pb_settings_clicked(){this->setFixedHeight(this->height()<200 ? 400 : 165);}
void PodKastik::on_pb_browse_default_clicked()
{
    default_folder = QFileDialog::getExistingDirectory(this, "Default directory", default_folder, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->pb_browse_default->setText("Default: "+default_folder);
    ui->pb_browse_default->setToolTip(ui->pb_browse_default->text());
    saveSettings();
}
void PodKastik::on_pb_browse_ytdl_clicked()
{
    ytdl_folder = QFileDialog::getOpenFileName(this, "Youtube-dl directory", ytdl_folder, "youtube-dl (*.exe)");
    ui->pb_browse_ytdl->setText("YT-dl Exe: "+ytdl_folder);
    ui->pb_browse_ytdl->setToolTip(ui->pb_browse_ytdl->text());
    saveSettings();
}
void PodKastik::on_pb_browse_ffmpeg_clicked()
{
    ffmpeg_folder = QFileDialog::getOpenFileName(this, "ffmpeg directory", ffmpeg_folder, "ffmpeg (*.exe)");
    ui->pb_browse_ffmpeg->setText("FFMPEG Exe: "+ffmpeg_folder);
    ui->pb_browse_ffmpeg->setToolTip(ui->pb_browse_ffmpeg->text());
    saveSettings();
}
void PodKastik::on_dsb_tempo_valueChanged(double arg1)
{
    speed_tempo = arg1; saveSettings();
}
void PodKastik::on_sb_kbits_valueChanged(double arg1)
{
    to_kbit = arg1; saveSettings();
}
void PodKastik::on_cb_to_mono_stateChanged(int arg1)
{
    stereo_to_mono = arg1; saveSettings();
}
void PodKastik::on_rb_video_toggled(bool checked)
{
    ui->cb_then->setEnabled(!checked);
    ui->pb_convert->setEnabled(!checked);
}
void PodKastik::on_pb_select_file_to_convert_clicked()
{
    current_file_name = QFileDialog::getOpenFileName(this, "Select file to convert", output_path);
    ui->pb_convert->click();
}
void PodKastik::on_pb_open_output_path_clicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(output_path));
}

/******************************************************************************************/
/*********************** OTHERS ***********************************************************/
/******************************************************************************************/
void PodKastik::clip_paste(){ui->pb_download->setToolTip(clipboard->text());}
bool PodKastik::urlExists(QString url_string)
{
    QUrl url(url_string);
    QTextStream out(stdout);
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
