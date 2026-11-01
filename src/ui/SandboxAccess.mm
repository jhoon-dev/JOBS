#include "ui/SandboxAccess.h"
#include "persistence/ApplicationStorage.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QMetaObject>
#include <QSettings>
#include <QThread>
#import <AppKit/AppKit.h>
#import <Security/SecTask.h>

namespace jobs {
namespace {
NSString *native(const QString &text) {
    return [[NSString alloc] initWithCharacters:reinterpret_cast<const unichar *>(text.utf16())
                                         length:static_cast<NSUInteger>(text.size())];
}
QString pathText(NSString *text) {
    return QString::fromUtf8(text.UTF8String);
}
QString canonical(const QString &path) {
    const auto absolute = QFileInfo(path).absoluteFilePath();
    const auto resolved = QFileInfo(absolute).canonicalFilePath();
    return QDir::cleanPath(resolved.isEmpty() ? absolute : resolved);
}
bool within(const QString &root, const QString &path) {
    return path == root || path.startsWith(root.endsWith('/') ? root : root + '/');
}
QSettings bookmarks() {
    return QSettings(QDir(applicationStorage().stateDirectory).filePath("mac-sandbox-folders.ini"),
                     QSettings::IniFormat);
}
struct Folders {
    NSMutableArray<NSURL *> *urls = [NSMutableArray array];
    QStringList paths;
    ~Folders() {
        for (NSURL *url in urls)
            [url stopAccessingSecurityScopedResource];
    }
    bool contains(const QString &path) const {
        for (const auto &folder : paths)
            if (within(folder, path))
                return true;
        return false;
    }
    bool retain(NSURL *url) {
        if (![url startAccessingSecurityScopedResource])
            return false;
        [urls addObject:url];
        paths.push_back(canonical(pathText(url.path)));
        return true;
    }
};
Folders &folders() {
    static Folders value;
    return value;
}
NSURL *resolve(NSData *data, BOOL *stale) {
    return [NSURL URLByResolvingBookmarkData:data
                                     options:NSURLBookmarkResolutionWithSecurityScope | NSURLBookmarkResolutionWithoutUI
                               relativeToURL:nil
                         bookmarkDataIsStale:stale
                                       error:nil];
}
NSData *bookmark(NSURL *url) {
    return [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
         includingResourceValuesForKeys:nil
                          relativeToURL:nil
                                  error:nil];
}
bool authorize(QWidget *parent, const QString &path) {
    @autoreleasepool {
        const auto directory = canonical(QFileInfo(path).absolutePath());
        if (within(canonical(pathText(NSHomeDirectory())), directory) ||
            within(canonical(QDir::tempPath()), directory) || folders().contains(directory))
            return true;
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = NO;
        panel.canChooseDirectories = YES;
        panel.allowsMultipleSelection = NO;
        panel.canCreateDirectories = NO;
        panel.title = @"PPM 폴더 접근";
        panel.prompt = @"폴더 선택";
        panel.message =
            @"이 프로젝트를 열거나 저장할 폴더를 선택하세요. 선택한 폴더의 접근 권한을 다음 실행에도 사용합니다.";
        panel.directoryURL = [NSURL fileURLWithPath:native(directory) isDirectory:YES];
        if ([panel runModal] != NSModalResponseOK)
            return false;
        NSURL *chosen = panel.URL;
        const auto authorizedDirectory = canonical(QFileInfo(path).absolutePath());
        if (!chosen || canonical(pathText(chosen.path)) != authorizedDirectory) {
            QMessageBox::warning(parent, "PPM 폴더 접근", "해당 파일이 있는 폴더를 선택해 주세요.");
            return false;
        }
        NSData *data = bookmark(chosen);
        BOOL stale = NO;
        NSURL *scoped = data ? resolve(data, &stale) : nil;
        if (!scoped || !folders().retain(scoped)) {
            QMessageBox::warning(parent, "PPM 폴더 접근", "폴더 접근 권한을 저장하지 못했습니다. 다시 시도해 주세요.");
            return false;
        }
        auto settings = bookmarks();
        QDir().mkpath(QFileInfo(settings.fileName()).absolutePath());
        const auto key = QCryptographicHash::hash(authorizedDirectory.toUtf8(), QCryptographicHash::Sha256).toHex();
        settings.setValue(QString::fromLatin1(key),
                          QByteArray(static_cast<const char *>(data.bytes), static_cast<qsizetype>(data.length)));
        settings.sync();
        if (settings.status() != QSettings::NoError) {
            QMessageBox::warning(parent, "PPM 폴더 접근",
                                 "이번 실행에서는 사용할 수 있지만 다음 실행의 접근 권한을 저장하지 못했습니다.");
        }
        return true;
    }
}
} // namespace
bool sandboxActive() {
    static const bool enabled = [] {
        SecTaskRef task = SecTaskCreateFromSelf(kCFAllocatorDefault);
        if (!task)
            return false;
        CFTypeRef value = SecTaskCopyValueForEntitlement(task, CFSTR("com.apple.security.app-sandbox"), nullptr);
        const bool result =
            value && CFGetTypeID(value) == CFBooleanGetTypeID() && CFBooleanGetValue(static_cast<CFBooleanRef>(value));
        if (value)
            CFRelease(value);
        CFRelease(task);
        return result;
    }();
    return enabled;
}
void restoreSandboxFolders() {
    if (!sandboxActive())
        return;
    @autoreleasepool {
        auto settings = bookmarks();
        const auto keys = settings.childKeys();
        for (const auto &key : keys) {
            if (folders().paths.size() >= 256)
                break;
            const auto bytes = settings.value(key).toByteArray();
            if (bytes.isEmpty() || bytes.size() > 1024 * 1024)
                continue;
            NSData *data = [NSData dataWithBytes:bytes.constData() length:static_cast<NSUInteger>(bytes.size())];
            BOOL stale = NO;
            NSURL *url = resolve(data, &stale);
            if (!url || !folders().retain(url))
                continue;
            if (stale) {
                NSData *updated = bookmark(url);
                if (updated)
                    settings.setValue(key, QByteArray(static_cast<const char *>(updated.bytes),
                                                      static_cast<qsizetype>(updated.length)));
            }
        }
        settings.sync();
    }
}
bool authorizeSandboxFile(QWidget *parent, const QString &path) {
    if (!sandboxActive())
        return true;
    auto *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app || QCoreApplication::closingDown())
        return false;
    if (QThread::currentThread() == app->thread())
        return authorize(parent, path);
    bool allowed = false;
    // Worker requests use an application-owned dialog; no QWidget is accessed off-thread.
    QMetaObject::invokeMethod(app, [&] { allowed = authorize(nullptr, path); }, Qt::BlockingQueuedConnection);
    return allowed;
}
} // namespace jobs
