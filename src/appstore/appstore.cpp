#include "appstore.hpp"
#include "logger.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
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

static std::string readFile(const std::string& p) {
    std::ifstream f(p); if (!f) return "";
    std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

} // anonymous

namespace tkde {

std::string AppStore::runCmd(const std::string& cmd) { return runOut(cmd); }
int AppStore::runFork(const std::string& cmd) { return ::runFork(cmd); }

AppStore::AppStore() {
    buildCatalog();
}

void AppStore::buildCatalog() {
    catalog_.clear();

    // Internet
    catalog_.push_back({"firefox", "Mozilla Firefox", "Web browser", "firefox", "firefox", AppCategory::INTERNET, true});
    catalog_.push_back({"chromium", "Chromium", "Web browser", "chromium", "chromium", AppCategory::INTERNET, true});
    catalog_.push_back({"w3m", "w3m", "Terminal web browser", "w3m", "", AppCategory::INTERNET, true});

    // Multimedia
    catalog_.push_back({"vlc", "VLC Media Player", "Video player", "vlc", "vlc", AppCategory::MULTIMEDIA, true});
    catalog_.push_back({"mpv", "mpv", "Video player", "mpv", "mpv", AppCategory::MULTIMEDIA, true});
    catalog_.push_back({"gimp", "GIMP", "Image editor", "gimp", "gimp", AppCategory::MULTIMEDIA, true});
    catalog_.push_back({"inkscape", "Inkscape", "Vector graphics", "inkscape", "inkscape", AppCategory::MULTIMEDIA, true});
    catalog_.push_back({"audacity", "Audacity", "Audio editor", "audacity", "audacity", AppCategory::MULTIMEDIA, true});
    catalog_.push_back({"ffmpeg", "FFmpeg", "Audio/video converter", "ffmpeg", "", AppCategory::MULTIMEDIA, true});

    // Development
    catalog_.push_back({"neovim", "Neovim", "Text editor", "nvim", "neovim", AppCategory::DEVELOPMENT, true});
    catalog_.push_back({"vscode", "VS Code", "IDE", "code", "code", AppCategory::DEVELOPMENT, false, "ubuntu"});
    catalog_.push_back({"geany", "Geany", "Lightweight IDE", "geany", "geany", AppCategory::DEVELOPMENT, true});
    catalog_.push_back({"git", "Git", "Version control", "git", "", AppCategory::DEVELOPMENT, true});
    catalog_.push_back({"python", "Python", "Python interpreter", "python3", "", AppCategory::DEVELOPMENT, true});
    catalog_.push_back({"node", "Node.js", "JavaScript runtime", "node", "", AppCategory::DEVELOPMENT, true});

    // Office
    catalog_.push_back({"libreoffice", "LibreOffice", "Office suite", "lo", "libreoffice", AppCategory::OFFICE, false, "ubuntu"});
    catalog_.push_back({"evince", "Evince", "PDF viewer", "evince", "evince", AppCategory::OFFICE, false, "ubuntu"});
    catalog_.push_back({"zathura", "Zathura", "PDF viewer", "zathura", "zathura", AppCategory::OFFICE, true});

    // Utility
    catalog_.push_back({"htop", "htop", "System monitor", "htop", "", AppCategory::UTILITY, true});
    catalog_.push_back({"neofetch", "Neofetch", "System info", "neofetch", "", AppCategory::UTILITY, true});
    catalog_.push_back({"ranger", "ranger", "File manager", "ranger", "", AppCategory::UTILITY, true});
    catalog_.push_back({"starship", "Starship", "Cross-shell prompt", "starship", "", AppCategory::UTILITY, true});

    // System
    catalog_.push_back({"gparted", "GParted", "Partition editor", "gparted", "gparted", AppCategory::SYSTEM, false, "debian"});
    catalog_.push_back({"systemsettings", "System Settings", "KDE system settings", "systemsettings", "preferences-system", AppCategory::SYSTEM, true});
    catalog_.push_back({"plasma-discover", "Discover", "App store", "plasma-discover", "plasma-discover", AppCategory::SYSTEM, true});
    catalog_.push_back({"konsole", "Konsole", "Terminal emulator", "konsole", "utilities-terminal", AppCategory::SYSTEM, true});
    catalog_.push_back({"dolphin", "Dolphin", "File manager", "dolphin", "system-file-manager", AppCategory::SYSTEM, true});

    // Check installed status
    for (auto& app : catalog_) {
        if (app.in_termux) {
            app.installed = runFork("which " + app.exec + " 2>/dev/null") == 0;
        } else {
            app.installed = runFork("proot-distro list 2>/dev/null | grep -q " + app.distro) == 0
                && runFork("proot-distro login " + app.distro + " -- command -v " + app.exec + " 2>/dev/null") == 0;
        }
    }

    TKDE_INFO("App catalog built: %zu apps", catalog_.size());
}

std::vector<DesktopApp> AppStore::getApps(AppCategory cat) const {
    std::vector<DesktopApp> results;
    for (const auto& app : catalog_) {
        if (app.category == cat) results.push_back(app);
    }
    return results;
}

std::vector<DesktopApp> AppStore::search(const std::string& query) const {
    std::vector<DesktopApp> results;
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);
    for (const auto& app : catalog_) {
        std::string name = app.name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::string desc = app.description;
        std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);
        if (name.find(q) != std::string::npos || desc.find(q) != std::string::npos) {
            results.push_back(app);
        }
    }
    return results;
}

DesktopApp AppStore::getApp(const std::string& id) const {
    for (const auto& app : catalog_) {
        if (app.id == id) return app;
    }
    return {};
}

