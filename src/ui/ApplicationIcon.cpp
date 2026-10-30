#include "ui/ApplicationIcon.h"
#include <QResource>

static void initializeJobsBrand() {
    Q_INIT_RESOURCE(jobs_brand);
}
namespace jobs {
QIcon applicationIcon() {
    static const bool initialized = [] {
        initializeJobsBrand();
        return true;
    }();
    Q_UNUSED(initialized);
    return QIcon(":/jobs/app.ico");
}
} // namespace jobs
