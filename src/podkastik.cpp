#include "podkastik.h"
#include "ui_podkastik.h"

PodKastik::PodKastik(QWidget *parent, QString outputPath) :
    QWidget(parent),
    ui(new Ui::PodKastik),
    output_path(outputPath),
    ytdl_AudioOnly(true),
    ytdl_IsPlaylist(false)
{
    ui->setupUi(this);

    setAcceptDrops(true);

    clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()), this, SLOT(clip_paste()));
    clip_paste();

    QSettings my_settings("Studio1656", "PodKastik", this);
    ytdl_folder = my_settings.value("ytdl_folder").toString();
    CreateYtdlProcess(ytdl_folder);
    ytdl_use_patched = my_settings.value("use_ytdl_patched").toBool();

    youtube_dl->initialize_process(ytdl_use_patched);

    ffmpeg_folder = my_settings.value("ffmpeg_folder").toString();
    ffmpeg = new ffmpeg_process(this, ffmpeg_folder);
        connect(ffmpeg, SIGNAL(process_ready(bool)), this, SLOT(ffmpeg_process_ready(bool)));
        connect(ffmpeg, SIGNAL(process_running(bool)), this, SLOT(ffmpeg_process_running(bool)));
        connect(ffmpeg, SIGNAL(process_out_update()), this, SLOT(ffmpeg_process_out()));
        connect(ffmpeg, SIGNAL(log(QString)), this, SLOT(logging(QString)));
        connect(ffmpeg, SIGNAL(conversion_finished()), this, SLOT(ffmpeg_finished()));
    ffmpeg->initialize_process();
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
        {
            current_file_name = urlList[0].toLocalFile();
            do_ffmpeg(current_file_name);
        }
        else if(!urlList[0].isLocalFile() && youtube_dl->available && !youtube_dl->running)
        {
            current_file_name = urlList[0].toLocalFile();
            if(youtube_dl.get() == nullptr) CreateYtdlProcess(ytdl_folder);
            youtube_dl->exe_process(urlList[0].toString(), output_path, ytdl_IsPlaylist, ytdl_AudioOnly);
        }
    }
}

void PodKastik::loadSettings()
{bool ok;
    ui->settingsWidget->setHidden(true);
    this->setFixedHeight(this->height() - ui->settingsWidget->height());

    QSettings my_settings("Studio1656", "PodKastik", this);

    default_folder = my_settings.value("default_folder").toString();
    ffmpeg->speed_tempo = my_settings.value("speed_tempo").toDouble(&ok);
    ffmpeg->to_kbit = my_settings.value("kbit").toDouble(&ok);
    ffmpeg->stereo_to_mono = my_settings.value("stereo_to_mono").toBool();
    default_prefix = my_settings.value("default_prefix").toString();

    if(output_path.isEmpty()) output_path = default_folder;
    SetTextToButton(ui->pb_browse_default, "Default: "+default_folder, Qt::ElideMiddle);
    SetTextToButton(ui->pb_browse_output, "Output: "+output_path, Qt::ElideMiddle);

    ui->dsb_tempo->setValue(ffmpeg->speed_tempo);
    ui->cb_to_mono->setChecked(ffmpeg->stereo_to_mono);
    ui->sb_kbits->setValue(ffmpeg->to_kbit);
    ui->le_prefix_format->setText(default_prefix);
    ui->cb_ytdl_patched->setChecked(ytdl_use_patched);
}

void PodKastik::saveSettings()
{
    QSettings my_settings("Studio1656", "PodKastik", this);
    my_settings.setValue("default_folder", default_folder);
    my_settings.setValue("ytdl_folder", ytdl_folder);
    my_settings.setValue("default_prefix", default_prefix);
    my_settings.setValue("ffmpeg_folder", ffmpeg_folder);
    my_settings.setValue("speed_tempo", ffmpeg->speed_tempo);
    my_settings.setValue("stereo_to_mono", ffmpeg->stereo_to_mono);
    my_settings.setValue("kbit", ffmpeg->to_kbit);
    my_settings.setValue("use_ytdl_patched", ytdl_use_patched);
}

void PodKastik::addToConvertList(QString fileName)
{
    filesToConvert<<fileName;
}

/*********************** YOUTUBE-DL********************************************************/
void PodKastik::CreateYtdlProcess(QString exe_path)
{
    youtube_dl = std::make_unique<youtubedl_process>(this, exe_path);
        connect(youtube_dl.get(), SIGNAL(process_ready(bool)), this, SLOT(ytdl_process_ready(bool)));
        connect(youtube_dl.get(), SIGNAL(process_running(bool)), this, SLOT(ytdl_process_running(bool)));
        connect(youtube_dl.get(), SIGNAL(process_out_update()), this, SLOT(ytdl_process_out()));
        connect(youtube_dl.get(), SIGNAL(download_finished()), this, SLOT(ytdl_finished()));
        connect(youtube_dl.get(), SIGNAL(log(QString)), this, SLOT(logging(QString)));
}

