#ifndef TKDE_TERMUX_HPP
#define TKDE_TERMUX_HPP

#include <map>
#include <string>
#include <vector>

namespace tkde {

struct TermuxInfo {
    std::string termux_version;
    std::string android_version;
    std::string sdk_version;
    std::string device_model;
    std::string abi_list;
    bool is_termux_x11_installed = false;
    bool is_vnc_installed = false;
};

struct BatteryStatus {
    int percentage = 0;
    bool charging = false;
    int temperature_c = 0;
    std::string status;
};

class TermuxBridge {
    static std::string runCmd(const std::string& cmd);
    static int runFork(const std::string& cmd);

public:
    // Query system info
    TermuxInfo getInfo() const;
    std::string getTermuxVersion() const;
    std::string getAndroidVersion() const;
    std::string getDeviceModel() const;

    // Battery monitoring
    BatteryStatus getBatteryStatus() const;
    bool isCharging() const;
    int getBatteryPercent() const;
    int getBatteryTempC() const;

    // Wake lock (keep CPU alive)
    bool acquireWakeLock();
    bool releaseWakeLock();

    // Termux:X11 integration
    bool isTermuxX11Installed() const;
    bool launchTermuxX11(const std::string& args = "");
    bool stopTermuxX11();

    // Clipboard
    std::string getClipboard() const;
    bool setClipboard(const std::string& text);
    bool hasClipboard();

    // Notifications
    bool sendNotification(const std::string& title, const std::string& body, const std::string& id = "");
    bool clearNotification(const std::string& id);

    // Audio
    bool isPulseAudioRunning() const;
    bool startPulseAudio();
    bool stopPulseAudio();
    bool configurePulseAudio();
    bool setDefaultSink(const std::string& sink_name);
    bool setDefaultSource(const std::string& source_name);

    // WiFi display
    bool isWifiDisplayAvailable() const;
    bool castToWifiDisplay();

    // Volume control
    bool setVolume(const std::string& stream, int level); // stream: music, alarm, notification, etc.
    int getVolume(const std::string& stream) const;

    // Vibrate
    bool vibrate(int duration_ms = 100);
};

} // namespace tkde
#endif // TKDE_TERMUX_HPP