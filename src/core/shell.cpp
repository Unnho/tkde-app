#include "shell.hpp"
#include "logger.hpp"
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
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

static int runFork(const std::string& cmd) {
    return WEXITSTATUS(pclose(popen(cmd.c_str(), "r")));
}

} // anonymous

namespace tkde {

std::string ShellManager::runCmd(const std::string& cmd) { return runOut(cmd); }
int ShellManager::runFork(const std::string& cmd) { return ::runFork(cmd); }

std::vector<ShellProfile> ShellManager::listProfiles() const {
    std::vector<ShellProfile> profiles;

    ShellProfile zsh;
    zsh.name = "zsh";
    zsh.ble_init = false;
    zsh.loaded = access("/data/data/com.termux/files/usr/bin/zsh", X_OK) == 0;
    zsh.font = "0xProto";
    profiles.push_back(zsh);

    ShellProfile bash;
    bash.name = "bash";
    bash.loaded = access("/data/data/com.termux/files/usr/bin/bash", X_OK) == 0;
    bash.font = "FiraCode";
    profiles.push_back(bash);

    ShellProfile bash_ble;
    bash_ble.name = "bash_with_ble";
    bash_ble.loaded = bash.loaded;
    bash_ble.ble_init = access("/data/data/com.termux/files/home/.bash_ble/init.sh", R_OK) == 0;
    profiles.push_back(bash_ble);

    return profiles;
}

bool ShellManager::setDefaultShell(const std::string& name) {
    TKDE_INFO("Setting default shell to: %s", name.c_str());
    if (name == "zsh") {
        return runFork(
            "chsh -s /data/data/com.termux/files/usr/bin/zsh 2>/dev/null") == 0
            || runFork(
                "echo '/data/data/com.termux/files/usr/bin/zsh' >> /data/data/com.termux/files/home/.shell 2>/dev/null") == 0;
    } else if (name == "bash") {
        return runFork("chsh -s /data/data/com.termux/files/usr/bin/bash 2>/dev/null") == 0;
    }
    return false;
}

std::string ShellManager::getDefaultShell() const {
    std::string cur = runOut("getent passwd $(whoami) 2>/dev/null | cut -d: -f7");
    if (cur.find("zsh") != std::string::npos) return "zsh";
    if (cur.find("bash") != std::string::npos) return "bash";
    return "unknown";
}

bool ShellManager::installZshTheme(const std::string& theme_name) {
    TKDE_INFO("Installing zsh theme: %s", theme_name.c_str());
    std::string theme_path = "/data/data/com.termux/files/home/.zsh-themes";

    if (theme_name == "td_zsh") {
        // termux-desktop custom zsh theme
        return runFork(
            "curl -fsSL "
            "https://raw.githubusercontent.com/sabamdarif/termux-desktop/"
            "main/setup-files/look_1/font.tar.gz "
            "-o /tmp/td_zsh_theme.tar.gz 2>/dev/null && "
            "mkdir -p " + theme_path + " && "
            "tar xzf /tmp/td_zsh_theme.tar.gz -C " + theme_path + " 2>/dev/null") == 0;
    } else if (theme_name == "p10k_zsh") {
        return runFork(
            "git clone --depth 1 https://github.com/romkatv/powerlevel10k.git "
            "/data/data/com.termux/files/home/.oh-my-zsh/custom/themes/powerlevel10k 2>/dev/null") == 0;
    } else if (theme_name == "pure_zsh") {
        return runFork(
            "mkdir -p /data/data/com.termux/files/home/.zsh/pure && "
            "curl -fsSL https://raw.githubusercontent.com/sindresorhus/pure/main/pure.zsh "
            "-o /data/data/com.termux/files/home/.zsh/pure/pure.zsh 2>/dev/null && "
            "curl -fsSL https://raw.githubusercontent.com/sindresorhus/pure/main/async.zsh "
            "-o /data/data/com.termux/files/home/.zsh/pure/async.zsh 2>/dev/null") == 0;
    }
    return false;
}

bool ShellManager::installTDZsh() { return installZshTheme("td_zsh"); }
bool ShellManager::installPureZsh() { return installZshTheme("pure_zsh"); }
bool ShellManager::installP10k() { return installZshTheme("p10k_zsh"); }

bool ShellManager::installBle() {
    TKDE_INFO("Installing ble.sh (bash-like emulator for zsh)");
    return runFork(
        "curl -fsSL https://github.com/akinomyoga/ble.sh/releases/download/v0.4.0/ble-0.4.0.tar.xz "
        "-o /tmp/ble.tar.xz 2>/dev/null && "
        "mkdir -p /data/data/com.termux/files/home/.bash_ble && "
        "tar xJf /tmp/ble.tar.xz -C /data/data/com.termux/files/home/.bash_ble --strip-components=1 2>/dev/null && "
        "echo 'source /data/data/com.termux/files/home/.bash_ble/init.sh' >> /data/data/com.termux/files/home/.bashrc 2>/dev/null") == 0;
}

std::vector<NerdFont> ShellManager::listNerdFonts() const {
    std::vector<NerdFont> fonts;
    const char* names[] = { "0xProto", "FiraCode", "JetBrainsMono", "Hack", "SourceCodePro" };
    for (auto n : names) {
        NerdFont f;
        f.name = n;
        f.installed = access(
            ("/data/data/com.termux/files/home/.fonts/" + std::string(n) + ".ttf").c_str(), R_OK) == 0;
        fonts.push_back(f);
    }
    return fonts;
}

bool ShellManager::installNerdFont(const std::string& font_name) {
    TKDE_INFO("Installing Nerd Font: %s", font_name.c_str());
    std::string url;
    if (font_name == "0xProto") {
        url = "https://github.com/alias-ahmed/0xProto/releases/download/v0.1.3/0xProto.zip";
    } else if (font_name == "FiraCode") {
        url = "https://github.com/ryanoasis/nerd-fonts/releases/download/v3.0.2/FiraCode.zip";
    } else if (font_name == "JetBrainsMono") {
        url = "https://github.com/ryanoasis/nerd-fonts/releases/download/v3.0.2/JetBrainsMono.zip";
    } else if (font_name == "Hack") {
        url = "https://github.com/ryanoasis/nerd-fonts/releases/download/v3.0.2/Hack.zip";
    } else {
        return false;
    }

    std::string install_cmd =
        "mkdir -p /data/data/com.termux/files/home/.fonts && "
        "curl -fsSL " + url + " -o /tmp/" + font_name + ".zip && "
        "unzip -o /tmp/" + font_name + ".zip -d /data/data/com.termux/files/home/.fonts/ 2>/dev/null && "
        "fc-cache -f 2>/dev/null && rm -f /tmp/" + font_name + ".zip";
    return runFork(install_cmd) == 0;
}

bool ShellManager::installAllNerdFonts() {
    return installNerdFont("0xProto") && installNerdFont("FiraCode");
}

bool ShellManager::configureZsh(const ShellProfile& profile) {
    TKDE_INFO("Configuring zsh with theme: %s", profile.zsh_theme.c_str());

    // Copy termux-desktop .zshenv and .zshrc
    std::string src_env = "/data/data/com.termux/files/home/tkde/termux-desktop/other/.zshenv";
    std::string src_rc = "/data/data/com.termux/files/home/tkde/termux-desktop/other/.zshrc";

    if (access(src_env.c_str(), R_OK) == 0) {
        runFork("cp " + src_env + " /data/data/com.termux/files/home/.zshenv 2>/dev/null");
    }
    if (access(src_rc.c_str(), R_OK) == 0) {
        runFork("cp " + src_rc + " /data/data/com.termux/files/home/.zshrc 2>/dev/null");
    }

    if (!profile.zsh_theme.empty()) {
        installZshTheme(profile.zsh_theme);
    }

    // Set up oh-my-zsh if not present
    if (access("/data/data/com.termux/files/home/.oh-my-zsh", R_OK) != 0) {
        runFork(
            "curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh "
            "| sh -s -- --keep-zshrc 2>/dev/null");
    }

    // Install zsh-completions
    runFork("mkdir -p /data/data/com.termux/files/home/.zsh/completions && "
            "cp /data/data/com.termux/files/home/tkde/termux-desktop/completion/zsh/_* "
            "/data/data/com.termux/files/home/.zsh/completions/ 2>/dev/null");

    TKDE_INFO("Zsh configured successfully");
    return true;
}

bool ShellManager::configureBash() {
    TKDE_INFO("Configuring bash");

    std::string src = "/data/data/com.termux/files/home/tkde/termux-desktop/other/setup-bash";
    if (access(src.c_str(), R_OK) == 0) {
        return runFork("bash " + src + " 2>/dev/null") == 0;
    }
    return false;
}

bool ShellManager::installShellCompletions() {
    TKDE_INFO("Installing shell completions");
    std::string src = "/data/data/com.termux/files/home/tkde/termux-desktop/completion";

    if (access(src.c_str(), R_OK) == 0) {
        runFork("cp " + src + "/bash/* /data/data/com.termux/files/home/.termux/completion/ 2>/dev/null");
        runFork("cp " + src + "/zsh/* /data/data/com.termux/files/home/.zsh/completions/ 2>/dev/null");
        TKDE_INFO("Completions installed");
        return true;
    }
    return false;
}

bool ShellManager::setupNeovim(const std::string& distro) {
    TKDE_INFO("Setting up Neovim: %s", distro.c_str());

    // Install neovim if not present
    runFork("pkg install neovim 2>/dev/null");

    if (distro == "nvchad") {
        return runFork(
            "mv /data/data/com.termux/files/home/.config/nvim /data/data/com.termux/files/home/.config/nvim.bak 2>/dev/null; "
            "git clone --depth 1 https://github.com/NvChad/NvChad /data/data/com.termux/files/home/.config/nvim 2>/dev/null && "
            "cp /data/data/com.termux/files/home/.config/nvim/config/nvchad_init.example.lua "
            "/data/data/com.termux/files/home/.config/nvim/init.lua 2>/dev/null") == 0;
    } else if (distro == "lazyvim") {
        return runFork(
            "mv /data/data/com.termux/files/home/.config/nvim /data/data/com.termux/files/home/.config/nvim.bak 2>/dev/null; "
            "git clone --depth 1 https://github.com/LazyVim/starter "
            "/data/data/com.termux/files/home/.config/nvim 2>/dev/null") == 0;
    } else if (distro == "mynvim") {
        // termux-desktop custom nvim config
        std::string src = "/data/data/com.termux/files/home/tkde/termux-desktop/other/config.jsonc";
        if (access(src.c_str(), R_OK) == 0) {
            runFork("mkdir -p /data/data/com.termux/files/home/.config/nvim && "
                    "cp " + src + " /data/data/com.termux/files/home/.config/nvim/init.lua 2>/dev/null");
            return true;
        }
    }
    // Stock neovim - just install it
    return runFork("pkg install neovim 2>/dev/null") == 0;
}

bool ShellManager::installTerminalUtilities() {
    TKDE_INFO("Installing terminal utilities");
    return runFork(
        "pkg install neofetch htop bmon bashtop neovim nano vim git "
        "termux-api 2>/dev/null") == 0;
}

bool ShellManager::installFMEnhancements() {
    TKDE_INFO("Installing file manager enhancements");
    return runFork(
        "pkg install ffmpeg imagemagick poppler fd trash-cli 2>/dev/null") == 0;
}

} // namespace tkde