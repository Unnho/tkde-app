#include "gpu.hpp"
#include "logger.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

namespace {

static std::string readFile(const std::string& p) {
    std::ifstream f(p);
    if (!f) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
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

static std::string runOut(const std::string& cmd) {
    char buf[4096];
    std::string r;
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return "";
    while (fgets(buf, sizeof(buf), p)) r += buf;
    pclose(p);
    return trim(r);
}

} // anonymous

namespace tkde {

bool GPUMonitor::init() {
    if (!std::ifstream(gpuPath_ + "/gpu_model").good()) {
        TKDE_WARN("GPU sysfs not found at %s", gpuPath_.c_str());
        return false;
    }
    initialized_ = true;
    TKDE_INFO("GPU monitor initialized: %s", probeGPUModel().c_str());
    return true;
}

GPUInfo GPUMonitor::probe() const {
    GPUInfo i;
    i.model = probeGPUModel();
    i.arch = probeGPUArch();
    i.cores = probeCoreCount();
    i.max_freq_khz = probeMaxFreqKHz();
    i.cur_freq_khz = probeCurFreqKHz();
    i.utilization = probeUtilization();
    i.vulkan_available = probeVulkanSupport();
    i.virgl_available = probeVirglSupport();
    i.drm_available = probeDRMSupport();
    i.available_drivers = listDrivers(i.arch);
    i.hw_accel_available = (i.drm_available || i.virgl_available);
    return i;
}

std::string GPUMonitor::probeGPUModel() const {
    return trim(readFile(gpuPath_ + "/gpu_model"));
}

std::string GPUMonitor::probeGPUArch() const {
    if (!trim(readFile(maliDevPath_ + "/gpuinfo")).empty())
        return "mali";
    if (access("/sys/class/misc/adreno0", F_OK) == 0)
        return "adreno";
    if (access("/sys/class/misc/xclipse0", F_OK) == 0)
        return "xclipse";
    return "unknown";
}

int GPUMonitor::probeCoreCount() const {
    std::string info = trim(readFile(maliDevPath_ + "/gpuinfo"));
    size_t pos = info.rfind(' ');
    if (pos != std::string::npos) {
        std::string num = trim(info.substr(pos));
        char* e = nullptr;
        long v = strtol(num.c_str(), &e, 10);
        if (*e == '\0') return static_cast<int>(v);
    }
    return 0;
}

int GPUMonitor::probeMaxFreqKHz() const {
    std::string t = trim(readFile(gpuPath_ + "/gpu_max_clock"));
    if (t.empty()) return 0;
    char* e = nullptr;
    long v = strtol(t.c_str(), &e, 10);
    if (*e == '\0') return static_cast<int>(v);
    return 0;
}

int GPUMonitor::probeCurFreqKHz() const {
    std::string t = trim(readFile(gpuPath_ + "/gpu_clock"));
    if (t.empty()) return 0;
    char* e = nullptr;
    long v = strtol(t.c_str(), &e, 10);
    if (*e == '\0') return static_cast<int>(v);
    return 0;
}

int GPUMonitor::probeUtilization() const {
    std::string t = trim(readFile(gpuPath_ + "/gpu_busy"));
    if (t.empty()) return 0;
    char* e = nullptr;
    long v = strtol(t.c_str(), &e, 10);
    if (*e == '\0') return static_cast<int>(v);
    return 0;
}

bool GPUMonitor::probeVulkanSupport() const {
    return !readFile("/data/data/com.termux/files/usr/share/vulkan/icd.d/wrapper_icd.aarch64.json").empty();
}

bool GPUMonitor::probeVirglSupport() const {
    return access("/data/data/com.termux/files/usr/bin/virgl_test_server_android", X_OK) == 0;
}

bool GPUMonitor::probeDRMSupport() const {
    return access("/dev/dri/", F_OK) == 0;
}

std::vector<GPUDriver> GPUMonitor::listDrivers(const std::string& arch) const {
    std::vector<GPUDriver> drivers;

    if (probeVirglSupport()) {
        drivers.push_back({"virgl", "virgl",
            "GALLIUM_DRIVER=virpipe MESA_GL_VERSION_OVERRIDE=4.3COMPAT "
            "MESA_GLES_VERSION_OVERRIDE=3.2 LIBGL_DRI3_DISABLE=1 MESA_NO_ERROR=1",
            "", true});
        drivers.push_back({"virgl_angle", "virgl",
            "GALLIUM_DRIVER=virpipe MESA_GL_VERSION_OVERRIDE=4.3COMPAT "
            "MESA_GLES_VERSION_OVERRIDE=3.2",
            "--angle-gl", true});
        drivers.push_back({"virgl_vulkan", "virgl",
            "GALLIUM_DRIVER=virpipe MESA_GL_VERSION_OVERRIDE=4.1COMPAT "
            "MESA_GLES_VERSION_OVERRIDE=3.2 EPOXY_USE_ANGLE=1",
            "--angle-vulkan", true});
    }

    if (access("/data/data/com.termux/files/usr/share/vulkan/icd.d/", F_OK) == 0) {
        drivers.push_back({"zink", "zink",
            "GALLIUM_DRIVER=zink MESA_GL_VERSION_OVERRIDE=4.3COMPAT "
            "MESA_GLES_VERSION_OVERRIDE=3.2 ZINK_DESCRIPTORS=lazy",
            "--angle-vulkan", true});
        drivers.push_back({"zink_virgl", "zink",
            "GALLIUM_DRIVER=virpipe MESA_GL_VERSION_OVERRIDE=4.3COMPAT ZINK_DESCRIPTORS=lazy",
            "--use-egl-surfaceless --use-gles", true});
    }

    if (arch == "adreno") {
        drivers.push_back({"turnip", "turnip",
            "VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/freedreno_icd.aarch64.json "
            "MESA_LOADER_DRIVER_OVERRIDE=zink TU_DEBUG=noconform",
            "", true});
    }

    if (arch == "mali" && probeDRMSupport()) {
        drivers.push_back({"mali_native", "native",
            "GALLIUM_DRIVER=auto MESA_NO_ERROR=1",
            "", true});
    }

    return drivers;
}

GPUDriver GPUMonitor::detectBestDriver(const GPUInfo& info) const {
    if (info.drm_available && info.arch == "mali") {
        for (const auto& d : info.available_drivers) {
            if (d.type == "native") return d;
        }
    }
    if (info.arch == "adreno" && info.vulkan_available) {
        for (const auto& d : info.available_drivers) {
            if (d.type == "turnip") return d;
        }
    }
    for (const auto& d : info.available_drivers) {
        if (d.type == "zink") return d;
    }
    for (const auto& d : info.available_drivers) {
        if (d.type == "virgl") return d;
    }
    return {};
}

bool GPUMonitor::setDriver(const GPUDriver& driver, bool) {
    TKDE_INFO("Setting driver: %s", driver.name.c_str());
    return true;
}

bool GPUMonitor::applyVirglEnv(bool) const {
    setenv("GALLIUM_DRIVER", "virpipe", 1);
    setenv("MESA_GL_VERSION_OVERRIDE", "4.3COMPAT", 1);
    setenv("MESA_GLES_VERSION_OVERRIDE", "3.2", 1);
    setenv("LIBGL_DRI3_DISABLE", "1", 1);
    TKDE_INFO("Applied VirGL env vars");
    return true;
}

bool GPUMonitor::applyZinkEnv() const {
    setenv("GALLIUM_DRIVER", "zink", 1);
    setenv("MESA_GL_VERSION_OVERRIDE", "4.3COMPAT", 1);
    setenv("ZINK_DESCRIPTORS", "lazy", 1);
    return true;
}

bool GPUMonitor::applyTurnipEnv() const {
    setenv("VK_ICD_FILENAMES", "/usr/share/vulkan/icd.d/freedreno_icd.aarch64.json", 1);
    setenv("MESA_LOADER_DRIVER_OVERRIDE", "zink", 1);
    return true;
}

bool GPUMonitor::switchToDriver(const std::string& driver_name) {
    GPUInfo info = probe();
    for (const auto& d : info.available_drivers) {
        if (d.name == driver_name) {
            TKDE_INFO("Switching to driver: %s (%s)", d.name.c_str(), d.type.c_str());
            if (d.type == "virgl") return applyVirglEnv();
            if (d.type == "zink") return applyZinkEnv();
            if (d.type == "turnip") return applyTurnipEnv();
        }
    }
    TKDE_ERROR("Driver not found: %s", driver_name.c_str());
    return false;
}

std::map<std::string, int> GPUMonitor::getLiveStats() const {
    return {
        {"utilization", probeUtilization()},
        {"freq_khz", probeCurFreqKHz()},
        {"max_freq_khz", probeMaxFreqKHz()},
    };
}

bool GPUMonitor::isUnderLoad() const {
    return probeUtilization() > 5;
}

bool GPUMonitor::supportsOpenGLES(int version) const {
    if (version <= 2) return true;
    if (version == 3) return probeVirglSupport();
    return false;
}

bool GPUMonitor::supportsVulkan() const {
    return probeVulkanSupport();
}

std::string GPUMonitor::benchmark(float duration_sec) const {
    int before = probeUtilization();
    TKDE_INFO("GPU benchmark: %s (util before: %d%%)", probeGPUModel().c_str(), before);
    usleep(static_cast<useconds_t>(duration_sec * 1000000));
    int after = probeUtilization();
    return "Model: " + probeGPUModel() +
           " | Cores: " + std::to_string(probeCoreCount()) +
           " | Freq: " + std::to_string(probeCurFreqKHz() / 1000) + " MHz" +
           " | Util: " + std::to_string(after) + "%";
}

} // namespace tkde