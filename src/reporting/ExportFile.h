#pragma once
#include <QSaveFile>
#include <QFileInfo>
#include <QLockFile>
#include <stdexcept>
namespace jobs {
class ExportFile {
public:
    QSaveFile file;
    QLockFile lock;
    ExportFile(const QString& path, const QString& extension) : file(path), lock(path + ".lock") {
        if (QFileInfo(path).suffix().compare(extension, Qt::CaseInsensitive) != 0) throw std::runtime_error("내보내기 확장자가 올바르지 않습니다.");
        lock.setStaleLockTime(0);
        if (!lock.tryLock(0)) throw std::runtime_error("출력 파일의 잠금을 얻지 못했습니다.");
        if (QFileInfo::exists(path)) throw std::runtime_error("출력 대상이 이미 있습니다. 기존 파일을 보존하려면 새 파일명을 선택하세요.");
        file.setDirectWriteFallback(false);
        if (!file.open(QIODevice::WriteOnly)) throw std::runtime_error(file.errorString().toUtf8().toStdString());
    }
    void commit() {
        if (file.error() != QFileDevice::NoError || file.size() > 64 * 1024 * 1024) throw std::runtime_error("출력 실패 또는 보고서 파일 한도(64 MiB) 초과입니다.");
        if (QFileInfo::exists(file.fileName())) throw std::runtime_error("출력 중 대상 파일이 생성되었습니다. 기존 파일을 보존했습니다.");
        if (!file.commit()) throw std::runtime_error(file.errorString().toUtf8().toStdString());
    }
};
}
