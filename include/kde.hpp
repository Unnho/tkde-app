#ifndef TKDE_KDE_HPP
#define TKDE_KDE_HPP

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace tkde {

// KDE plasma component health
struct KDEResource {
    std::string name;
    int pid = 0;
    bool running = false;
    int mem_kb = 0;
    double cpu_pct = 0.0;
};

struct KDEPanel {
    std::string id;
    std::string location; // top | bottom | left | right | floating
    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;
    bool visible = true;
};

struct KDEPlasmaSession {
    bool kwin_running = false;
    bool krunner_running = false;
    bool plasma_running = false;
    bool wayland = false;
    bool x11 = true;
    std::vector<KDEResource> components;
    std::vector<KDEPanel> panels;
    std::string kde_version;
    std::string kde_session_id;
};

struct KDEConfig {
    std::string theme_name;
    std::string icon_theme;
    std::string cursor_theme;
    std::string font_dpi;
    std::string wallpaper_path;
    bool compositor_enabled = true;
    int desktop_count = 4;
    bool show_desktop_icons = true;
};

class KDEManager {
    std::string kdeConfigPath_;
    std::string kdeDataPath_;
    std::string kwriteconfig_;
    std::string kreadconfig_;
    bool sessionValid_ = false;

    static std::string runCmd(const std::string& cmd);

public:
    KDEManager();
    ~KDEManager();

    // Session management
    bool isSessionActive() const;
    bool startSession(const std::string& de_name = "plasma");
    bool stopSession(bool force = false);
    bool restartSession();
    KDEPlasmaSession querySession() const;

    // Component control
    std::vector<KDEResource> listKDEComponents() const;
    bool isComponentRunning(const std::string& name) const;
    bool restartComponent(const std::string& name);
    bool killComponent(const std::string& name);

    // KWin compositor
    bool isCompositorEnabled() const;
    bool enableCompositor();
    bool disableCompositor();
    bool setCompositorBackend(const std::string& backend); // opengl | xrender | wayland

    // Plasma config (kwriteconfig5 / kconfig)
    KDEConfig readKDEConfig() const;
    bool writeKDEConfig(const KDEConfig& cfg);
    bool setTheme(const std::string& theme_name);
    bool setWallpaper(const std::string& path);
    bool setFontDPI(int dpi);
    bool setIconTheme(const std::string& theme_name);
    bool setCursorTheme(const std::string& theme_name);
    bool setDesktopCount(int count);
    bool setPanelVisible(const std::string& panel_id, bool visible);
    bool addDesktopActivity(const std::string& name);

    // KRunner
    bool isKRunnerActive() const;
    bool triggerKRunner(const std::string& query) const;
    bool reloadKRunnerIndex();

    // System config
    bool setPowerProfile(const std::string& profile); // powersave | performance | balanced
    bool configureTouchpadSensitivity(int level); // 0-10

    // Theming
    bool applyCatppuccin(const std::string& variant = "mocha");
    bool applyNord(const std::string& variant = "nord");
    bool applyDracula();
    bool applyTokyoNight();

    // Widgets
    bool installWidget(const std::string& package_path);
    bool uninstallWidget(const std::string& widget_id);
    std::vector<std::string> listInstalledWidgets() const;

    // Events
    std::function<void(const KDEPlasmaSession&)> onSessionChange;
    std::function<void(const KDEResource&)> onComponentChange;
};

} // namespace tkde
#endif // TKDE_KDE_HPP