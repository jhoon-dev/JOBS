#include "ParserHarness.h"
#include <QCoreApplication>
#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    static int argc = 1;
    static char name[] = "jobs-parser-fuzzer";
    static char* argv[] = {name, nullptr};
    static QCoreApplication application(argc, argv);
    return 0;
}
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size <= jobs::fuzz::maxInputBytes)
        (void)jobs::fuzz::probe(JOBS_FUZZ_PARSER, QByteArray(reinterpret_cast<const char*>(data), qsizetype(size)));
    return 0;
}
