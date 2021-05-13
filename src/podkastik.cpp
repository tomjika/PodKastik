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

    QSettings my_settings("Studio1656", "PodKastik", this);
    ytdl_folder = my_settings.value("ytdl_folder").toString();
    ui->pb_browse_ytdl->setText("YT-dl Exe: "+ytdl_folder);
    youtube_dl = new youtubedl_process(this, ytdl_folder);
        connect(youtube_dl, SIGNAL(process_ready()), this, SLOT(loadSettings()));
        connect(youtube_dl, SIGNAL(process_out_update()), this, SLOT(ytdl_process_out()));
        connect(youtube_dl, SIGNAL(log(QString)), this, SLOT(logging(QString)));
        connect(youtube_dl, SIGNAL(process_state(QProcess::ProcessState)), this, SLOT(ytdl_state_changed(QProcess::ProcessState)));
        connect(youtube_dl, SIGNAL(process_ended()), this, SLOT(ytdl_finished()));

    ffmpeg_folder = my_settings.value("ffmpeg_folder").toString();
    ui->pb_browse_ffmpeg->setText("FFmpeg Exe: "+ffmpeg_folder);
    ffmpeg = new ffmpeg_process(this, ffmpeg_folder);
        connect(ffmpeg, SIGNAL(process_ready()), this, SLOT(loadSettings()));
        connect(ffmpeg, SIGNAL(process_out_update()), this, SLOT(ffmpeg_process_out()));
        connect(ffmpeg, SIGNAL(log(QString)), this, SLOT(logging(QString)));
        connect(ffmpeg, SIGNAL(process_state(QProcess::ProcessState)), this, SLOT(ffmpeg_state_changed(QProcess::ProcessState)));
        connect(ffmpeg, SIGNAL(process_ended()), this, SLOT(ffmpeg_finished()));

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

    if(youtube_dl->ytdl_available)
    {
        ui->pb_browse_ytdl->setText(youtube_dl->use_portable ? "YT-dl Exe: "+ytdl_folder : "Youtube-dl is installed");
        ui->pb_browse_ytdl->setToolTip("Version: "+youtube_dl->version);
    }else ui->pb_browse_ytdl->setText("Youtube-dl is not installed or .exe not found");

    if(ffmpeg->ffmpeg_available)
    {
        ui->pb_browse_ffmpeg->setText(ffmpeg->use_portable ? "FFmpeg Exe: "+ffmpeg_folder : "FFmpeg is installed");
        ui->pb_browse_ffmpeg->setToolTip("Version: "+ffmpeg->version);
    }else ui->pb_browse_ffmpeg->setText("FFmpeg is not installed or .exe not found");

    ffmpeg->speed_tempo = my_settings.value("speed_tempo").toDouble(&ok);
    ui->dsb_tempo->setValue(ffmpeg->speed_tempo);

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
    my_settings.setValue("speed_tempo", ffmpeg->speed_tempo);
    my_settings.setValue("stereo_to_mono", stereo_to_mono);
    my_settings.setValue("kbit", to_kbit);
}

