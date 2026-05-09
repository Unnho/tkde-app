#include "kde.hpp"
#include "logger.hpp"
#include <algorithm>
#include <cstdio>
#include <thread>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include <sstream>

namespace {

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::string runOut(const std::string& cmd) {
    char buf[4096];
    std::string r;
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return "";
    while (fgets(buf, sizeof(buf), p)) r += buf;
    pclose(p);
    return trim(r);
}

static int runFork(const std::string& cmd) {
    return WEXITSTATUS(pclose(popen(cmd.c_str(), "r")));
}

} // anonymous namespace

namespace tkde {

KDEManager::KDEManager()
    : kdeConfigPath_(getenv("HOME") ? std::string(getenv("HOME")) + "/.config" : "/data/data/com.termux/files/home/.config")
    , kdeDataPath_(getenv("HOME") ? std::string(getenv("HOME")) + "/.local/share" : "/data/data/com.termux/files/home/.local/share")
    , kwriteconfig_("/data/data/com.termux/files/usr/bin/kwriteconfig5")
    , kreadconfig_("/data/data/com.termux/files/usr/bin/kreadconfig5")
{
    // Try to find kwriteconfig
    if (access(kwriteconfig_.c_str(), X_OK) != 0) {
        kwriteconfig_ = "kwriteconfig5";
    }
    if (access(kreadconfig_.c_str(), X_OK) != 0) {
        kreadconfig_ = "kreadconfig5";
    }
}

KDEManager::~KDEManager() = default;

bool KDEManager::isSessionActive() const {
    return runFork("pgrep -f 'startplasma-x11' > /dev/null 2>&1") == 0
        && runFork("pgrep -f 'kwin_x11' > /dev/null 2>&1") == 0;
}

bool KDEManager::startSession(const std::string& de_name) {
    TKDE_INFO("Starting KDE session: %s", de_name.c_str());

    // Export required env vars
    setenv("XDG_CONFIG_DIRS", "/data/data/com.termux/files/usr/etc/xdg", 1);
    setenv("XDG_RUNTIME_DIR", "/data/data/com.termux/files/usr/tmp", 1);
    setenv("DISPLAY", ":1", 1);

    // Start D-Bus session
    runOut("mkdir -p /data/data/com.termux/files/usr/tmp");
    std::string dbus_launch = runOut("dbus-launch --exit-with-session");
    if (!dbus_launch.empty()) {
        TKDE_INFO("D-Bus session: %s", dbus_launch.c_str());
    }

    std::string de_startup;
    if (de_name == "plasma") de_startup = "startplasma-x11";
    else if (de_name == "xfce") de_startup = "startxfce4";
    else if (de_name == "lxqt") de_startup = "startlxqt";
    else if (de_name == "mate") de_startup = "mate-session";
    else de_startup = de_name;

    int rc = runFork(de_startup + " &");
    sessionValid_ = (rc == 0);
    return sessionValid_;
}

bool KDEManager::stopSession(bool force) {
    TKDE_INFO("Stopping KDE session (force=%d)", force);
    if (force) {
        return runFork(
            "pkill -9 startplasma-x11; "
            "pkill -9 kwin_x11; "
            "pkill -9 plasma-desktop; "
            "pkill -9 krunner; "
            "pkill -9 baloo; "
            "pkill -9 knotificationservice; "
            "pkill -9 klipper; "
            "pkill -9 powerdevil"
        ) == 0;
    }
    return runFork("qdbus org.kde.ksmserver /KSMServer logout 0 0 0") == 0;
}

bool KDEManager::restartSession() {
    return stopSession(true) && startSession("plasma");
}

KDEPlasmaSession KDEManager::querySession() const {
    KDEPlasmaSession s;

    s.kwin_running = runFork("pgrep -f 'kwin_x11' > /dev/null 2>&1") == 0;
    s.plasma_running = runFork("pgrep -f 'plasma-desktop' > /dev/null 2>&1") == 0;
    s.krunner_running = runFork("pgrep -f 'krunner' > /dev/null 2>&1") == 0;
    s.x11 = true;

    s.kde_version = runOut("plasma-desktop --version 2>/dev/null | head -1");
    s.kde_session_id = runOut("qdbus org.kde.ksmserver /KSMServer org.kde.KSMServerInterface.sessionGroup 2>/dev/null");

    // Component list
    const char* components[] = {
        "kwin_x11", "plasma-desktop", "krunner", "baloo", "kded5",
        "knotificationservice", "klipper", "powerdevil", "solid", "kscreen"
    };
    for (auto c : components) {
        KDEResource r;
        r.name = c;
        r.running = runFork(std::string("pgrep -f '") + c + "' > /dev/null 2>&1") == 0;
        r.pid = 0;
        std::string pid_str = runOut("pgrep '" + std::string(c) + "' 2>/dev/null | head -1");
        if (!pid_str.empty()) {
            char* e=nullptr; long p=strtol(pid_str.c_str(),&e,10); if(*e==0) r.pid=static_cast<int>(p);
        }
        s.components.push_back(r);
    }

    return s;
}

std::vector<KDEResource> KDEManager::listKDEComponents() const {
    return querySession().components;
}

bool KDEManager::isComponentRunning(const std::string& name) const {
    return runFork("pgrep -f '" + name + "' > /dev/null 2>&1") == 0;
}

bool KDEManager::restartComponent(const std::string& name) {
    TKDE_INFO("Restarting component: %s", name.c_str());
    runFork("pkill -f '" + name + "'");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return runFork(name + " &") == 0;
}

bool KDEManager::killComponent(const std::string& name) {
    return runFork("pkill -9 '" + name + "'") == 0;
}

bool KDEManager::isCompositorEnabled() const {
    std::string out = runOut(
        "qdbus org.kde.KWin /KWin org.kde.KWin.Compositing.active 2>/dev/null");
    return out == "true" || out == "1";
}

bool KDEManager::enableCompositor() {
    return runFork("qdbus org.kde.KWin /KWin org.kde.KWin.Compositing.activate 2>/dev/null") == 0;
}

bool KDEManager::disableCompositor() {
    return runFork("qdbus org.kde.KWin /KWin org.kde.KWin.Compositing.suspend 2>/dev/null") == 0;
}

bool KDEManager::setCompositorBackend(const std::string& backend) {
    std::string cmd = kwriteconfig_ + " --file kwinrc --group Compositing --key Backend " + backend;
    TKDE_INFO("Setting compositor backend: %s", backend.c_str());
    int rc = runFork(cmd);
    if (rc == 0) {
        return restartComponent("kwin_x11");
    }
    return rc == 0;
}

KDEConfig KDEManager::readKDEConfig() const {
    KDEConfig cfg;

    cfg.theme_name = runOut(kreadconfig_ + " --file plasma-desktop-appletsrc --group General --key theme 2>/dev/null");
    cfg.icon_theme = runOut(kreadconfig_ + " --file plasma-desktop-appletsrc --group Icons --key Theme 2>/dev/null");
    cfg.cursor_theme = runOut(kreadconfig_ + " --filekcmshell6 cursortheme 2>/dev/null | head -1");
    cfg.font_dpi = runOut(kreadconfig_ + " --filefonts.hwcomposer --group General --key dpi 2>/dev/null");
    cfg.desktop_count = 4; // default
    cfg.compositor_enabled = isCompositorEnabled();

    // Wallpaper from plasma-desktop-appletsrc
    std::string wallpaper_line = runOut("grep 'Image=' " + kdeConfigPath_ + "/plasma-desktop-appletsrc 2>/dev/null | head -1");
    if (!wallpaper_line.empty()) {
        size_t eq = wallpaper_line.find('=');
        if (eq != std::string::npos) {
            cfg.wallpaper_path = trim(wallpaper_line.substr(eq + 1));
        }
    }

    return cfg;
}

bool KDEManager::writeKDEConfig(const KDEConfig& cfg) {
    if (!cfg.theme_name.empty()) setTheme(cfg.theme_name);
    if (!cfg.icon_theme.empty()) setIconTheme(cfg.icon_theme);
    if (!cfg.cursor_theme.empty()) setCursorTheme(cfg.cursor_theme);
    if (!cfg.font_dpi.empty()) setFontDPI(static_cast<int>(strtol(cfg.font_dpi.c_str(), nullptr, 10)));
    if (!cfg.wallpaper_path.empty()) setWallpaper(cfg.wallpaper_path);
    TKDE_INFO("KDE config written");
    return true;
}

bool KDEManager::setTheme(const std::string& theme_name) {
    TKDE_INFO("Setting theme: %s", theme_name.c_str());
    return runFork(kwriteconfig_ + " --file plasma-desktop-appletsrc --group General --key theme --type string " + theme_name) == 0
        && runFork("plasmathemesetup " + theme_name + " 2>/dev/null &") == 0;
}

bool KDEManager::setWallpaper(const std::string& path) {
    TKDE_INFO("Setting wallpaper: %s", path.c_str());
    return runFork("plasma_change_wallpaper '" + path + "' 2>/dev/null &") == 0;
}

bool KDEManager::setFontDPI(int dpi) {
    std::string dpi_str = std::to_string(dpi);
    TKDE_INFO("Setting font DPI: %d", dpi);
    runFork(kwriteconfig_ + " --file fonts.hwcomposer --group General --key dpi --type int " + dpi_str);
    // Also export for X11
    runOut("export QT_FONT_DPI=" + dpi_str);
    return true;
}

bool KDEManager::setIconTheme(const std::string& theme_name) {
    return runFork(kwriteconfig_ + " --file plasma-desktop-appletsrc --group Icons --key Theme --type string " + theme_name) == 0
        && runFork("kiconsettings " + theme_name + " 2>/dev/null &") == 0;
}

bool KDEManager::setCursorTheme(const std::string& theme_name) {
    return runFork(kwriteconfig_ + " --filekcmshell6 cursortheme --group 'KDE Global Settings' --key CursorTheme --type string " + theme_name) == 0;
}

bool KDEManager::setDesktopCount(int count) {
    TKDE_INFO("Setting desktop count: %d", count);
    return runFork(
        "qdbus org.kde.plasmashell /Plasma "
        "org.kde.PlasmaShell.setNumberOfDesktops " + std::to_string(count) + " 2>/dev/null") == 0;
}

bool KDEManager::setPanelVisible(const std::string& panel_id, bool visible) {
    std::string visible_str = visible ? "true" : "false";
    return runFork(
        "qdbus org.kde.plasmashell /Plasma/Container/" + panel_id +
        " org.freedesktop.DBus.Properties.Set "
        "org.kde.plasma.shell.Container visible " + visible_str + " 2>/dev/null") == 0;
}

bool KDEManager::addDesktopActivity(const std::string& name) {
    return runFork(
        "qdbus org.kde.plasmashell /Plasma "
        "org.kde.PlasmaShell.createDesktopActivity 2>/dev/null") == 0;
}

bool KDEManager::isKRunnerActive() const {
    return isComponentRunning("krunner");
}

bool KDEManager::triggerKRunner(const std::string& query) const {
    return runFork("qdbus org.kde.krunner /Runner org.kde.krunner.Runner.executes 2>/dev/null '" + query + "'") == 0;
}

bool KDEManager::reloadKRunnerIndex() {
    return runFork("balooctl disable; balooctl enable; balooctl index 2>/dev/null &") == 0;
}

bool KDEManager::setPowerProfile(const std::string& profile) {
    TKDE_INFO("Setting power profile: %s", profile.c_str());
    return runFork("powerprofilesctl set " + profile + " 2>/dev/null") == 0
        || runFork("powerdevil -p " + profile + " 2>/dev/null &") == 0;
}

bool KDEManager::configureTouchpadSensitivity(int level) {
    return runFork(
        "synclient AccelFactor=" + std::to_string(level / 10.0) + " 2>/dev/null") == 0;
}

bool KDEManager::applyCatppuccin(const std::string& variant) {
    TKDE_INFO("Applying Catppuccin theme: %s", variant.c_str());
    // Download and install Catppuccin KDE theme
    std::string install_cmd =
        "git clone --depth 1 https://github.com/catppuccin/kde.git /tmp/catppuccin-kde 2>/dev/null && "
        "cp -r /tmp/catppuccin-kde ~/.local/share/plasma/desktoptheme/ 2>/dev/null && "
        "rm -rf /tmp/catppuccin-kde";
    return runFork(install_cmd) == 0 && setTheme("Catppuccin-" + variant);
}

bool KDEManager::applyNord(const std::string&) {
    TKDE_INFO("Applying Nord theme");
    return runFork(
        "git clone --depth 1 https://github.com/Elena--/nord-kde.git /tmp/nord-kde 2>/dev/null && "
        "cp -r /tmp/nord-kde ~/.local/share/plasma/desktoptheme/ 2>/dev/null && "
        "rm -rf /tmp/nord-kde") == 0 && setTheme("nord");
}

bool KDEManager::applyDracula() {
    TKDE_INFO("Applying Dracula theme");
    return runFork(
        "git clone --depth 1 https://github.com/dracula/kde.git /tmp/dracula-kde 2>/dev/null && "
        "cp -r /tmp/dracula-kde ~/.local/share/plasma/desktoptheme/ 2>/dev/null && "
        "rm -rf /tmp/dracula-kde") == 0 && setTheme("Dracula");
}

bool KDEManager::applyTokyoNight() {
    TKDE_INFO("Applying Tokyo Night theme");
    return runFork(
        "git clone --depth 1 https://github.com/Enoch-Chang/TokyoNight-KDE.git /tmp/tokyonight-kde 2>/dev/null && "
        "cp -r /tmp/tokyonight-kde ~/.local/share/plasma/desktoptheme/ 2>/dev/null && "
        "rm -rf /tmp/tokyonight-kde") == 0 && setTheme("TokyoNight");
}

bool KDEManager::installWidget(const std::string& package_path) {
    return runFork("plasmapkg2 -t kwinscript -i '" + package_path + "' 2>/dev/null") == 0
        || runFork("plasmapkg2 -t plasma-applet -i '" + package_path + "' 2>/dev/null") == 0;
}

bool KDEManager::uninstallWidget(const std::string& widget_id) {
    return runFork("plasmapkg2 -r '" + widget_id + "' 2>/dev/null") == 0;
}

std::vector<std::string> KDEManager::listInstalledWidgets() const {
    std::string out = runOut("plasmapkg2 --list 2>/dev/null");
    std::vector<std::string> widgets;
    std::string cur;
    for (char c : out) {
        if (c == '\n') {
            if (!cur.empty()) widgets.push_back(cur);
            cur.clear();
        } else cur += c;
    }
    if (!cur.empty()) widgets.push_back(cur);
    return widgets;
}

} // namespace tkde