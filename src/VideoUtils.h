#ifndef VIDEOUTILS_H
#define VIDEOUTILS_H

#include <QString>

namespace VideoUtils {
    bool isValidVideoFile(const QString& filePath);
    QString getVideoResolution(const QString& filePath);
}

#endif // VIDEOUTILS_H