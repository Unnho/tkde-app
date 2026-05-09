#include "display.hpp"
#include "logger.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

namespace tkde {

// ─── Utility helpers ───────────────────────────────────────────────────────────

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string readProcFile(const std::string& path) {
    return readFile(path);
}

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static int runFork(const std::string& cmd) {
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return -1;
    int rc = pclose(p);
    return WEXITSTATUS(rc);
}

static std::string runOutput(const std::string& cmd) {
    char buf[4096];
    std::string out;
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return "";
    while (fgets(buf, sizeof(buf), p)) out += buf;
    pclose(p);
    return trim(out);
}

// ─── DisplayManager ─────────────────────────────────────────────────────────────

DisplayManager::DisplayManager() {
    termuxDesktopPath_ = "/data/data/com.termux/files/usr/etc/termux-desktop";
    deStartup_ = "startplasma-x11";
}

bool DisplayManager::isProcessRunning(const std::string& name) const {
    std::string cmd = "pgrep -f '" + name + "' > /dev/null 2>&1";
    return runFork(cmd) == 0;
}

int DisplayManager::getProcessPid(const std::string& name) const {
    int pid = runFork("pgrep -f '" + name + "' 2>/dev/null");
    return pid;
}

int DisplayManager::runScript(const std::vector<std::string>& args, int timeout_sec) const {
    if (args.empty()) return -1;
    const char* argv[32];
    for (size_t i = 0; i < args.size() && i < 31; ++i) argv[i] = args[i].c_str();
    argv[args.size()] = nullptr;

    pid_t pid;
    int rc = posix_spawn(&pid, argv[0], nullptr, nullptr, const_cast<char* const*>(argv), nullptr);
    if (rc != 0) {
        TKDE_ERROR("posix_spawn failed: %s", strerror(rc));
        return -1;
    }

    int status;
    timespec ts = { timeout_sec, 0 };
    if ( TEMP_FAILURE_RETRY(waitpid(pid, &status, 0)) == -1) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

std::string DisplayManager::runCommand(const std::string& cmd) const {
    return runOutput(cmd);
}

DisplayStatus DisplayManager::getStatus() const {
    DisplayStatus s;

    s.vnc_running = isProcessRunning("Xvnc");
    s.tx11_running = isProcessRunning("com.termux.x11") || isProcessRunning("termux-x11");

    s.vnc_pid = 0;
    std::string vnc_pid_str = runOutput("pgrep -f 'Xvnc' 2>/dev/null | head -1");
    if (!vnc_pid_str.empty()) {
        char* e=nullptr; long p=strtol(vnc_pid_str.c_str(),&e,10); if(*e==0) s.vnc_pid=static_cast<int>(p);
    }

    s.tx11_pid = 0;
    std::string tx11_pid_str = runOutput("pgrep -f 'com.termux.x11' 2>/dev/null | head -1");
    if (!tx11_pid_str.empty()) {
        char* e2=nullptr; long p2=strtol(tx11_pid_str.c_str(),&e2,10); if(*e2==0) s.tx11_pid=static_cast<int>(p2);
    }

    s.gpu_utilization = getGPUUtilization();
    s.gpu_freq_khz = getGPUFrequencyKHz();
    s.gpu_model = getGPUModel();

    return s;
}

bool DisplayManager::startVNC(bool use_gpu) {
    TKDE_INFO("Starting VNC (gpu=%d)", use_gpu);

    stopVNC(true);
    runFork("termux-wake-lock");

    std::string script = "/data/data/com.termux/files/usr/bin/vncstart";
    if (!use_gpu) {
        return runScript({"/bin/bash", script, "--nogpu"}, 15) == 0;
    }
    return runScript({"/bin/bash", script}, 15) == 0;
}

bool DisplayManager::stopVNC(bool force) {
    TKDE_INFO("Stopping VNC (force=%d)", force);
    std::string script = "/data/data/com.termux/files/usr/bin/vncstop";
    if (force) {
        return runFork("pkill -9 Xvnc 2>/dev/null; pkill -9 virgl_test_server_android 2>/dev/null") == 0;
    }
    return runScript({"/bin/bash", script}) == 0;
}

bool DisplayManager::restartVNC(bool use_gpu) {
    return stopVNC(true) && startVNC(use_gpu);
}

bool DisplayManager::startTX11(bool use_gpu, bool use_dbus, bool legacy, const std::string& xstartup) {
    TKDE_INFO("Starting Termux:X11 (gpu=%d, dbus=%d, legacy=%d)", use_gpu, use_dbus, legacy);

    stopTX11(true);

    std::string script = "/data/data/com.termux/files/usr/bin/tx11start";
    std::vector<std::string> args = {"/bin/bash", script};

    if (!use_gpu) args.push_back("--nogpu");
    if (!use_dbus) args.push_back("--nodbus");
    if (legacy) args.push_back("--legacy");
    if (!xstartup.empty()) {
        args.push_back("--xstartup");
        args.push_back(xstartup);
    }

    return runScript(args, 20) == 0;
}

bool DisplayManager::stopTX11(bool force) {
    TKDE_INFO("Stopping Termux:X11 (force=%d)", force);
    if (force) {
        return runFork(
            "pkill -9 com.termux.x11 2>/dev/null; "
            "pkill -9 startplasma-x11 2>/dev/null; "
            "pkill -9 virgl_test_server_android 2>/dev/null"
        ) == 0;
    }
    std::string script = "/data/data/com.termux/files/usr/bin/tx11stop";
    return runScript({"/bin/bash", script}) == 0;
}

bool DisplayManager::restartTX11(bool use_gpu) {
    return stopTX11(true) && startTX11(use_gpu);
}

bool DisplayManager::start(const std::string& mode, bool use_gpu) {
    if (mode == "vnc") return startVNC(use_gpu);
    if (mode == "tx11" || mode == "termux_x11") return startTX11(use_gpu);
    TKDE_ERROR("Unknown display mode: %s", mode.c_str());
    return false;
}

bool DisplayManager::stop(const std::string& mode, bool force) {
    if (mode == "vnc") return stopVNC(force);
    if (mode == "tx11" || mode == "termux_x11") return stopTX11(force);
    TKDE_ERROR("Unknown display mode: %s", mode.c_str());
    return false;
}

bool DisplayManager::forwardDisplay(const std::string& ip, int port) {
    std::string script = "/data/data/com.termux/files/usr/bin/gui";
    std::string disp = ip + ":" + std::to_string(port);
    TKDE_INFO("Forwarding display to %s", disp.c_str());
    return runScript({"/bin/bash", script, "--display", disp}) == 0;
}

int DisplayManager::getGPUUtilization() const {
    std::string val = trim(readFile("/sys/kernel/gpu/gpu_busy"));
    if (val.empty()) return 0;
    char* e=nullptr; long v=strtol(val.c_str(),&e,10); return (*e==0 ? static_cast<int>(v) : 0);
}

int DisplayManager::getGPUFrequencyKHz() const {
    std::string val = trim(readFile("/sys/kernel/gpu/gpu_clock"));
    if (val.empty()) return 0;
    char* e=nullptr; long v=strtol(val.c_str(),&e,10); return (*e==0 ? static_cast<int>(v) : 0);
}

std::string DisplayManager::getGPUModel() const {
    return trim(readFile("/sys/kernel/gpu/gpu_model"));
}

bool DisplayManager::isMaliGPU() const {
    return runFork("grep -q mali /sys/class/misc/mali0/dev 2>/dev/null") == 0;
}

} // namespace tkde