#include "biopic/version.hpp"

#include <onnxruntime_c_api.h>
#include <sqlite3.h>

#include <sstream>

#ifndef BIOPIC_VERSION_MAJOR
#define BIOPIC_VERSION_MAJOR 0
#endif
#ifndef BIOPIC_VERSION_MINOR
#define BIOPIC_VERSION_MINOR 1
#endif
#ifndef BIOPIC_VERSION_PATCH
#define BIOPIC_VERSION_PATCH 0
#endif
#ifndef BIOPIC_GIT_COMMIT
#define BIOPIC_GIT_COMMIT "unknown"
#endif
#ifndef BIOPIC_BUILD_TYPE
#define BIOPIC_BUILD_TYPE "unknown"
#endif

namespace biopic {

BuildInfo build_info() {
    BuildInfo info;
    info.major = BIOPIC_VERSION_MAJOR;
    info.minor = BIOPIC_VERSION_MINOR;
    info.patch = BIOPIC_VERSION_PATCH;
    std::ostringstream stream;
    stream << info.major << '.' << info.minor << '.' << info.patch;
    info.version_string = stream.str();
    info.git_commit = BIOPIC_GIT_COMMIT;
    info.build_type = BIOPIC_BUILD_TYPE;
    return info;
}

std::string onnx_runtime_version() {
    return OrtGetApiBase()->GetVersionString();
}

std::string sqlite_version_string() {
    return sqlite3_libversion();
}

} // namespace biopic