void PodKastik::ytdl_process_out()
{
    SetTextToLabel(ui->l_current_file_name, youtube_dl->current_file_name, Qt::ElideMiddle);
    ui->l_output->setText(youtube_dl->advance_status);
    ui->pb_progress->setValue(youtube_dl->dl_progress);
    this->setWindowTitle("PodKastik | DL: "+QString::number(youtube_dl->dl_progress)+"%");

    if(youtube_dl->dl_progress == 0)
    {
        ui->pb_progress->setFormat("Requesting download...");
    }
    else if(youtube_dl->dl_progress < 100)
    {
        ui->pb_progress->setFormat("Downloading...");
    }
    else if(youtube_dl->dl_progress == 100)
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
        //youtube_dl.reset();
    }else{
        ui->l_output->setText("Youtube-dl or FFmpeg not found, see Settings");
        ui->pb_browse_ytdl->setText("Youtube-dl is not installed or .exe not found");
    #ifdef __linux__
        ui->pb_browse_ytdl->setToolTip("Click to go to website (https://github.com/ytdl-org/youtube-dl/blob/master/README.md#readme) or sudo -H pip install --upgrade youtube-dl (Ubuntu))");
    #elif _WIN32
        ui->pb_browse_ytdl->setToolTip("Click to select youtube-dl.exe to use or go to website (https://github.com/ytdl-org/youtube-dl/blob/master/README.md#readme)");
    #endif
    }
}

void PodKastik::ytdl_process_running(bool isRunning)
{qDebug()<<"ytdl_process_running";
    if(!isRunning)
    {
        ui->pb_download->setText("Paste and download");
        if(ffmpeg->available) ui->pb_select_file_to_convert->setEnabled(true);
    }else{
        ffmpeg->init_timer->stop();
        ui->pb_download->setText("Stop download");
        ui->pb_select_file_to_convert->setEnabled(false);
    }
}

void PodKastik::ytdl_finished()
{
    qDebug()<<"ytdl_finished";
    if((youtube_dl->dl_progress =! 100)) { return;}
    if(!ffmpeg->available || !ui->cb_auto_convert->isEnabled() || !ui->cb_auto_convert->isChecked())
    {
        ui->pb_progress->setFormat("...");
        ui->l_output->setText("Download finished!");
        this->setWindowTitle("PodKastik | "+ui->l_output->text());
    }
    else if(!youtube_dl->current_file_path_name.isEmpty())
    {
        current_file_name = youtube_dl->current_file_path_name;
        do_ffmpeg(youtube_dl->current_file_path_name);
    }

    //youtube_dl.reset();
}

/*********************** FFmpeg ***********************************************************/
void PodKastik::ffmpeg_process_out()
{
    ui->l_output->setText(ffmpeg->advance_status);
    ui->pb_progress->setValue(ffmpeg->conv_progress);
    this->setWindowTitle("PodKastik | C: "+QString::number(ffmpeg->conv_progress,'f',2)+"%");

    if(ffmpeg->conv_progress == 100){
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
    #ifdef __linux__
        ui->pb_browse_ffmpeg->setToolTip("Click to go to website (https://ffmpeg.org/download.html) or sudo apt-get install ffmpeg (Ubuntu)");
    #elif _WIN32
        ui->pb_browse_ffmpeg->setToolTip("Click to select ffmpeg.exe to use or go to website (https://ffmpeg.org/download.html)");
    #endif
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
    qDebug()<<"do_ffmpeg"<<file_path_name;
    ui->pb_progress->setFormat("Converting...");
    ui->pb_progress->setValue(0);

    output_file_name = file_path_name;
    output_file_name = output_file_name.left(output_file_name.lastIndexOf(".")).append("_eaready.mp3");
    SetTextToLabel(ui->l_current_file_name, output_file_name, Qt::ElideMiddle);

    ffmpeg->output_file_name = output_file_name;

    ffmpeg->exe_process(file_path_name);
}

void PodKastik::ffmpeg_finished()
{qDebug()<<"ffmpeg_finished";
    if(!ffmpeg->initializing)
    {
        tag_and_del();
        this->setWindowTitle("PodKastik | done!");
        ui->pb_progress->setFormat("done!");
    }

    if(!filesToConvert.isEmpty())
    {
        do_ffmpeg(filesToConvert.takeFirst());
    }
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
        if(youtube_dl.get() == nullptr) CreateYtdlProcess(ytdl_folder);
        youtube_dl->exe_process(clipboard->text(), output_path, ytdl_IsPlaylist, ytdl_AudioOnly);
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
        if(!str.isEmpty())
        {
            current_file_name = str;
            do_ffmpeg(current_file_name);
        }
    }
}

