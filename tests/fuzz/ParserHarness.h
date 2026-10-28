#pragma once
#include <QByteArray>
#include <string_view>

namespace jobs::fuzz {
inline constexpr std::size_t maxInputBytes = 64 * 1024;
struct ProbeResult {
    unsigned accepted = 0, rejected = 0;
};
// Only ordinary parser/domain runtime_error refusals are expected. Allocation,
// logic/index errors, cancellation and sanitizer failures must escape.
ProbeResult probe(std::string_view parser, const QByteArray& bytes);
}