bool AppStore::installApp(const std::string& id) {
    auto app = getApp(id);
    if (app.id.empty()) {
        TKDE_ERROR("Unknown app: %s", id.c_str());
        return false;
    }

    TKDE_INFO("Installing app: %s (%s)", app.name.c_str(), app.exec.c_str());

    if (app.in_termux) {
        // Termux native - use pkg
        std::string pkg = app.exec;
        if (id == "neovim") pkg = "neovim";
        else if (id == "firefox") pkg = "firefox";
        else if (id == "chromium") pkg = "chromium";
        else if (id == "vlc") pkg = "vlc";
        else if (id == "mpv") pkg = "mpv";
        else if (id == "gimp") pkg = "gimp";
        else if (id == "git") pkg = "git";
        else if (id == "python") pkg = "python";
        else if (id == "node") pkg = "node";
        else if (id == "htop") pkg = "htop";
        else if (id == "neofetch") pkg = "neofetch";
        else if (id == "ranger") pkg = "ranger";

        return runFork("pkg install " + pkg + " -y 2>/dev/null") == 0;
    } else {
        // Proot-distro app
        std::string cmd;
        if (app.distro == "ubuntu") {
            cmd = "apt update && apt install -y " + app.exec;
        } else if (app.distro == "debian") {
            cmd = "apt update && apt install -y " + app.exec;
        } else if (app.distro == "archlinux") {
            cmd = "pacman -Sy --noconfirm " + app.exec;
        } else if (app.distro == "fedora") {
            cmd = "dnf install -y " + app.exec;
        } else {
            cmd = app.exec;
        }
        return runFork("proot-distro login " + app.distro + " -- " + cmd + " 2>/dev/null") == 0;
    }
}

bool AppStore::removeApp(const std::string& id) {
    auto app = getApp(id);
    if (app.id.empty()) return false;

    TKDE_INFO("Removing app: %s", id.c_str());

    if (app.in_termux) {
        return runFork("pkg uninstall " + app.exec + " -y 2>/dev/null") == 0;
    } else {
        // For distro apps, just remove from menu
        return removeDistroAppFromMenu(id);
    }
}

std::vector<DesktopApp> AppStore::getDistroApps(const std::string& distro) const {
    std::vector<DesktopApp> results;
    for (const auto& app : catalog_) {
        if (!app.in_termux && app.distro == distro) results.push_back(app);
    }
    return results;
}

bool AppStore::addDistroAppToMenu(const std::string& distro, const std::string& desktop_file) {
    TKDE_INFO("Adding distro app to menu: %s from %s", desktop_file.c_str(), distro.c_str());
    return runFork(
        "cp '" + desktop_file + "' /data/data/com.termux/files/home/.local/share/applications/ 2>/dev/null && "
        "update-desktop-database /data/data/com.termux/files/home/.local/share/applications/ 2>/dev/null") == 0;
}

bool AppStore::removeDistroAppFromMenu(const std::string& app_id) {
    std::string menu_path = "/data/data/com.termux/files/home/.local/share/applications/" + app_id + ".desktop";
    return runFork("rm -f '" + menu_path + "' 2>/dev/null") == 0;
}

bool AppStore::installWine(const std::string& variant) {
    TKDE_INFO("Installing Wine: %s", variant.c_str());
    if (variant == "stock") {
        return runFork("proot-distro login ubuntu -- apt install -y wine64 2>/dev/null") == 0;
    } else if (variant == "mobox") {
        return runFork(
            "pkg install wget unzip 2>/dev/null && "
            "wget -q https://github.com/tonymtz/mobox/releases/latest/download/mobox.zip -O /tmp/mobox.zip && "
            "unzip -o /tmp/mobox.zip -d /data/data/com.termux/files/home/mobox 2>/dev/null") == 0;
    } else if (variant == "wine_hangover") {
        return runFork(
            "pkg install wine 2>/dev/null") == 0
            || runFork("proot-distro login ubuntu -- apt install -y winehq-winelinker 2>/dev/null") == 0;
    }
    return false;
}

bool AppStore::installWindowsApp(const std::string& exe_path, const std::string& name) {
    TKDE_WARN("Installing Windows app via Wine is experimental on Android");
    return runFork(
        "WINEPREFIX=/data/data/com.termux/files/home/.wine "
        "WINEARCH=win64 "
        "proot-distro login ubuntu -- wine " + exe_path + " 2>/dev/null &") == 0;
}

std::vector<WineApp> AppStore::listWineApps() const {
    std::vector<WineApp> apps;
    std::string prefix = "/data/data/com.termux/files/home/.wine/drive_c/ProgramData/Microsoft/Windows/Start Menu/Programs";
    std::string out = runOut("find '" + prefix + "' -name '*.lnk' 2>/dev/null | head -20");
    // Simple parsing - just extract names
    std::string cur;
    for (char c : out) {
        if (c == '\n') {
            if (!cur.empty()) {
                WineApp w;
                w.name = cur;
                w.wine_prefix = "/data/data/com.termux/files/home/.wine";
                apps.push_back(w);
                cur.clear();
            }
        } else cur += c;
    }
    return apps;
}

bool AppStore::updateAppIndex() {
    TKDE_INFO("Updating app index");
    buildCatalog();
    return true;
}

bool AppStore::launchApp(const std::string& id) {
    auto app = getApp(id);
    if (app.id.empty()) return false;

    TKDE_INFO("Launching app: %s", app.name.c_str());

    if (app.in_termux) {
        return runFork(app.exec + " &") == 0;
    } else {
        // Launch in proot-distro via pdrun
        std::string cmd = "pdrun " + app.exec;
        if (!app.distro.empty()) {
            cmd = "proot-distro login " + app.distro + " -- " + app.exec;
        }
        return runFork(cmd + " &") == 0;
    }
}

} // namespace tkde