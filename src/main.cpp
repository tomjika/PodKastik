#include "podkastik.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("PodKastik");
    QCoreApplication::setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("PodKastik"
                                     "\nA little application to easily download, speed up, stereo-to-mono, compress and resample podcasts !"
                                     "\nFor podcast consumers who needs them faster, louder, lighter to listen on old devices."
                                     "\nA Qt C++ app using youtube-dl and FFmpeg, for Windows and Linux");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("output directory", "output directory");
    //parser.addPositionalArgument("destination", "Destination directory");

    // Process the actual command line arguments given by the user
    parser.process(app);

    QString outputDirectory = "";
    QString inputFile = "";
    bool doConvertion = false;

    const QStringList args = parser.positionalArguments();
    if(!args.isEmpty())
    {
        inputFile = args.at(0);
    }

    if(inputFile.endsWith(".mp3")
     ||inputFile.endsWith(".m4a")
     ||inputFile.endsWith(".opus"))
    {
        int idx = inputFile.lastIndexOf("/");
        outputDirectory = inputFile.left(idx);
        doConvertion = true;
    }
    else
    {
        outputDirectory = inputFile;
    }

    PodKastik w(nullptr, outputDirectory);
    w.show();
    w.loadSettings();

    if(doConvertion) w.addToConvertList(inputFile);

    return app.exec();
}
