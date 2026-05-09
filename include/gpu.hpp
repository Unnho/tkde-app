#ifndef TKDE_GPU_HPP
#define TKDE_GPU_HPP

#include <map>
#include <string>
#include <vector>

namespace tkde {

struct GPUDriver {
    std::string name;
    std::string type;         // virgl | zink | turnip | mesa | native
    std::string env_vars;
    std::string virgl_cmd;    // virgl_test_server_android args
    bool available = false;
};

struct GPUInfo {
    std::string model;        // e.g. "Mali-G72"
    std::string arch;        // mali | adreno | xclipse | other
    std::string vendor;      // arm | qualcomm | samsung | etc
    int cores = 0;
    int max_freq_khz = 0;
    int cur_freq_khz = 0;
    int utilization = 0;    // 0-100
    bool hw_accel_available = false;
    bool drm_available = false;
    bool virgl_available = false;
    bool vulkan_available = false;
    std::vector<GPUDriver> available_drivers;
};

class GPUMonitor {
    std::string gpuPath_;
    std::string maliDevPath_;
    bool initialized_ = false;

public:
    GPUMonitor() = default;
    bool init();

    GPUInfo probe() const;
    std::string probeGPUModel() const;
    std::string probeGPUArch() const;
    int probeCoreCount() const;
    int probeMaxFreqKHz() const;
    int probeCurFreqKHz() const;
    int probeUtilization() const;
    bool probeVulkanSupport() const;
    bool probeVirglSupport() const;
    bool probeDRMSupport() const;

    // Driver management
    std::vector<GPUDriver> listDrivers(const std::string& arch) const;
    GPUDriver detectBestDriver(const GPUInfo& info) const;
    bool setDriver(const GPUDriver& driver, bool for_distro = false);
    bool applyVirglEnv(bool for_distro = false) const;
    bool applyZinkEnv() const;
    bool applyTurnipEnv() const;

    // Dynamic switching
    bool switchToDriver(const std::string& driver_name);
    bool enableMaliNative();
    bool enableVirgl(bool use_angle = false, bool use_vulkan = false);
    bool enableZink(bool via_virgl = false);
    bool enableTurnip();

    // Live stats
    std::map<std::string, int> getLiveStats() const;
    bool isUnderLoad() const;
    bool supportsOpenGLES(int version = 3) const;
    bool supportsVulkan() const;
    std::string benchmark(float duration_sec = 3.0f) const;
};

} // namespace tkde
#endif // TKDE_GPU_HPP