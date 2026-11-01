#include "ui/SandboxAccess.h"

namespace jobs {
#ifndef JOBS_MAC_APP_STORE
bool sandboxActive() {
    return false;
}
void restoreSandboxFolders() {}
bool authorizeSandboxFile(QWidget *, const QString &) {
    return true;
}
#endif
} // namespace jobs
