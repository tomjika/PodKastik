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
        connect(youtube_dl, SIGNAL(download_finished()), this, SLOT(ytdl_finished()));

    ffmpeg_folder = my_settings.value("ffmpeg_folder").toString();
    ui->pb_browse_ffmpeg->setText("FFmpeg Exe: "+ffmpeg_folder);
    ffmpeg = new ffmpeg_process(this, ffmpeg_folder);
        connect(ffmpeg, SIGNAL(process_ready()), this, SLOT(loadSettings()));
        connect(ffmpeg, SIGNAL(process_out_update()), this, SLOT(ffmpeg_process_out()));
        connect(ffmpeg, SIGNAL(log(QString)), this, SLOT(logging(QString)));
        connect(ffmpeg, SIGNAL(process_state(QProcess::ProcessState)), this, SLOT(ffmpeg_state_changed(QProcess::ProcessState)));
        connect(ffmpeg, SIGNAL(conversion_finished()), this, SLOT(ffmpeg_finished()));
}
PodKastik::~PodKastik(){delete ui;}
void PodKastik::loadSettings()
{bool ok;
    QSettings my_settings("Studio1656", "PodKastik", this);

    default_folder = my_settings.value("default_folder").toString();
    output_path = default_folder;
    youtube_dl->output_folder = output_path;
    ui->pb_browse_default->setText("Default: "+default_folder);
    ui->pb_browse->setText("Output path: "+default_folder);

    if(youtube_dl->available)
    {
        ui->pb_browse_ytdl->setText(youtube_dl->use_portable ? "YT-dl Exe: "+ytdl_folder : "Youtube-dl is installed");
        ui->pb_browse_ytdl->setToolTip("Version: "+youtube_dl->version);
        ui->pb_download->setEnabled(youtube_dl->available);
    }else ui->pb_browse_ytdl->setText("Youtube-dl is not installed or .exe not found");

    if(ffmpeg->available)
    {
        ui->pb_browse_ffmpeg->setText(ffmpeg->use_portable ? "FFmpeg Exe: "+ffmpeg_folder : "FFmpeg is installed");
        ui->pb_browse_ffmpeg->setToolTip("Version: "+ffmpeg->version);
        ui->pb_select_file_to_convert->setEnabled(ffmpeg->available);
    }else ui->pb_browse_ffmpeg->setText("FFmpeg is not installed or .exe not found");

    ffmpeg->speed_tempo = my_settings.value("speed_tempo").toDouble(&ok);
    ui->dsb_tempo->setValue(ffmpeg->speed_tempo);

    ffmpeg->stereo_to_mono = my_settings.value("stereo_to_mono").toBool();
    ui->cb_to_mono->setChecked(ffmpeg->stereo_to_mono);

    ffmpeg->to_kbit = my_settings.value("kbit").toDouble(&ok);
    ui->sb_kbits->setValue(ffmpeg->to_kbit);
}
void PodKastik::saveSettings()
{
    QSettings my_settings("Studio1656", "PodKastik", this);
    my_settings.setValue("default_folder", default_folder);
    my_settings.setValue("ytdl_folder", ytdl_folder);
    my_settings.setValue("ffmpeg_folder", ffmpeg_folder);
    my_settings.setValue("speed_tempo", ffmpeg->speed_tempo);
    my_settings.setValue("stereo_to_mono", ffmpeg->stereo_to_mono);
    my_settings.setValue("kbit", ffmpeg->to_kbit);
}

/*********************** YOUTUBE-DL********************************************************/
void PodKastik::ytdl_process_out()
{
    ui->l_current_file_name->setText(youtube_dl->current_file_name);
    ui->l_output->setText(youtube_dl->advance_status);
    ui->pb_progress->setValue(youtube_dl->dl_progress);
    this->setWindowTitle("PodKastik | DL: "+QString::number(youtube_dl->dl_progress)+"%");

    if(youtube_dl->dl_progress == 100)
    {
        ui->pb_progress->setFormat("...");
        ui->l_output->setText("Download finished!");
        this->setWindowTitle("PodKastik | "+ui->l_output->text());
    }
}
void PodKastik::ytdl_state_changed(QProcess::ProcessState s)
{
    switch(s){
        case QProcess::NotRunning: ui->pb_download->setText("Paste and download"); ui->pb_select_file_to_convert->setEnabled(true); break;
        case QProcess::Starting: ui->pb_download->setText("Stop download"); ui->pb_select_file_to_convert->setEnabled(false); break;
        case QProcess::Running: ui->pb_download->setText("Stop download"); ui->pb_select_file_to_convert->setEnabled(false); break;
    }
}
void PodKastik::do_ytdl()
{
    dl_link = clipboard->text();
    //dl_link = "https://www.youtube.com/watch?v=n4CjtoNoQ1Q";
    if(!dl_link.contains("http"))dl_link.prepend("https://");
    if(!urlExists(dl_link)){QMessageBox::information(this, "Error", "Invalid url: "+dl_link, QMessageBox::Ok); return;}
    ui->pb_progress->setFormat("Downloading...");

    youtube_dl->exe_process(dl_link);
}
void PodKastik::ytdl_finished()
{
    if(ui->cb_then->isEnabled() && ui->cb_then->isChecked()
        && ffmpeg->process->state() == QProcess::NotRunning)
            do_ffmpeg(youtube_dl->current_file_path_name);
}

