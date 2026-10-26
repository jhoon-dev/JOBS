#pragma once
// Shared Qt presentation helpers; the calculation/domain libraries remain Qt-free.
#include "domain/Project.h"
#include <QString>
#include <QRegularExpression>
#include <stdexcept>
namespace jobs::moneyui {
inline std::optional<std::int64_t> parse(QString text, const std::string& currency) {
    text = text.trimmed(); if (text.isEmpty()) return {};
    const int decimals = currencyDecimals(currency);
    const auto pattern = decimals ? "^[0-9]+(?:\\.[0-9]{1,2})?$" : "^[0-9]+$";
    if (!QRegularExpression(pattern).match(text).hasMatch()) throw std::runtime_error("금액은 음수·구분기호 없이 통화 정밀도에 맞춰 입력하세요. 공란은 미정입니다.");
    auto parts = text.split('.'); bool ok = false; auto whole = parts[0].toULongLong(&ok); const auto scale = decimals ? 100ULL : 1ULL;
    if (!ok || whole > std::uint64_t(maxMoney) / scale) throw std::runtime_error("금액 한도를 초과했습니다.");
    auto n = whole * scale;
    if (parts.size() == 2) n += parts[1].leftJustified(decimals, '0').toULongLong();
    if (n > std::uint64_t(maxMoney)) throw std::runtime_error("금액 한도를 초과했습니다.");
    return std::int64_t(n);
}
inline QString format(std::optional<std::int64_t> n, const std::string& currency, const QString& unknown = "—") {
    if (!n) return unknown;
    const int decimals = currencyDecimals(currency);
    if (!decimals) return QString::number(*n);
    auto value = *n < 0 ? -*n : *n;
    return (*n < 0 ? "-" : "") + QString::number(value / 100) + "." + QString::number(value % 100).rightJustified(2, '0');
}
}
