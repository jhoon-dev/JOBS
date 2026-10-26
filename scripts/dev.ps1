param([switch]$TestOnly)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskQt = Join-Path $taskRoot '.deps/Qt/6.8.3/mingw_64'
$taskCompiler = Join-Path $taskRoot '.deps/Qt/Tools/mingw1310_64/bin'
if (!(Test-Path -LiteralPath $taskQt) -or !(Test-Path -LiteralPath $taskCompiler)) {
    throw '로컬 Qt/MinGW가 없습니다. docs/development.md의 일반 빌드 방법을 사용하세요.'
}
$taskScripts = Join-Path $env:APPDATA 'Python/Python314/Scripts'
$taskOldPath = $env:PATH
try {
    $env:PATH = "$taskQt/bin;$taskCompiler;$taskScripts;$taskOldPath"
    cmake -S $taskRoot -B "$taskRoot/build" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=$taskQt"
    if ($LASTEXITCODE -ne 0) { throw 'CMake configure 실패' }
    cmake --build "$taskRoot/build" --parallel 4
    if ($LASTEXITCODE -ne 0) { throw '빌드 실패' }
    ctest --test-dir "$taskRoot/build" --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw '테스트 실패' }
    if (!$TestOnly) { & "$taskRoot/build/JOBS.exe" }
} finally { $env:PATH = $taskOldPath }
