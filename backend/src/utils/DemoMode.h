#pragma once

namespace demo_mode {
inline constexpr bool kDemoEnabled = false;
inline constexpr bool kViewerModeEnabled = false;
inline constexpr bool autoLogin = true;
inline constexpr bool kReadOnlyMode = kViewerModeEnabled || autoLogin;
inline constexpr bool disableClaimHistoryGuideAndReport = true;
}
