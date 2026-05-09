#include "config.hpp"
#include "logger.hpp"
#include <algorithm>
#include <cstdlib>
#include <cerrno>
#include <fstream>
#include <sstream>

namespace {

const char* DEFAULT_CONFIG = R"(
# Termux Desktop KDE App Configuration
# Auto-generated - do not edit manually

# GPU Configuration
GPU_NAME=mali
enable_hw_acc=y

# Display Configuration
gui_mode=both
display_number=0

# Desktop Environment
de_name=plasma
de_startup=startplasma-x11

# Termux-Desktop paths
TERMUX_DESKTOP_PATH=@TERMUX_PREFIX@/etc/termux-desktop
)";

} // anonymous namespace

namespace tkde {

ConfigManager::ConfigManager()
    : configPath_(getenv("HOME") ? fs::path(getenv("HOME")) / ".config" / "tkde-app" / "config.conf" : "/data/data/com.termux/files/home/.config/tkde-app/config.conf")
    , cacheDir_(getenv("HOME") ? fs::path(getenv("HOME")) / ".cache" / "tkde-app" : "/data/data/com.termux/files/home/.cache/tkde-app")
{
    // Set defaults
    gpu_.name = "mali";
    gpu_.hw_answer = "virgl";
    gpu_.vulkan_driver = "vulkan_wrapper_android";
    gpu_.pd_hw_answer = "virgl";
    gpu_.enabled = true;

    display_.mode = "both";
    display_.display_number = 0;
    display_.autostart = false;
    display_.default_gui_mode = "termux_x11";
    display_.de_on_startup = false;

    de_.name = "plasma";
    de_.startup = "startplasma-x11";
    de_.themes_folder = getenv("HOME") ? fs::path(getenv("HOME")) / ".themes" : "/data/data/com.termux/files/home/.themes";
    de_.icons_folder = getenv("HOME") ? fs::path(getenv("HOME")) / ".icons" : "/data/data/com.termux/files/home/.icons";
    de_.style_number = 0;
    de_.style_name = "Stock";

    distro_.type = "proot";
    distro_.name = "ubuntu";
    distro_.audio_config = false;
    distro_.user_add = false;
    distro_.user_name = "ahaan";
    distro_.pass = "root";

    app_.shell = "zsh";
    app_.zsh_theme = "pure_zsh";
    app_.nerd_font = "0xProto";
    app_.installed_browser = "firefox";
    app_.installed_ide = "neovim";
    app_.nvim_distro = "nvchad";
    app_.installed_media_player = "vlc";
    app_.installed_photo_editor = "skip";
    app_.installed_wine = "skip";
    app_.terminal_utility = false;
    app_.fm_tools = false;
}

bool ConfigManager::load() {
    std::lock_guard<std::mutex> l(mutex_);

    // Create config dir if missing
    if (!fs::exists(configPath_.parent_path())) {
        fs::create_directories(configPath_.parent_path());
    }

    if (!fs::exists(configPath_)) {
        TKDE_WARN("Config file not found at %s, using defaults", configPath_.c_str());
        return false;
    }

    std::ifstream f(configPath_);
    if (!f) {
        TKDE_ERROR("Failed to open config: %s", configPath_.c_str());
        return false;
    }

    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        if (key.empty()) continue;

        values_[key] = val;

        // Parse into sections
        if (key == "GPU_NAME") gpu_.name = val;
        else if (key == "enable_hw_acc") gpu_.enabled = (val == "y");
        else if (key == "termux_hw_answer") gpu_.hw_answer = val;
        else if (key == "exp_termux_vulkan_hw_answer") gpu_.vulkan_driver = val;
        else if (key == "pd_hw_answer") gpu_.pd_hw_answer = val;
        else if (key == "gui_mode") display_.mode = val;
        else if (key == "display_number") display_.display_number = static_cast<int>(strtol(val.c_str(), nullptr, 10));
        else if (key == "de_on_startup") display_.de_on_startup = (val == "y");
        else if (key == "default_gui_mode") display_.default_gui_mode = val;
        else if (key == "de_name") de_.name = val;
        else if (key == "de_startup") de_.startup = val;
        else if (key == "themes_folder") de_.themes_folder = val;
        else if (key == "icons_folder") de_.icons_folder = val;
        else if (key == "style_answer") de_.style_number = static_cast<int>(strtol(val.c_str(), nullptr, 10));
        else if (key == "selected_distro") distro_.name = val;
        else if (key == "selected_distro_type") distro_.type = val;
        else if (key == "pd_audio_config_answer") distro_.audio_config = (val == "y");
        else if (key == "pd_useradd_answer") distro_.user_add = (val == "y");
        else if (key == "user_name") distro_.user_name = val;
        else if (key == "chosen_shell_name") app_.shell = val;
        else if (key == "selected_zsh_theme_name") app_.zsh_theme = val;
        else if (key == "installed_browser") app_.installed_browser = val;
        else if (key == "installed_ide") app_.installed_ide = val;
        else if (key == "nvim_distro") app_.nvim_distro = val;
        else if (key == "installed_media_player") app_.installed_media_player = val;
        else if (key == "terminal_utility_setup_answer") app_.terminal_utility = (val == "y");
        else if (key == "fm_tools") app_.fm_tools = (val == "y");
    }