void PodKastik::on_pb_browse_output_clicked()
{
    QString p = QFileDialog::getExistingDirectory(this, "Output directory", output_path, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    output_path = p.isEmpty() ? output_path : p;
    SetTextToButton(ui->pb_browse_output, "Output: "+output_path, Qt::ElideMiddle);
}

void PodKastik::on_pb_settings_clicked()
{
    bool settingsHidden = ui->settingsWidget->isHidden();
    int settingsWidgetHeight = ui->settingsWidget->height();
    ui->settingsWidget->setHidden(!settingsHidden);

    if(settingsHidden)
        this->setFixedHeight(this->height() + settingsWidgetHeight);
    else
        this->setFixedHeight(this->height() - settingsWidgetHeight);
}

void PodKastik::on_pb_browse_default_clicked()
{
    default_folder = QFileDialog::getExistingDirectory(this, "Default directory", default_folder, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    SetTextToButton(ui->pb_browse_default, "Default: "+default_folder, Qt::ElideMiddle);
    SetTextToButton(ui->pb_browse_output, "Output: "+output_path, Qt::ElideMiddle);
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
        saveSettings();
        ui->pb_browse_ytdl->setText("YT-dl Exe: "+ytdl_folder);
        //ui->pb_browse_ytdl->setToolTip(ui->pb_browse_ytdl->text());
        youtube_dl->initialize_process();
    }
   #endif
}

void PodKastik::on_pb_browse_ffmpeg_clicked()
{
   #ifdef __linux__
    if(!ffmpeg->available) QDesktopServices::openUrl(QUrl("https://ffmpeg.org/download.html"));
   #elif _WIN32
    if(!(ffmpeg_folder = QFileDialog::getOpenFileName(this, "ffmpeg directory", ffmpeg_folder, "ffmpeg (*.exe)")).isNull())
    {
        saveSettings();
        ui->pb_browse_ffmpeg->setText("FFmpeg Exe: "+ffmpeg_folder);
        //ui->pb_browse_ffmpeg->setToolTip(ui->pb_browse_ffmpeg->text());
        ffmpeg->initialize_process();
    }
   #endif
}

void PodKastik::on_dsb_tempo_valueChanged(double arg1){ ffmpeg->speed_tempo = arg1; saveSettings();}

void PodKastik::on_sb_kbits_valueChanged(double arg1){ ffmpeg->to_kbit = arg1; saveSettings();}

void PodKastik::on_cb_to_mono_stateChanged(int arg1){ ffmpeg->stereo_to_mono = arg1; saveSettings();}

void PodKastik::on_rb_audio_toggled(bool checked){ ytdl_AudioOnly = checked;}

void PodKastik::on_rb_video_toggled(bool checked){ ui->cb_auto_convert->setEnabled(!checked);}

void PodKastik::on_cb_playlist_stateChanged(int arg1){ ytdl_IsPlaylist = arg1;}

void PodKastik::on_cb_ytdl_patched_stateChanged(int arg1){ ytdl_use_patched = arg1; saveSettings();}

void PodKastik::on_pb_open_output_path_clicked(){ QDesktopServices::openUrl(QUrl::fromLocalFile(output_path));}

void PodKastik::on_le_prefix_format_textChanged(const QString &arg1){ default_prefix = arg1; saveSettings();}


/*********************** OTHERS ***********************************************************/
void PodKastik::tag_and_del()
{qDebug()<<"tag and del";

    bool rm = removeOldFile();

    bool rn = renameNewFile();

    QString err_str = "";
    err_str += rm ? "" : "Can't remove old file\n";
    err_str += rn ? "" : "Can't rename new file";
    if(!rm || !rn) QMessageBox::warning(this, "Problem at end", err_str);
}

bool PodKastik::removeOldFile()
{
    QFile file(current_file_name);
    file.open(QFile::ReadWrite);

    bool rm = file.remove(); qDebug()<<file.errorString();
    return rm;
}

bool PodKastik::renameNewFile()
{
    QFile file(output_file_name);
    file.open(QFile::ReadWrite);

    QString new_name = output_file_name;
    int start_name = new_name.lastIndexOf("\\")==-1 ? new_name.lastIndexOf("/") : new_name.lastIndexOf("\\");
    QString time_txt = QTime(0,0,0,0).addMSecs(ffmpeg->total_target_ms).toString(default_prefix);
    new_name.insert(start_name+1, time_txt);

    if(QFile::exists(new_name))
    {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Problem", "A file with the same name already exists."
                                                                                   "\nDo you want to replace it?"
                                                                                   "\n\n(No will add '_new' to the new file name)");

        if (reply == QMessageBox::Yes)
            QFile(new_name).remove();
        else
        {
            while(QFile::exists(new_name))
            {
                new_name.remove(".mp3");
                new_name = new_name.append("_new.mp3");
            }
        }
    }

    bool rn = file.rename(new_name); qDebug()<<file.errorString();
    return rn;
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

void PodKastik::SetTextToLabel(QLabel* label, QString text, Qt::TextElideMode elideMode)
{
    QFontMetrics metrix(label->font());
    int width = label->width() - 2;
    QString clippedText = metrix.elidedText(text, elideMode, width);
    label->setText(clippedText);
    label->setToolTip(text);
}

void PodKastik::SetTextToButton(QPushButton* button, QString text, Qt::TextElideMode elideMode)
{
    QFontMetrics metrix(button->font());
    int width = button->width() - 2;
    QString clippedText = metrix.elidedText(text, elideMode, width);
    button->setText(clippedText);
    button->setToolTip(text);
}
