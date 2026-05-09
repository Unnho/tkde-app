#ifndef TKDE_APPSTORE_HPP
#define TKDE_APPSTORE_HPP

#include <map>
#include <string>
#include <vector>

namespace tkde {

enum class AppCategory { INTERNET, MULTIMEDIA, DEVELOPMENT, OFFICE, UTILITY, SYSTEM };

struct DesktopApp {
    std::string id;
    std::string name;
    std::string description;
    std::string exec;
    std::string icon;
    AppCategory category;
    bool in_termux = true;   // true = native termux, false = proot-distro
    std::string distro;      // if not in_termux, which distro
    std::string desktop_file;
    bool installed = false;
};

struct WineApp {
    std::string name;
    std::string exe_path;
    std::string wine_prefix;
};

class AppStore {
    static std::string runCmd(const std::string& cmd);
    static int runFork(const std::string& cmd);

    std::vector<DesktopApp> catalog_;
    void buildCatalog();

public:
    AppStore();

    // Browse/search
    std::vector<DesktopApp> getApps(AppCategory cat = AppCategory::INTERNET) const;
    std::vector<DesktopApp> search(const std::string& query) const;
    DesktopApp getApp(const std::string& id) const;

    // Install/remove
    bool installApp(const std::string& id);
    bool removeApp(const std::string& id);

    // proot-distro apps (add2menu integration)
    std::vector<DesktopApp> getDistroApps(const std::string& distro) const;
    bool addDistroAppToMenu(const std::string& distro, const std::string& desktop_file);
    bool removeDistroAppFromMenu(const std::string& app_id);

    // Wine support
    bool installWine(const std::string& variant = "wine_hangover"); // stock | mobox | wine_hangover
    bool installWindowsApp(const std::string& exe_path, const std::string& name);
    std::vector<WineApp> listWineApps() const;

    // Update
    bool updateAppIndex();

    // Menu integration
    bool launchApp(const std::string& id);
};

} // namespace tkde
#endif // TKDE_APPSTORE_HPP