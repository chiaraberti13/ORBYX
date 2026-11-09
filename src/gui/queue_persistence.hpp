// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QDir>
#include <QLockFile>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <stdexcept>
// Optimistic revision + scoped process lock: a second window cannot silently
// replace jobs saved by another instance. No acquisition files are inspected.
class QueuePersistence final {
    QString path;QByteArray revision;bool existed=false;
    static QByteArray read(const QString& file) {
        if(QFileInfo(file).isSymLink())throw std::runtime_error("Queue storage must not be a symlink");
        QFile stream(file);if(!stream.open(QIODevice::ReadOnly))throw std::runtime_error("Cannot read queue storage");
        auto bytes=stream.read(1024*1024+1);if(bytes.size()>1024*1024)throw std::runtime_error("Queue storage exceeds 1 MiB");return bytes;
    }
public:
    explicit QueuePersistence(QString file):path(std::move(file)){}
    QJsonObject load() {
        existed=QFileInfo::exists(path);revision.clear();
        if(!existed){if(QFileInfo(path).isSymLink())throw std::runtime_error("Queue storage symlink");return {{"version",1},{"jobs",QJsonArray{}},{"statuses",QJsonObject{}}};}
        const auto bytes=read(path);revision=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256);
        QJsonParseError error;auto doc=QJsonDocument::fromJson(bytes,&error);
        if(error.error!=QJsonParseError::NoError||!doc.isObject()||doc.object()["version"].toInt()!=1||!doc.object()["jobs"].isArray()||!doc.object()["statuses"].isObject())throw std::runtime_error("Invalid stored queue schema");
        return doc.object();
    }
    void save(const QJsonObject& document) {
        QDir().mkpath(QFileInfo(path).absolutePath());QLockFile lock(path+".lock");
        if(!lock.tryLock(0))throw std::runtime_error("Another instance is saving the queue");
        if(QFileInfo(path).isSymLink())throw std::runtime_error("Queue storage must not be a symlink");
        const bool now=QFileInfo::exists(path);
        if(now!=existed||(now&&QCryptographicHash::hash(read(path),QCryptographicHash::Sha256)!=revision))throw std::runtime_error("Queue changed in another instance; reopen the window to reload it");
        auto payload=QJsonDocument(document).toJson(QJsonDocument::Compact);
        if(payload.size()>1024*1024)throw std::runtime_error("Queue exceeds 1 MiB");
        QSaveFile stream(path);stream.setDirectWriteFallback(false);
        if(!stream.open(QIODevice::WriteOnly))throw std::runtime_error("Cannot save queue");
        stream.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
        if(stream.write(payload)!=payload.size()||!stream.commit())throw std::runtime_error("Cannot commit queue");
        existed=true;revision=QCryptographicHash::hash(payload,QCryptographicHash::Sha256);
    }
};
