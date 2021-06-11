#include "podkastik.h"
#include "ui_podkastik.h"

PodKastik::PodKastik(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PodKastik)
{
    ui->setupUi(this);
    this->setFixedHeight(165);

    setAcceptDrops(true);

    clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()), this, SLOT(clip_paste()));
    clip_paste();

    QSettings my_settings("Studio1656", "PodKastik", this);
    ytdl_folder = my_settings.value("ytdl_folder").toString();
    youtube_dl = new youtubedl_process(this, ytdl_folder);
        connect(youtube_dl, SIGNAL(process_ready(bool)), this, SLOT(ytdl_process_ready(bool)));
        connect(youtube_dl, SIGNAL(process_running(bool)), this, SLOT(ytdl_process_running(bool)));
        connect(youtube_dl, SIGNAL(process_out_update()), this, SLOT(ytdl_process_out()));
        connect(youtube_dl, SIGNAL(log(QString)), this, SLOT(logging(QString)));
        connect(youtube_dl, SIGNAL(download_finished()), this, SLOT(ytdl_finished()));
    youtube_dl->initialize_process();

    ffmpeg_folder = my_settings.value("ffmpeg_folder").toString();
    ffmpeg = new ffmpeg_process(this, ffmpeg_folder);
        connect(ffmpeg, SIGNAL(process_ready(bool)), this, SLOT(ffmpeg_process_ready(bool)));
        connect(ffmpeg, SIGNAL(process_running(bool)), this, SLOT(ffmpeg_process_running(bool)));
        connect(ffmpeg, SIGNAL(process_out_update()), this, SLOT(ffmpeg_process_out()));
        connect(ffmpeg, SIGNAL(log(QString)), this, SLOT(logging(QString)));
        connect(ffmpeg, SIGNAL(conversion_finished()), this, SLOT(ffmpeg_finished()));
    ffmpeg->initialize_process();

    loadSettings();
}
PodKastik::~PodKastik(){delete ui;}
void PodKastik::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    qDebug()<<mimeData;
    if(mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        if(urlList[0].isLocalFile() && ffmpeg->available && !ffmpeg->running)
            event->accept();
        else if(!urlList[0].isLocalFile() && youtube_dl->available && !youtube_dl->running)
            event->accept();
    }
}
void PodKastik::dropEvent(QDropEvent *event)
{
    event->accept();

    const QMimeData* mimeData = event->mimeData();
    qDebug()<<mimeData;
    if(mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        if(urlList[0].isLocalFile() && ffmpeg->available && !ffmpeg->running)
            do_ffmpeg(urlList[0].toLocalFile());
        else if(!urlList[0].isLocalFile() && youtube_dl->available && !youtube_dl->running)
            youtube_dl->exe_process(urlList[0].toString());
    }
}
void PodKastik::loadSettings()
{bool ok;
    QSettings my_settings("Studio1656", "PodKastik", this);

    default_folder = my_settings.value("default_folder").toString();
    output_path = default_folder;
    youtube_dl->output_folder = output_path;
    ui->pb_browse_default->setText("Default: "+default_folder);
    ui->pb_browse->setText("Output path: "+default_folder);

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
void PodKastik::ytdl_process_ready(bool isReady)
{
    ui->pb_download->setEnabled(youtube_dl->available);
    if(isReady)
    {
        ui->pb_browse_ytdl->setText(youtube_dl->use_portable ? "YT-dl Exe: "+ytdl_folder : "Youtube-dl is installed");
        ui->pb_browse_ytdl->setToolTip("Version: "+youtube_dl->version);
    }else{
        ui->l_output->setText("Youtube-dl or FFmpeg not found, see Settings");
        ui->pb_browse_ytdl->setText("Youtube-dl is not installed or .exe not found");
        ui->pb_download->setToolTip("Youtube-dl not found, see Settings");
        ui->pb_download->setToolTip("Click to go to website (https://github.com/ytdl-org/youtube-dl/blob/master/README.md#readme) or sudo -H pip install --upgrade youtube-dl (Ubuntu))");
    }
}
void PodKastik::ytdl_process_running(bool isRunning)
{qDebug()<<"ffmpeg_process_running";
    if(!isRunning)
    {
        ui->pb_download->setText("Paste and download");
        if(ffmpeg->available) ui->pb_select_file_to_convert->setEnabled(true);
    }else{
        ffmpeg->init_timer->stop();
        ui->pb_progress->setFormat("Downloading...");
        ui->pb_download->setText("Stop download");
        ui->pb_select_file_to_convert->setEnabled(false);
    }
}
void PodKastik::ytdl_finished()
{
    qDebug()<<ffmpeg->available << !ffmpeg->running<< ui->cb_auto_convert->isEnabled() << ui->cb_auto_convert->isChecked();
    if(ffmpeg->available && !ffmpeg->running
      && ui->cb_auto_convert->isEnabled() && ui->cb_auto_convert->isChecked())
    {        do_ffmpeg(youtube_dl->current_file_path_name);}
    else if(youtube_dl->dl_progress == 100)
    {
        ui->pb_progress->setFormat("...");
        ui->l_output->setText("Download finished!");
        this->setWindowTitle("PodKastik | "+ui->l_output->text());
    }
}

/*********************** FFmpeg ***********************************************************/
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
void PodKastik::ffmpeg_process_ready(bool isReady)
{qDebug()<<"ffmpeg_process_ready";
    ui->pb_select_file_to_convert->setEnabled(ffmpeg->available);
    if(isReady)
    {
        ui->pb_browse_ffmpeg->setText(ffmpeg->use_portable ? "FFmpeg Exe: "+ffmpeg_folder : "FFmpeg is installed");
        ui->pb_browse_ffmpeg->setToolTip("Version: "+ffmpeg->version);
    }else{
        ui->l_output->setText("Youtube-dl or FFmpeg not found, see Settings");
        ui->pb_select_file_to_convert->setToolTip("FFmpeg not found, see Settings");
        ui->pb_browse_ffmpeg->setText("FFmpeg is not installed or .exe not found");
        ui->pb_browse_ffmpeg->setToolTip("Click to go to website (https://ffmpeg.org/download.html) or sudo apt-get install ffmpeg (Ubuntu)");
    }
}
void PodKastik::ffmpeg_process_running(bool isRunning)
{qDebug()<<"ffmpeg_process_running";
    if(!isRunning)
    {
        ui->pb_select_file_to_convert->setText("Select and convert");
        if(youtube_dl->available) ui->pb_download->setEnabled(true);
    }else{
        youtube_dl->init_timer->stop();
        ui->pb_select_file_to_convert->setText("Stop convertion");
        ui->pb_download->setEnabled(false);
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
    if(youtube_dl->process->state() != QProcess::NotRunning) //stop download
    {
        youtube_dl->process->kill();
        ui->pb_progress->setValue(0);
        ui->pb_progress->setFormat("...");
        ui->l_output->setText("Download stopped");
        this->setWindowTitle("PodKastik | "+ui->l_output->text());
        return;
    }
    else
        youtube_dl->exe_process(clipboard->text());
}
void PodKastik::on_pb_select_file_to_convert_clicked()
{
    if(ffmpeg->process->state() != QProcess::NotRunning) //stop conversion
    {
        ffmpeg->process->kill();
        ui->pb_progress->setValue(0);
        ui->pb_progress->setFormat("...");
        ui->l_output->setText("Conversion stopped");
        this->setWindowTitle("PodKastik | "+ui->l_output->text());
        return;
    }else{
        QString str = QFileDialog::getOpenFileName(this, "Select file to convert", output_path);
        if(!str.isEmpty()) do_ffmpeg(str);
    }
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
void PodKastik::on_pb_settings_clicked(){this->setFixedHeight(this->height()<200 ? 440 : 165);}
void PodKastik::on_pb_browse_default_clicked()
{
    default_folder = QFileDialog::getExistingDirectory(this, "Default directory", default_folder, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->pb_browse_default->setText("Default: "+default_folder);
    ui->pb_browse_default->setToolTip(ui->pb_browse_default->text());
    saveSettings();
}
void PodKastik::on_pb_browse_ytdl_clicked()
{
    #ifdef __linux__
    if(!youtube_dl->available) QDesktopServices::openUrl(QUrl("https://github.com/ytdl-org/youtube-dl/blob/master/README.md#readme"));
    #elif _WIN32
    if(!(ytdl_folder = QFileDialog::getOpenFileName(this, "Youtube-dl directory", ytdl_folder, "youtube-dl (*.exe)")).isNull())
    {
        //ytdl_folder = str;
        ui->pb_browse_ytdl->setText("YT-dl Exe: "+ytdl_folder);
        ui->pb_browse_ytdl->setToolTip(ui->pb_browse_ytdl->text());
        saveSettings();
    }
    #endif
}
void PodKastik::on_pb_browse_ffmpeg_clicked()
{qDebug()<<"ici";
    #ifdef __linux__
    if(!ffmpeg->available) QDesktopServices::openUrl(QUrl("https://ffmpeg.org/download.html"));
    #elif _WIN32
    if(!(ffmpeg_folder = QFileDialog::getOpenFileName(this, "ffmpeg directory", ffmpeg_folder, "ffmpeg (*.exe)")).isNull())
    {
        //ffmpeg_folder = str;
        ui->pb_browse_ffmpeg->setText("FFmpeg Exe: "+ffmpeg_folder);
        ui->pb_browse_ffmpeg->setToolTip(ui->pb_browse_ffmpeg->text());
        saveSettings();
    }
    #endif
}
void PodKastik::on_dsb_tempo_valueChanged(double arg1){ ffmpeg->speed_tempo = arg1; saveSettings();}
void PodKastik::on_sb_kbits_valueChanged(double arg1){ ffmpeg->to_kbit = arg1; saveSettings();}
void PodKastik::on_cb_to_mono_stateChanged(int arg1){ ffmpeg->stereo_to_mono = arg1; saveSettings();}
void PodKastik::on_rb_audio_toggled(bool checked){ youtube_dl->audio_only = checked;}
void PodKastik::on_rb_video_toggled(bool checked){ ui->cb_auto_convert->setEnabled(!checked);}
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
void PodKastik::on_pb_about_clicked()
{
    QMessageBox about_box(this);
    about_box.setTextFormat(Qt::RichText);
    about_box.setText("About PodKastik");
    about_box.setText("Download audio file from link in clipboard.<br>"
                        "Convert it depending on parameters.<br><br>"
                        "<a href='https://ytdl-org.github.io/youtube-dl/download.html'>Youtube-dl</a> and "
                        "<a href='https://ffmpeg.org/download.html'>FFmpeg</a> are needed.<br><br>"
                        "<a href='https://github.com/tomjika/PodKastik'>PodKastik on Github!</a>");
    about_box.exec();
}
