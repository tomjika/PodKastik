# PodKastik
**A little application to easily download, speed up, stereo-to-mono, compress and resample podcasts !**

**For podcast consumers who needs them in faster, louder, lighter to listen on old devices.**

*A Qt C++ app using [youtube-dl](https://github.com/ytdl-org/youtube-dl/blob/master/README.md#options) and [FFmpeg](https://ffmpeg.org/documentation.html)* (.exe required), for Windows (hopefully not for long)

![Look!](/src/screenshot.PNG?raw=true "Screenshot")

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

Select a file to convert

Video download (no conversion)

Drag and drop a link for download

Drag and drop a file for conversion

Output directory, speed and resample values are remembered


#### EULA ####
This software uses libraries from the FFmpeg project under the LGPLv2.1