# PodKastik
**A little application to easily download, speed up, stereo-to-mono, compress and resample podcasts !**

**For podcast consumers who needs them in faster, louder, lighter to listen on old devices.**

*A Qt C++ app using [youtube-dl](https://github.com/ytdl-org/youtube-dl/blob/master/README.md#options) and [ffmpeg](https://ffmpeg.org/documentation.html)* (.exe required), for Windows (hopefully not for long)

### Functionalities ###
Download audio file from link in clipboard ( *youtube-dl -x --playlist-items 1 [link] --restrict-filenames -o [output-path]/%(title)s.%(ext)s* )

Convert the file in mp3 with: ( *ffmpeg -progress pipe:1 -loglevel 32 -y -i [file-name] -ac 1 -ab [bitrate]k -acodec mp3 -af dynaudnorm=f=150:g=15,atempo=[tempo]* )

- stereo to mono (or not)
- change bitrate (resample)
- compress
- playback speed change (tempo)
- export in mp3

Auto-convert after download (or not)

Download playlist (no conversion)

Conversion only by selecting a file

Video download (no conversion)

Output directory, speed and resample values are remembered

![Look!](/src/screenshot.PNG?raw=true "Screenshot")