/******************************************************************************************/
/*********************** YOUTUBE-DL********************************************************/
/******************************************************************************************/
void PodKastik::ytdl_process_out()
{
    ui->l_current_file_name->setText(youtube_dl->current_file_name);
    ui->l_output->setText(youtube_dl->advance_status);
    ui->pb_progress->setValue(youtube_dl->dl_progress);
    this->setWindowTitle("PodKastik | DL: "+QString::number(youtube_dl->dl_progress)+"%");
    ui->pb_progress->setFormat("Downloading...");

    if(youtube_dl->dl_progress == 100
       && youtube_dl->process->waitForFinished()
       && ui->cb_then->isEnabled()
       && ui->cb_then->isChecked())
    {
        ui->pb_progress->setValue(0);
        ui->pb_progress->setFormat("...");
        ui->l_output->setText("--");
        this->setWindowTitle("PodKastik | "+ui->l_output->text());
        if(ffmpeg->process->state() == QProcess::Running){ffmpeg->process->kill(); return;}
        do_ffmpeg(youtube_dl->current_file_path_name);
    }
}
void PodKastik::ytdl_state_changed(QProcess::ProcessState s)
{
    switch(s){
        case QProcess::NotRunning: ui->pb_download->setText("Paste and download"); break;
        case QProcess::Starting: ui->pb_download->setText("Stop download"); break;
        case QProcess::Running: ui->pb_download->setText("Stop download"); break;
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

    youtube_dl->exe_process(args);
}
void PodKastik::ytdl_finished(){}

/******************************************************************************************/
/*********************** FFmpeg ***********************************************************/
/******************************************************************************************/
void PodKastik::ffmpeg_process_out()
{
    ui->l_current_file_name->setText(output_file_name);
    ui->l_output->setText(ffmpeg->advance_status);
    ui->pb_progress->setValue(ffmpeg->conv_progress);
    this->setWindowTitle("PodKastik | C: "+QString::number(ffmpeg->conv_progress,'f',2)+"%");
    ui->pb_progress->setFormat("Converting...");
qDebug()<<ffmpeg->conv_progress<<ui->pb_progress->value();
    if(ffmpeg->conv_progress == 100
       && ffmpeg->process->waitForFinished())
        this->setWindowTitle("PodKastik | done!");
}
void PodKastik::ffmpeg_error_state(QProcess::ProcessError err){qDebug()<<"ffmpeg_err_state"<<err;}
void PodKastik::ffmpeg_state_changed(QProcess::ProcessState s)
{qDebug()<<"ffmpeg state: "<<s;
    switch(s){
        case QProcess::NotRunning: ui->pb_select_file_to_convert->setText("Select and convert"); ui->pb_download->setEnabled(true); break;
        case QProcess::Starting: ui->pb_select_file_to_convert->setText("Stop convert"); ui->pb_download->setEnabled(false); break;
        case QProcess::Running: ui->pb_select_file_to_convert->setText("Stop convert"); ui->pb_download->setEnabled(false); break;
    }
}
void PodKastik::do_ffmpeg(QString file_path_name)
{//sox -S -t mp3 -b 64 "$i" ./$fn2"X.mp3" channels 1 tempo $tmpo (S: show progress, t: filetype, -b: bitrate,
    output_file_name = file_path_name;
    output_file_name = output_file_name.left(output_file_name.lastIndexOf(".")).append("_eaready.mp3");
    qDebug()<<"current_file_name EXISTS"<<QFile::exists(youtube_dl->current_file_path_name);
    QStringList args;
    args<<"-progress"<<"pipe:1";
    args<<"-loglevel"<<"32";
    args<<"-y";//overwrite
    args<<"-i";
    args<<file_path_name;
    args<<"-ac"<<(stereo_to_mono ? "1" : "2");
    args<<"-ab"<<QString::number(to_kbit, 'f', 0).append("k");
    args<<"-acodec"<<"mp3";
//    args<<"-af"<<QString("dynaudnorm=f=150:g=15,atempo=").append(QString::number(speed_tempo, 'f', 2));
    args<<output_file_name;

    ffmpeg->exe_process(args);
}
void PodKastik::ffmpeg_finished()
{/*qDebug()<<"ffmpeg_FINISHED"<<code<<state;
    qDebug()<<"REMOVE"<<youtube_dl->current_file_path_name<<QFile::remove(youtube_dl->current_file_path_name);
    QString new_name = output_file_name;
    int start_name = new_name.lastIndexOf("\\")==-1 ? new_name.lastIndexOf("/") : new_name.lastIndexOf("\\");
    new_name.insert(start_name+1, "("+QTime(0,0,0,0).addMSecs(total_target_ms).toString("hh_mm_ss")+") ");
    qDebug()<<"output_file_name EXISTS"<<QFile::exists(current_file_name);
    qDebug()<<"RENAME"<<new_name<<QFile::rename(output_file_name, new_name);*/
}

/******************************************************************************************/
/*********************** VIEW *************************************************************/
/******************************************************************************************/
void PodKastik::on_pb_download_clicked()
{
    ui->pb_progress->setValue(0);
    ui->pb_progress->setFormat("...");
    ui->l_output->setText("--"); this->setWindowTitle("PodKastik | "+ui->l_output->text());
    //if(youtube_dl->state() == QProcess::Running){youtube_dl->kill(); return;}
    do_ytdl();
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
    QString str = QFileDialog::getOpenFileName(this, "Youtube-dl directory", ytdl_folder, "youtube-dl (*.exe)");
    if(!str.isNull()) ytdl_folder = str;
    ui->pb_browse_ytdl->setText("YT-dl Exe: "+ytdl_folder);
    ui->pb_browse_ytdl->setToolTip(ui->pb_browse_ytdl->text());
    saveSettings();
}
void PodKastik::on_pb_browse_ffmpeg_clicked()
{
    QString str = QFileDialog::getOpenFileName(this, "ffmpeg directory", ffmpeg_folder, "ffmpeg (*.exe)");
    if(!str.isNull()) ffmpeg_folder = str;
    ui->pb_browse_ffmpeg->setText("FFmpeg Exe: "+ffmpeg_folder);
    ui->pb_browse_ffmpeg->setToolTip(ui->pb_browse_ffmpeg->text());
    saveSettings();
}
void PodKastik::on_dsb_tempo_valueChanged(double arg1)
{
    ffmpeg->speed_tempo = arg1; saveSettings();
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
}
void PodKastik::on_pb_select_file_to_convert_clicked()
{
    if(ffmpeg->process->state() == QProcess::Running) //stop conversion
    {
        ffmpeg->process->kill();
        ui->pb_progress->setValue(0);
        ui->pb_progress->setFormat("...");
        ui->l_output->setText("Conversion stopped");
        this->setWindowTitle("PodKastik | "+ui->l_output->text());
        return;
    }
    current_file_name = QFileDialog::getOpenFileName(this, "Select file to convert", output_path);
    do_ffmpeg(current_file_name);
}
void PodKastik::on_pb_open_output_path_clicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(output_path));
}

/******************************************************************************************/
/*********************** OTHERS ***********************************************************/
/******************************************************************************************/
void PodKastik::logging(QString str)
{
    ui->te_log->appendPlainText(str.trimmed());
}
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
