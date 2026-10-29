#include "updates/StoreUpdateClient.h"
#include <QElapsedTimer>
#include <QTimer>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <roapi.h>
#include <winstring.h>
#include <shobjidl.h>
#endif

namespace jobs {
#ifdef Q_OS_WIN
namespace {
// WinRT ABI slots from Microsoft's Windows.Services.Store metadata. Opaque method calls keep
// the same backend usable with MSVC and MinGW without a C++/WinRT or managed-runtime dependency.
// IStoreContext: IInspectable (6) + 17 methods before GetAppAndOptionalStorePackageUpdatesAsync.
// https://github.com/microsoft/windows-rs/blob/master/crates/libs/windows/src/Windows/Services/Store/mod.rs
template <int Slot, typename Function> Function method(IUnknown *object) {
    return reinterpret_cast<Function>((*reinterpret_cast<void ***>(object))[Slot]);
}
template <typename T> void release(T *&object) {
    if (object) {
        object->Release();
        object = nullptr;
    }
}
constexpr GUID contextStaticsId{0x9c06ee5f, 0x15c0, 0x4e72, {0x93, 0x30, 0xd6, 0x19, 0x1c, 0xeb, 0xd1, 0x9c}};
constexpr GUID asyncInfoId{0x00000036, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
class WindowsStoreClient final : public StoreUpdateClient {
public:
    WindowsStoreClient() {
        timer_.setInterval(50);
        connect(&timer_, &QTimer::timeout, this, [this] { poll(); });
    }
    ~WindowsStoreClient() override {
        cancel();
        if (initialized_)
            RoUninitialize();
    }
    void cancel() override {
        timer_.stop();
        completion_ = {};
        if (info_) {
            method<9, HRESULT(STDMETHODCALLTYPE *)(IUnknown *)>(info_)(info_);
            method<10, HRESULT(STDMETHODCALLTYPE *)(IUnknown *)>(info_)(info_);
        }
        release(info_);
        release(operation_);
        release(context_);
    }
    void check(quintptr window, Completion completion) override {
        cancel();
        completion_ = std::move(completion);
        HRESULT status = S_OK;
        if (!initialized_) {
            status = RoInitialize(RO_INIT_SINGLETHREADED);
            if (SUCCEEDED(status))
                initialized_ = true;
            else if (status != RPC_E_CHANGED_MODE) {
                done(status);
                return;
            }
        }
        HSTRING name = nullptr;
        constexpr wchar_t runtime[] = L"Windows.Services.Store.StoreContext";
        status = WindowsCreateString(runtime, UINT32(std::size(runtime) - 1), &name);
        if (FAILED(status)) {
            done(status);
            return;
        }
        IUnknown *factory = nullptr;
        status = RoGetActivationFactory(name, contextStaticsId, reinterpret_cast<void **>(&factory));
        WindowsDeleteString(name);
        if (FAILED(status)) {
            done(status);
            return;
        }
        status = method<6, HRESULT(STDMETHODCALLTYPE *)(IUnknown *, IUnknown **)>(factory)(factory, &context_);
        release(factory);
        if (FAILED(status)) {
            done(status);
            return;
        }
        IInitializeWithWindow *owner = nullptr;
        status = context_->QueryInterface(IID_IInitializeWithWindow, reinterpret_cast<void **>(&owner));
        if (SUCCEEDED(status)) {
            status = owner->Initialize(reinterpret_cast<HWND>(window));
            release(owner);
        }
        if (FAILED(status)) {
            done(status);
            return;
        }
        status = method<23, HRESULT(STDMETHODCALLTYPE *)(IUnknown *, IUnknown **)>(context_)(context_, &operation_);
        if (FAILED(status)) {
            done(status);
            return;
        }
        status = operation_->QueryInterface(asyncInfoId, reinterpret_cast<void **>(&info_));
        if (FAILED(status)) {
            done(status);
            return;
        }
        elapsed_.start();
        timer_.start();
    }

private:
    IUnknown *context_ = nullptr, *operation_ = nullptr, *info_ = nullptr;
    bool initialized_ = false;
    QTimer timer_;
    QElapsedTimer elapsed_;
    Completion completion_;
    void done(HRESULT status, unsigned count = 0) {
        auto callback = std::move(completion_);
        cancel();
        if (callback)
            callback({SUCCEEDED(status), count,
                      SUCCEEDED(status) ? QString{}
                                        : QString("Store 연결 또는 계정/배포 상태를 확인하세요. HRESULT 0x%1")
                                              .arg(quint32(status), 8, 16, QChar('0'))});
    }
    void poll() {
        int state = 0;
        auto status = method<7, HRESULT(STDMETHODCALLTYPE *)(IUnknown *, int *)>(info_)(info_, &state);
        if (FAILED(status)) {
            done(status);
            return;
        }
        if (state == 0) {
            if (elapsed_.elapsed() > 14000)
                done(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
            return;
        }
        if (state != 1) {
            HRESULT error = E_ABORT;
            if (state == 3)
                method<8, HRESULT(STDMETHODCALLTYPE *)(IUnknown *, HRESULT *)>(info_)(info_, &error);
            done(FAILED(error) ? error : E_FAIL);
            return;
        }
        IUnknown *values = nullptr;
        status = method<8, HRESULT(STDMETHODCALLTYPE *)(IUnknown *, IUnknown **)>(operation_)(operation_, &values);
        unsigned count = 0;
        if (SUCCEEDED(status) && values)
            status = method<7, HRESULT(STDMETHODCALLTYPE *)(IUnknown *, unsigned *)>(values)(values, &count);
        else if (SUCCEEDED(status))
            status = E_UNEXPECTED;
        release(values);
        done(status, count);
    }
};
} // namespace
std::unique_ptr<StoreUpdateClient> createStoreUpdateClient() {
    return std::make_unique<WindowsStoreClient>();
}
#else
namespace {
class UnavailableStoreClient final : public StoreUpdateClient {
public:
    void check(quintptr, Completion completion) override {
        completion({false, 0, "이 플랫폼에서는 Microsoft Store를 사용할 수 없습니다."});
    }
    void cancel() override {}
};
} // namespace
std::unique_ptr<StoreUpdateClient> createStoreUpdateClient() {
    return std::make_unique<UnavailableStoreClient>();
}
#endif
} // namespace jobs
