#include "ParserHarness.h"
#include "persistence/ProjectFile.h"
#include "persistence/ProjectJson.h"
#include "persistence/ProjectXml.h"
#include "persistence/PrimaveraXer.h"
#include "persistence/WorkspaceFile.h"
#include <QJsonDocument>
#include <stdexcept>

namespace jobs::fuzz {
namespace {
template <class F> void attempt(ProbeResult &result, F &&parse) {
    try {
        parse();
        ++result.accepted;
    } catch (const std::runtime_error &) {
        ++result.rejected;
    }
}
} // namespace
ProbeResult probe(std::string_view parser, const QByteArray &bytes) {
    ProbeResult result;
    if (std::size_t(bytes.size()) > maxInputBytes)
        return result;
    if (parser == "jobs") {
        attempt(result, [&] { (void)decodeProjectFile(bytes); });
        const auto json = QJsonDocument::fromJson(bytes);
        if (json.isObject()) {
            const auto root = json.object();
            const auto body = root.value("payload").isObject() ? root.value("payload").toObject() : root;
            for (int schema = 1; schema <= 9; ++schema)
                attempt(result, [&] { (void)json::decodeProject(body, schema); });
        }
    } else if (parser == "xml") {
        attempt(result, [&] { (void)importProjectXml(bytes, "fuzz.xml"); });
    } else if (parser == "xer") {
        for (auto encoding : {XerEncoding::Utf8, XerEncoding::Windows1252})
            attempt(result, [&] { (void)importPrimaveraXer(bytes, "fuzz.xer", encoding); });
    } else if (parser == "workspace") {
        attempt(result, [&] { (void)decodeWorkspaceFile(bytes, QStringLiteral("/fuzz")); });
    } else {
        throw std::logic_error("unknown parser harness");
    }
    return result;
}
} // namespace jobs::fuzz
