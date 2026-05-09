#ifndef TKDE_DISPLAY_HPP
#define TKDE_DISPLAY_HPP

#include <functional>
#include <string>
#include <vector>

namespace tkde {

enum class DisplayMode { TERMUX_X11, VNC, BOTH };

struct DisplayStatus {
    bool vnc_running = false;
    bool tx11_running = false;
    int vnc_pid = 0;
    int tx11_pid = 0;
    int gpu_utilization = 0;    // 0-100
    int gpu_freq_khz = 0;
    std::string gpu_model;
};

class DisplayManager {
    std::string termuxDesktopPath_;
    std::string deStartup_;

    int runScript(const std::vector<std::string>& args, int timeout_sec = 30) const;
    std::string runCommand(const std::string& cmd) const;
    bool isProcessRunning(const std::string& name) const;
    int getProcessPid(const std::string& name) const;

public:
    DisplayManager();

    // Query status
    DisplayStatus getStatus() const;

    // VNC control
    bool startVNC(bool use_gpu = true);
    bool stopVNC(bool force = false);
    bool restartVNC(bool use_gpu = true);

    // Termux:X11 control
    bool startTX11(bool use_gpu = true, bool use_dbus = true, bool legacy = false, const std::string& xstartup = "");
    bool stopTX11(bool force = false);
    bool restartTX11(bool use_gpu = true);

    // Generic
    bool start(const std::string& mode, bool use_gpu = true);
    bool stop(const std::string& mode, bool force = false);
    void setDE(const std::string& de_name, const std::string& startup) {
        deStartup_ = startup;
    }

    // Display forwarding
    bool forwardDisplay(const std::string& ip, int port = 0);

    // GPU stats
    int getGPUUtilization() const;
    int getGPUFrequencyKHz() const;
    std::string getGPUModel() const;
    bool isMaliGPU() const;

    // Event callbacks
    std::function<void(const DisplayStatus&)> onStatusChange;
};

} // namespace tkde
#endif // TKDE_DISPLAY_HPP