    TKDE_INFO("Config loaded from %s (%zu keys)", configPath_.c_str(), values_.size());
    return true;
}

bool ConfigManager::save() {
    std::lock_guard<std::mutex> l(mutex_);

    fs::create_directories(configPath_.parent_path());

    std::ofstream f(configPath_);
    if (!f) {
        TKDE_ERROR("Failed to open config for writing: %s", configPath_.c_str());
        return false;
    }

    f << "# Termux Desktop KDE App Configuration\n";
    f << "# Auto-generated by tkde-app\n\n";

    f << "# GPU\n";
    f << "GPU_NAME=" << gpu_.name << "\n";
    f << "enable_hw_acc=" << (gpu_.enabled ? "y" : "n") << "\n";
    f << "termux_hw_answer=" << gpu_.hw_answer << "\n";
    f << "exp_termux_vulkan_hw_answer=" << gpu_.vulkan_driver << "\n";
    f << "pd_hw_answer=" << gpu_.pd_hw_answer << "\n\n";

    f << "# Display\n";
    f << "gui_mode=" << display_.mode << "\n";
    f << "display_number=" << display_.display_number << "\n";
    f << "de_on_startup=" << (display_.de_on_startup ? "y" : "n") << "\n";
    f << "default_gui_mode=" << display_.default_gui_mode << "\n\n";

    f << "# Desktop Environment\n";
    f << "de_name=" << de_.name << "\n";
    f << "de_startup=" << de_.startup << "\n";
    f << "themes_folder=" << de_.themes_folder << "\n";
    f << "icons_folder=" << de_.icons_folder << "\n";
    f << "style_answer=" << de_.style_number << "\n\n";

    f << "# Distro\n";
    f << "selected_distro=" << distro_.name << "\n";
    f << "selected_distro_type=" << distro_.type << "\n";
    f << "pd_audio_config_answer=" << (distro_.audio_config ? "y" : "n") << "\n";
    f << "pd_useradd_answer=" << (distro_.user_add ? "y" : "n") << "\n";
    f << "user_name=" << distro_.user_name << "\n\n";

    f << "# App Settings\n";
    f << "chosen_shell_name=" << app_.shell << "\n";
    f << "selected_zsh_theme_name=" << app_.zsh_theme << "\n";
    f << "installed_browser=" << app_.installed_browser << "\n";
    f << "installed_ide=" << app_.installed_ide << "\n";
    f << "nvim_distro=" << app_.nvim_distro << "\n";
    f << "installed_media_player=" << app_.installed_media_player << "\n";
    f << "terminal_utility_setup_answer=" << (app_.terminal_utility ? "y" : "n") << "\n";
    f << "fm_tools=" << (app_.fm_tools ? "y" : "n") << "\n";

    f.flush();
    TKDE_INFO("Config saved to %s", configPath_.c_str());
    return f.good();
}

std::optional<std::string> ConfigManager::get(const std::string& key) const {
    auto it = values_.find(key);
    if (it != values_.end()) return it->second;
    return std::nullopt;
}

void ConfigManager::set(const std::string& key, const std::string& val) {
    std::lock_guard<std::mutex> l(mutex_);
    values_[key] = val;
}

int ConfigManager::getInt(const std::string& key, int fallback) const {
    auto v = get(key);
    if (!v) return fallback;
    if (v->empty()) return fallback;
    char* end = nullptr;
    long val = std::strtol(v->c_str(), &end, 10);
    return (*end == '\0' && errno == 0) ? static_cast<int>(val) : fallback;
}

bool ConfigManager::getBool(const std::string& key, bool fallback) const {
    auto v = get(key);
    if (!v) return fallback;
    return *v == "y" || *v == "Y" || *v == "true" || *v == "1";
}
} // namespace tkde
