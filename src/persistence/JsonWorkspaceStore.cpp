#include "persistence/JsonWorkspaceStore.h"
#include "persistence/WorkspaceFile.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <cmath>

namespace jobs {
namespace {
constexpr qint64 limit = 8 * 1024 * 1024;
QString qs(const std::string &value) {
    return QString::fromStdString(value);
}
std::string str(const QString &value) {
    return value.toUtf8().toStdString();
}
std::string fingerprint(const QByteArray &bytes) {
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex().toStdString();
}
[[noreturn]] void invalid(const std::string &field) {
    throw AppError(ErrorCode::WORKSPACE_INVALID, {field});
}
void keys(const QJsonObject &object, std::initializer_list<const char *> allowed) {
    for (const auto &key : object.keys()) {
        bool found = false;
        for (const auto *item : allowed)
            if (key == QLatin1String(item))
                found = true;
        if (!found)
            invalid("unknown field: " + str(key));
    }
}
QJsonObject object(const QJsonValue &value) {
    if (!value.isObject())
        invalid("object");
    return value.toObject();
}
QJsonArray array(const QJsonObject &parent, const char *name, std::size_t maximum) {
    const auto value = parent.value(QLatin1String(name));
    if (!value.isArray() || std::size_t(value.toArray().size()) > maximum)
        invalid(name);
    return value.toArray();
}
std::string text(const QJsonObject &parent, const char *name, qsizetype maximum = 4096) {
    const auto value = parent.value(QLatin1String(name));
    if (!value.isString() || value.toString().size() > maximum || value.toString().contains(QChar(0)))
        invalid(name);
    return str(value.toString());
}
int integer(const QJsonValue &value, int minimum, int maximum) {
    const auto number = value.toDouble(-1);
    if (!value.isDouble() || !std::isfinite(number) || number != std::floor(number) || number < minimum ||
        number > maximum)
        invalid("integer");
    return int(number);
}
std::optional<int> units(const QJsonValue &value) {
    if (value.isNull())
        return {};
    return integer(value, 0, maxSharedResourceUnits);
}
QJsonValue units(std::optional<int> value) {
    return value ? QJsonValue(*value) : QJsonValue(QJsonValue::Null);
}
QByteArray read(const std::string &path) {
    QFile file(qs(path));
    if (!file.open(QIODevice::ReadOnly))
        throw AppError(ErrorCode::FILE_OPEN_FAILED, {str(file.errorString())}, ErrorContext{.path = path});
    if (file.size() > limit)
        throw AppError(ErrorCode::WORKSPACE_LIMIT);
    const auto bytes = file.read(limit + 1);
    if (bytes.size() > limit)
        throw AppError(ErrorCode::WORKSPACE_LIMIT);
    if (file.error() != QFileDevice::NoError)
        throw AppError(ErrorCode::FILE_READ_FAILED, {str(file.errorString())});
    return bytes;
}
QJsonObject encode(const Workspace &workspace, const QDir &directory) {
    QJsonArray projects, resources, bindings;
    for (const auto &reference : workspace.projects)
        projects.append(QJsonObject{{"documentId", qs(reference.documentId)},
                                    {"name", qs(reference.name)},
                                    {"path", directory.relativeFilePath(qs(reference.path))}});
    for (const auto &resource : workspace.resources) {
        QJsonArray exceptions, weekdays, periods, holidays;
        for (const auto &day : resource.exceptions)
            exceptions.append(QJsonObject{{"date", qs(day.date)}, {"units", units(day.units)}});
        for (auto day : resource.calendar.weekdays)
            weekdays.append(day);
        for (const auto &period : resource.calendar.periods)
            periods.append(QJsonObject{{"begin", period.begin}, {"end", period.end}});
        for (const auto &day : resource.calendar.holidays)
            holidays.append(qs(day));
        resources.append(QJsonObject{
            {"id", qs(resource.id)},
            {"name", qs(resource.name)},
            {"type", qs(resource.type)},
            {"maxUnits", units(resource.maxUnits)},
            {"exceptions", exceptions},
            {"calendar", QJsonObject{{"weekdays", weekdays}, {"periods", periods}, {"holidays", holidays}}}});
    }
    for (const auto &binding : workspace.bindings)
        bindings.append(QJsonObject{{"projectId", qs(binding.projectId)},
                                    {"resourceId", qs(binding.resourceId)},
                                    {"sharedId", qs(binding.sharedId)}});
    return {{"id", qs(workspace.id)}, {"projects", projects}, {"resources", resources}, {"bindings", bindings}};
}
} // namespace
Workspace decodeWorkspaceFile(const QByteArray &bytes, const QString &baseDirectory) {
    if (bytes.size() > limit)
        throw AppError(ErrorCode::WORKSPACE_LIMIT);
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        invalid("JSON");
    const auto root = document.object();
    keys(root, {"format", "schemaVersion", "payload", "sha256"});
    if (root.value("format") != "JOBS-WORKSPACE")
        invalid("format");
    if (root.value("schemaVersion") != QJsonValue(1))
        throw AppError(ErrorCode::FILE_VERSION_UNSUPPORTED);
    const auto payload = object(root.value("payload"));
    if (text(root, "sha256") != fingerprint(QJsonDocument(payload).toJson(QJsonDocument::Compact)))
        throw AppError(ErrorCode::FILE_INTEGRITY_FAILED);
    keys(payload, {"id", "projects", "resources", "bindings"});
    Workspace workspace;
    workspace.id = text(payload, "id");
    const QDir directory(baseDirectory);
    for (const auto &item : array(payload, "projects", 100)) {
        const auto ref = object(item);
        keys(ref, {"documentId", "path", "name"});
        const auto relative = text(ref, "path");
        if (relative.empty())
            invalid("project path");
        const auto full = str(QDir::cleanPath(directory.absoluteFilePath(qs(relative))));
        workspace.projects.push_back({text(ref, "documentId"), full, text(ref, "name", limit)});
    }
    for (const auto &item : array(payload, "resources", 1000)) {
        const auto value = object(item);
        keys(value, {"id", "name", "type", "maxUnits", "exceptions", "calendar"});
        SharedResource resource;
        resource.id = text(value, "id");
        resource.name = text(value, "name", limit);
        resource.type = text(value, "type");
        resource.maxUnits = units(value.value("maxUnits"));
        for (const auto &day : array(value, "exceptions", 10000)) {
            const auto entry = object(day);
            keys(entry, {"date", "units"});
            resource.exceptions.push_back({text(entry, "date"), units(entry.value("units"))});
        }
        const auto calendar = object(value.value("calendar"));
        keys(calendar, {"weekdays", "periods", "holidays"});
        resource.calendar.weekdays.clear();
        resource.calendar.periods.clear();
        for (const auto &day : array(calendar, "weekdays", 7))
            resource.calendar.weekdays.push_back(integer(day, 1, 7));
        for (const auto &period : array(calendar, "periods", 100)) {
            const auto entry = object(period);
            keys(entry, {"begin", "end"});
            resource.calendar.periods.push_back(
                {integer(entry.value("begin"), 0, 1440), integer(entry.value("end"), 0, 1440)});
        }
        for (const auto &day : array(calendar, "holidays", 10000)) {
            if (!day.isString())
                invalid("holiday");
            resource.calendar.holidays.push_back(str(day.toString()));
        }
        workspace.resources.push_back(std::move(resource));
    }
    for (const auto &item : array(payload, "bindings", 10000)) {
        const auto value = object(item);
        keys(value, {"projectId", "resourceId", "sharedId"});
        workspace.bindings.push_back({text(value, "projectId"), text(value, "resourceId"), text(value, "sharedId")});
    }
    validateWorkspace(workspace, true);
    return workspace;
}
StoredWorkspace JsonWorkspaceStore::load(const std::string &inputPath) const {
    const auto path = files_.normalizePath(inputPath);
    const auto bytes = read(path);
    auto workspace = decodeWorkspaceFile(bytes, QFileInfo(qs(path)).absolutePath());
    for (std::size_t i = 0; i < workspace.projects.size(); ++i) {
        auto &reference = workspace.projects[i];
        reference.path = files_.normalizePath(reference.path);
        for (std::size_t j = 0; j < i; ++j)
            if (files_.sameFile(reference.path, workspace.projects[j].path))
                throw AppError(ErrorCode::WORKSPACE_DUPLICATE_FILE, {reference.path});
    }
    return {std::move(workspace), path, fingerprint(bytes)};
}
std::string JsonWorkspaceStore::save(const std::string &inputPath, const Workspace &workspace,
                                     const std::string &expected) const {
    validateWorkspace(workspace, true);
    const auto path = files_.normalizePath(inputPath);
    const auto check = [&] {
        if (expected.empty()) {
            if (QFile::exists(qs(path)))
                throw AppError(ErrorCode::WORKSPACE_FILE_EXISTS, {}, ErrorContext{.path = path});
        } else if (!QFile::exists(qs(path)) || fingerprint(read(path)) != expected)
            throw AppError(ErrorCode::WORKSPACE_EXTERNAL_CHANGE, {}, ErrorContext{.path = path});
    };
    check();
    const auto payload = encode(workspace, QFileInfo(qs(path)).dir());
    const auto bytes =
        QJsonDocument(QJsonObject{{"format", "JOBS-WORKSPACE"},
                                  {"schemaVersion", 1},
                                  {"payload", payload},
                                  {"sha256", QString::fromStdString(
                                                 fingerprint(QJsonDocument(payload).toJson(QJsonDocument::Compact)))}})
            .toJson();
    if (bytes.size() > limit)
        throw AppError(ErrorCode::WORKSPACE_LIMIT);
    // Validate the exact envelope before replacing a file or reporting a successful save.
    (void)decodeWorkspaceFile(bytes, QFileInfo(qs(path)).absolutePath());
    QSaveFile file(qs(path));
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.flush())
        throw AppError(ErrorCode::FILE_OPERATION_FAILED, {str(file.errorString())}, ErrorContext{.path = path});
    check();
    if (!file.commit())
        throw AppError(ErrorCode::FILE_OPERATION_FAILED, {str(file.errorString())}, ErrorContext{.path = path});
    return fingerprint(bytes);
}
} // namespace jobs
