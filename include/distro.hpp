#ifndef TKDE_DISTRO_HPP
#define TKDE_DISTRO_HPP

#include <map>
#include <string>
#include <vector>

namespace tkde {

struct DistroInfo {
    std::string name;         // debian, ubuntu, archlinux, fedora
    std::string type;         // proot | chroot
    std::string rootfs_path;
    bool is_installed = false;
    bool is_running = false;
    int pid = 0;
    std::string version;
    size_t disk_usage_mb = 0;
};

enum class DistroType { DEBIAN, UBUNTU, ARCHLINUX, FEDORA };

class DistroManager {
    static std::string runCmd(const std::string& cmd);
    static int runFork(const std::string& cmd);

    static std::string distroHome(const std::string& name) {
        return "/data/data/com.termux/files/home/.termux-proot-distro/" + name;
    }

public:
    // Proot-distro integration
    std::vector<DistroInfo> listDistros() const;
    bool isDistroInstalled(const std::string& name) const;
    bool installDistro(const std::string& name, DistroType type = DistroType::DEBIAN);
    bool removeDistro(const std::string& name);
    bool loginDistro(const std::string& name);
    bool executeInDistro(const std::string& name, const std::string& cmd) const;
    bool stopDistro(const std::string& name);

    // Distro config (mirrors distro-container-setup script)
    bool configureDistroAudio(const std::string& name);
    bool addDistroUser(const std::string& name, const std::string& username, const std::string& password);
    bool installTurnip(const std::string& name, const std::string& mesa_version = "26.0.6");
    bool installXorgInDistro(const std::string& name);
    bool installDesktopEnv(const std::string& name, const std::string& de);
    bool runDistroUpdate(const std::string& name);

    // Disk usage
    size_t getDistroDiskUsage(const std::string& name) const;
    bool cleanupDistroCache(const std::string& name);

    // Proot args
    std::string getProotArgs(const std::string& name) const;
    bool setProotArgs(const std::string& name, const std::string& args);
};

} // namespace tkde
#endif // TKDE_DISTRO_HPP