// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>

namespace orbyx::gui {

// Files created for the user belong in a visible, predictable location.  The
// environment override is intentionally useful for tests and managed installs.
inline QString userFilesRoot() {
    const QString override = qEnvironmentVariable("ORBYX_USER_FILES_ROOT").trimmed();
    if (!override.isEmpty()) return QDir::cleanPath(QFileInfo(override).absoluteFilePath());
    QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (pictures.isEmpty()) pictures = QDir::homePath();
    return QDir(pictures).filePath("ORBYX");
}

inline QString outputImagesRoot() {
    const QString path = QDir(userFilesRoot()).filePath("Output Images");
    QDir().mkpath(path);
    return path;
}

inline QString calibrationRoot() {
    const QString path = QDir(userFilesRoot()).filePath("Calibration");
    QDir().mkpath(path);
    return path;
}

inline QString logsRoot() {
    const QString path = QDir(userFilesRoot()).filePath("Logs");
    QDir().mkpath(path);
    return path;
}

inline QString newFolder(const QString &parent, const QString &prefix) {
    QDir().mkpath(parent);
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    return QDir(parent).filePath(prefix + "-" + stamp + "-" +
        QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}

inline QString newOutputFolder(const QString &prefix) {
    return newFolder(outputImagesRoot(), prefix);
}

inline QString newCalibrationFolder() {
    return newFolder(calibrationRoot(), "calibration");
}

} // namespace orbyx::gui