/******************************************************************************************/
/*********************** FFmpeg ***********************************************************/
/******************************************************************************************/
void PodKastik::ffmpeg_process_out()
{
    ui->l_output->setText(ffmpeg->advance_status);
    ui->pb_progress->setValue(ffmpeg->conv_progress);
    this->setWindowTitle("PodKastik | C: "+QString::number(ffmpeg->conv_progress,'f',2)+"%");

    if(ffmpeg->conv_progress == 100)
    {
        ui->pb_progress->setFormat("...");
        ui->l_output->setText("Conversion finished!");
        this->setWindowTitle("Conversion | "+ui->l_output->text());
    }
}
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
    ui->pb_progress->setFormat("Converting...");
    ui->pb_progress->setValue(0);

    output_file_name = file_path_name;
    output_file_name = output_file_name.left(output_file_name.lastIndexOf(".")).append("_eaready.mp3");
    ui->l_current_file_name->setText(output_file_name);
    ffmpeg->output_file_name = output_file_name;

    ffmpeg->exe_process(file_path_name);
}
void PodKastik::ffmpeg_finished()
{qDebug()<<"ffmpeg_finished";
    tag_and_del();
    this->setWindowTitle("PodKastik | done!");
    ui->pb_progress->setFormat("done!");
}

/*********************** VIEW *************************************************************/
void PodKastik::on_pb_download_clicked()
{
    ui->pb_progress->setValue(0);
    ui->pb_progress->setFormat("...");
    ui->l_output->setText("--"); this->setWindowTitle("PodKastik | "+ui->l_output->text());
    //if(youtube_dl->state() == QProcess::Running){youtube_dl->kill(); return;}
    do_ytdl();
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
    QString str = QFileDialog::getOpenFileName(this, "Select file to convert", output_path);
    if(!str.isEmpty()) do_ffmpeg(str);
}
void PodKastik::on_pb_browse_clicked()
{
    QString p = QFileDialog::getExistingDirectory(this, "Output directory", output_path, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    output_path = p.isEmpty() ? output_path : p;
    ui->pb_browse->setText("Output path: "+output_path);
    youtube_dl->output_folder = output_path;
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
void PodKastik::on_dsb_tempo_valueChanged(double arg1){ ffmpeg->speed_tempo = arg1; saveSettings();}
void PodKastik::on_sb_kbits_valueChanged(double arg1){ ffmpeg->to_kbit = arg1; saveSettings();}
void PodKastik::on_cb_to_mono_stateChanged(int arg1){ ffmpeg->stereo_to_mono = arg1; saveSettings();}
void PodKastik::on_rb_audio_toggled(bool checked){ youtube_dl->audio_only = checked;}
void PodKastik::on_rb_video_toggled(bool checked){ ui->cb_then->setEnabled(!checked);}
void PodKastik::on_cb_playlist_stateChanged(int arg1){ youtube_dl->is_playlist = arg1;}
void PodKastik::on_pb_open_output_path_clicked(){ QDesktopServices::openUrl(QUrl::fromLocalFile(output_path));}

/*********************** OTHERS ***********************************************************/
void PodKastik::tag_and_del()
{qDebug()<<"tag and del";
    bool rm = QFile::remove(youtube_dl->current_file_path_name);
    QString new_name = output_file_name;
    int start_name = new_name.lastIndexOf("\\")==-1 ? new_name.lastIndexOf("/") : new_name.lastIndexOf("\\");
    new_name.insert(start_name+1, "("+QTime(0,0,0,0).addMSecs(ffmpeg->total_target_ms).toString("hh_mm_ss")+") ");
    bool rn = QFile::rename(output_file_name, new_name);
    if(!rm || !rn)QMessageBox::warning(this, "Problem at end", rm ? "Can't remove old file\n" : "" + rm ? "Can't rename new file" : "" );
}
void PodKastik::logging(QString str){ui->te_log->appendPlainText(str.trimmed());}
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
