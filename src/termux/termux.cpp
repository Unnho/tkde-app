#include "termux.hpp"
#include "logger.hpp"
#include <algorithm>
#include <cstdio>
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

static std::string readFile(const std::string& p) {
    std::ifstream f(p); if (!f) return "";
    std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

} // anonymous

namespace tkde {

std::string TermuxBridge::runCmd(const std::string& cmd) { return runOut(cmd); }
int TermuxBridge::runFork(const std::string& cmd) { return ::runFork(cmd); }

TermuxInfo TermuxBridge::getInfo() const {
    TermuxInfo i;
    i.termux_version = trim(runOut("cat /data/data/com.termux/files/usr/etc/termux/termux.properties 2>/dev/null | grep version | head -1 | cut -d= -f2"));
    i.android_version = trim(runOut("getprop ro.build.version.release 2>/dev/null"));
    i.sdk_version = trim(runOut("getprop ro.build.version.sdk 2>/dev/null"));
    i.device_model = trim(runOut("getprop ro.product.model 2>/dev/null"));
    i.abi_list = trim(runOut("getprop ro.product.cpu.abi 2>/dev/null"));
    i.is_termux_x11_installed = access("/data/data/com.termux/files/usr/bin/termux-x11", X_OK) == 0;
    i.is_vnc_installed = access("/data/data/com.termux/files/usr/bin/vncserver", X_OK) == 0;
    return i;
}

std::string TermuxBridge::getTermuxVersion() const { return getInfo().termux_version; }
std::string TermuxBridge::getAndroidVersion() const { return getInfo().android_version; }
std::string TermuxBridge::getDeviceModel() const { return getInfo().device_model; }

BatteryStatus TermuxBridge::getBatteryStatus() const {
    BatteryStatus b;
    std::string cap_str = trim(readFile("/sys/class/battery/capacity"));
    if (!cap_str.empty()) {
        char* e=nullptr; long v=strtol(cap_str.c_str(),&e,10); if(*e==0) b.percentage=static_cast<int>(v);
    }
    std::string status = trim(readFile("/sys/class/battery/status"));
    b.charging = (status.find("Charging") != std::string::npos || status.find("Full") != std::string::npos);
    b.status = status;
    std::string temp_str = trim(readFile("/sys/class/battery/temp"));
    if (!temp_str.empty()) {
        char* e=nullptr; long v=strtol(temp_str.c_str(),&e,10); if(*e==0) b.temperature_c=static_cast<int>(v);
    }
    return b;
}

bool TermuxBridge::isCharging() const { return getBatteryStatus().charging; }
int TermuxBridge::getBatteryPercent() const { return getBatteryStatus().percentage; }
int TermuxBridge::getBatteryTempC() const { return getBatteryStatus().temperature_c; }

bool TermuxBridge::acquireWakeLock() {
    TKDE_INFO("Acquiring wake lock");
    return runFork("termux-wake-lock 2>/dev/null") == 0
        || runFork("echo 'lock' > /sys/power/wake_lock 2>/dev/null") == 0;
}

bool TermuxBridge::releaseWakeLock() {
    TKDE_INFO("Releasing wake lock");
    return runFork("termux-wake-unlock 2>/dev/null") == 0
        || runFork("echo 'lock' > /sys/power/wake_unlock 2>/dev/null") == 0;
}

bool TermuxBridge::isTermuxX11Installed() const {
    return access("/data/data/com.termux/files/usr/bin/termux-x11", X_OK) == 0;
}

bool TermuxBridge::launchTermuxX11(const std::string& args) {
    TKDE_INFO("Launching Termux:X11");
    return runFork("termux-x11 :0 " + args + " &") == 0
        && runFork("am start --user 0 -n com.termux.x11/com.termux.x11.MainActivity 2>/dev/null &") == 0;
}

bool TermuxBridge::stopTermuxX11() {
    return runFork("pkill -9 com.termux.x11 2>/dev/null") == 0;
}

std::string TermuxBridge::getClipboard() const {
    return runOut("termux-clipboard-get 2>/dev/null");
}

bool TermuxBridge::setClipboard(const std::string& text) {
    // Escape single quotes in the text
    std::string escaped = text;
    for (size_t pos = 0; (pos = escaped.find('\'', pos)) != std::string::npos; pos += 3) {
        escaped.replace(pos, 1, "'\\''");
    }
    return runFork("echo '" + escaped + "' | termux-clipboard-set 2>/dev/null") == 0;
}

bool TermuxBridge::hasClipboard() {
    return !getClipboard().empty();
}

bool TermuxBridge::sendNotification(const std::string& title, const std::string& body, const std::string& id) {
    std::string nid = id.empty() ? "tkde-app" : id;
    return runFork(
        "termux-notification --title '" + title + "' --body '" + body + "' --id '" + nid + "' 2>/dev/null") == 0
        || runFork(
            "notification-send '" + title + "' '" + body + "' 2>/dev/null") == 0;
}

bool TermuxBridge::clearNotification(const std::string& id) {
    return runFork("termux-notification-remove " + id + " 2>/dev/null") == 0;
}

bool TermuxBridge::isPulseAudioRunning() const {
    return runFork("pgrep -f pulseaudio > /dev/null 2>&1") == 0;
}

bool TermuxBridge::startPulseAudio() {
    TKDE_INFO("Starting PulseAudio");
    runFork("pulseaudio --kill 2>/dev/null");
    runFork("rm -rf /data/data/com.termux/files/home/.config/pulse/*-runtime/pid");
    return runFork("pulseaudio --start --exit-idle-time=-1 2>/dev/null") == 0
        && runFork("pacmd load-module module-native-protocol-tcp auth-ip-acl=127.0.0.1 auth-anonymous=1 2>/dev/null") == 0
        && runFork("pactl load-module module-sles-source 2>/dev/null") == 0;
}

bool TermuxBridge::stopPulseAudio() {
    return runFork("pulseaudio --kill 2>/dev/null") == 0;
}

bool TermuxBridge::configurePulseAudio() {
    TKDE_INFO("Configuring PulseAudio");
    // This mirrors the termux-desktop setup
    runOut("mkdir -p /data/data/com.termux/files/home/.config/pulse");
    runOut("pactl load-module module-sles-source 2>/dev/null");
    return true;
}

bool TermuxBridge::setDefaultSink(const std::string& sink_name) {
    return runFork("pactl set-default-sink " + sink_name + " 2>/dev/null") == 0;
}

bool TermuxBridge::setDefaultSource(const std::string& source_name) {
    return runFork("pactl set-default-source " + source_name + " 2>/dev/null") == 0;
}

bool TermuxBridge::isWifiDisplayAvailable() const {
    return runFork("dumpsys display | grep -q mDisplayId 2>/dev/null") == 0;
}

bool TermuxBridge::castToWifiDisplay() {
    return runFork("am start --user 0 -n com.android.settings/.DisplaySettings 2>/dev/null") == 0;
}

bool TermuxBridge::setVolume(const std::string& stream, int level) {
    return runFork("termux-volume " + stream + " " + std::to_string(level) + " 2>/dev/null") == 0
        || runFork("media volume --set " + std::to_string(level) + " 2>/dev/null") == 0;
}

int TermuxBridge::getVolume(const std::string& stream) const {
    std::string out = runOut("termux-volume " + stream + " 2>/dev/null | head -1");
    if (!out.empty()) {
        size_t colon = out.find(':');
        if (colon != std::string::npos) {
            std::string num = trim(out.substr(colon + 1));
            char* e = nullptr; long v = strtol(num.c_str(), &e, 10);
            if (*e == '\0') return static_cast<int>(v);
        } else {
            char* e = nullptr; long v = strtol(out.c_str(), &e, 10);
            if (*e == '\0') return static_cast<int>(v);
        }
    }
    return -1;
}

bool TermuxBridge::vibrate(int duration_ms) {
    return runFork("termux-vibrate -d " + std::to_string(duration_ms) + " 2>/dev/null") == 0
        || runFork("input keyevent 6 2>/dev/null") == 0;
}

} // namespace tkde