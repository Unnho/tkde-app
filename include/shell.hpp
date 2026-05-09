#ifndef TKDE_SHELL_HPP
#define TKDE_SHELL_HPP

#include <string>
#include <vector>

namespace tkde {

struct ShellProfile {
    std::string name;       // zsh | bash | bash_with_ble
    std::string zsh_theme; // td_zsh | p10k_zsh | pure_zsh
    std::string font;
    bool ble_init = false;
    bool loaded = false;
};

struct NerdFont {
    std::string name;
    std::string path;
    bool installed = false;
};

class ShellManager {
    static std::string runCmd(const std::string& cmd);
    static int runFork(const std::string& cmd);

public:
    // Profiles
    std::vector<ShellProfile> listProfiles() const;
    bool setDefaultShell(const std::string& name);
    std::string getDefaultShell() const;

    // Zsh themes (from termux-desktop)
    bool installZshTheme(const std::string& theme_name);
    bool installTDZsh();    // termux-desktop custom zsh theme
    bool installPureZsh();  // pure prompt
    bool installP10k();     // powerlevel10k
    bool installBle();      // bash-like emulator for zsh

    // Nerd Fonts
    std::vector<NerdFont> listNerdFonts() const;
    bool installNerdFont(const std::string& font_name); // 0xProto | FiraCode | JetBrainsMono | etc
    bool installAllNerdFonts();

    // Shell configuration
    bool configureZsh(const ShellProfile& profile);
    bool configureBash();
    bool installShellCompletions();
    bool setupNeovim(const std::string& distro = "nvchad"); // stock | nvchad | lazyvim | mynvim

    // Terminal utilities (from termux-desktop "terminal utility")
    bool installTerminalUtilities();
    bool installFMEnhancements();
};

} // namespace tkde
#endif // TKDE_SHELL_HPP