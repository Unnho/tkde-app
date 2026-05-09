#ifndef TKDE_CONFIG_HPP
#define TKDE_CONFIG_HPP

#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace fs = std::filesystem;

namespace tkde {

struct GPUConfig {
    std::string name;              // "mali" | "adreno" | "xclipse" | "others"
    std::string hw_answer;         // virgl | zink | virgl_angle | virgl_vulkan | zink_virgl | etc
    std::string vulkan_driver;     // vulkan_wrapper_android | mesa_freedreno | freedreno_kgsl | skip
    std::string pd_hw_answer;      // virgl | turnip | zink  (for proot-distro)
    bool enabled = true;
};

struct DisplayConfig {
    std::string mode;              // "termux_x11" | "vnc" | "both"
    int display_number = 0;
    bool autostart = false;
    std::string default_gui_mode = "termux_x11";
    bool de_on_startup = false;
};

struct DEConfig {
    std::string name;              // xfce | lxqt | plasma | mate | cinnamon | gnome | openbox | i3wm | dwm | bspwm | awesome | fluxbox | icewm | wmaker
    std::string startup;           // startxfce4 | startlxqt | startplasma-x11 | mate-session | etc
    std::string themes_folder;
    std::string icons_folder;
    int style_number = 0;
    std::string style_name = "Stock";
};

struct DistroConfig {
    std::string type;              // proot | chroot
    std::string name;              // debian | ubuntu | archlinux | fedora
    bool audio_config = false;
    bool user_add = false;
    std::string user_name;
    std::string pass = "root";
};

struct AppConfig {
    std::string shell;             // zsh | bash | bash_with_ble | termux_bash
    std::string zsh_theme;        // td_zsh | p10k_zsh | pure_zsh
    std::string nerd_font;        // font name
    std::string installed_browser; // firefox | chromium | all | skip
    std::string installed_ide;     // code | geany | neovim | all | skip
    std::string nvim_distro;       // stock | nvchad | lazyvim | mynvim
    std::string installed_media_player; // vlc | mpv | all | skip
    std::string installed_photo_editor;  // gimp | inkscape | all | skip
    std::string installed_wine;    // stock | mobox | wine_hangover | skip
    bool terminal_utility = false;
    bool fm_tools = false;
};

class ConfigManager {
    fs::path configPath_;
    fs::path cacheDir_;
    std::mutex mutex_;

    std::map<std::string, std::string> values_;
    GPUConfig gpu_;
    DisplayConfig display_;
    DEConfig de_;
    DistroConfig distro_;
    AppConfig app_;

    static std::string trim(std::string s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }

public:
    ConfigManager();
    bool load();
    bool save();

    // Raw key/value
    std::optional<std::string> get(const std::string& key) const;
    void set(const std::string& key, const std::string& val);

    // Typed getters
    int getInt(const std::string& key, int fallback = 0) const;
    bool getBool(const std::string& key, bool fallback = false) const;

    // Section accessors
    const GPUConfig& gpu() const { return gpu_; }
    const DisplayConfig& display() const { return display_; }
    const DEConfig& de() const { return de_; }
    const DistroConfig& distro() const { return distro_; }
    const AppConfig& app() const { return app_; }

    void setGPU(const GPUConfig& g) { gpu_ = g; }
    void setDisplay(const DisplayConfig& d) { display_ = d; }
    void setDE(const DEConfig& de) { de_ = de; }
    void setDistro(const DistroConfig& d) { distro_ = d; }
    void setApp(const AppConfig& a) { app_ = a; }

    fs::path configPath() const { return configPath_; }
    fs::path cacheDir() const { return cacheDir_; }
};

} // namespace tkde
#endif // TKDE_CONFIG_HPP