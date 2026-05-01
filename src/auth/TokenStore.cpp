#include "TokenStore.h"

#ifdef Q_OS_WIN
#include "WindowsTokenStore.h"
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include "LinuxTokenStore.h"
#endif

namespace x4::auth {
    TokenStore *createTokenStore() {
#ifdef Q_OS_WIN
        return new WindowsTokenStore();
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        return new LinuxTokenStore();
#else
        return nullptr; // Unsupported platform
#endif
    }
} // namespace x4::auth