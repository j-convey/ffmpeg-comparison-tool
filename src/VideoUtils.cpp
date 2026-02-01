#include "VideoUtils.h"
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

namespace VideoUtils {

bool isValidVideoFile(const QString& filePath) {
    // Check file extension
    QStringList validExtensions = {"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "mpg", "mpeg", "m4v"};
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    return validExtensions.contains(extension);
}

QString getVideoResolution(const QString& filePath) {
    // Use ffprobe to get video resolution
    QProcess process;
    QStringList arguments;
    
    arguments << "-v" << "error"
              << "-select_streams" << "v:0"
              << "-show_entries" << "stream=width,height"
              << "-of" << "csv=s=x:p=0"
              << filePath;
    
    process.start("ffprobe", arguments);
    
    if (!process.waitForStarted(3000)) {
        return "";
    }
    
    if (!process.waitForFinished(5000)) {
        process.kill();
        return "";
    }
    
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    
    // Output should be in format "1920x1080"
    if (!output.isEmpty() && output.contains("x")) {
        return output;
    }
    
    return "";
}

} // namespace VideoUtils