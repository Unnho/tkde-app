#include "logger.hpp"
#include "config.hpp"
#include "display.hpp"
#include "gpu.hpp"
#include "kde.hpp"
#include "termux.hpp"
#include "distro.hpp"
#include "shell.hpp"
#include "appstore.hpp"
#include "ui.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>

namespace tkde { namespace app {

using namespace tkde::ui;

// ─── App state ───────────────────────────────────────────────────────────────

struct App {
    ConfigManager config;
    DisplayManager display;
    GPUMonitor gpu;
    KDEManager kde;
    TermuxBridge termux;
    DistroManager distro;
    ShellManager shell;
    AppStore appstore;
    Screen screen;
    bool running = false;
};

// ─── Info bar ────────────────────────────────────────────────────────────────

static void renderInfoBar(Screen& scr, App& app) {
    int W = scr.cols();
    auto batt = app.termux.getBatteryStatus();
    auto gpuStats = app.gpu.getLiveStats();
    auto status = app.display.getStatus();
    std::string gpuModel = app.gpu.probeGPUModel();

    std::ostringstream info;
    info << " Chodubot-TKDE ";
    info << " | Battery: " << batt.percentage << "%";
    if (batt.charging) info << " +";
    info << " | GPU: " << gpuModel;
    if (!gpuStats.empty()) {
        info << " (" << gpuStats.at("utilization") << "% @ "
             << (gpuStats.at("freq_khz") / 1000) << " MHz)";
    }
    info << " | VNC:" << (status.vnc_running ? "ON" : "OFF");
    info << " X11:" << (status.tx11_running ? "ON" : "OFF");
    info << " | " << (getenv("USER") ? getenv("USER") : "user") << "@termux";

    scr.drawRect({0, 0, W, 1}, BLUE);
    scr.drawText(0, 0, info.str(), WHITE, BLUE, true);
}

// ─── Main menu ────────────────────────────────────────────────────────────────

struct MainMenu {
    ListView view;
    App& app;

    MainMenu(App& a) : app(a) { rebuild(); }

    void rebuild() {
        std::vector<ListItem> items;
        items.push_back({"Display", "Start/stop VNC or Termux:X11", false, true, "display"});
        items.push_back({"GPU", "Switch GPU driver / view stats", false, true, "gpu"});
        items.push_back({"KDE Plasma", "Session, themes, compositor", false, true, "kde"});
        items.push_back({"App Store", "Install apps from catalog", false, true, "appstore"});
        items.push_back({"Distro", "Manage proot-distro containers", false, true, "distro"});
        items.push_back({"Shell", "Zsh themes, Nerd fonts, Neovim", false, true, "shell"});
        items.push_back({"Termux Utils", "Wake lock, clipboard, notifications", false, true, "termux"});
        items.push_back({"Configuration", "Edit config, save/load profiles", false, true, "config"});
        items.push_back({"System", "Device info, battery, storage", false, true, "system"});

        view.frame = {2, 2, app.screen.cols() - 4, app.screen.rows() - 4};
        view.title = "Main Menu";
        view.items = items;
        view.border = true;
    }
};

// ─── Display view ────────────────────────────────────────────────────────────

struct DisplayView {
    ListView view;
    App& app;

    DisplayView(App& a) : app(a) { refresh(); }

    void refresh() {
        auto status = app.display.getStatus();
        view.items.clear();
        view.items.push_back({"Start VNC (GPU)", "TigerVNC with hardware acceleration",
            !status.vnc_running, true, "start_vnc_gpu"});
        view.items.push_back({"Start VNC (CPU)", "TigerVNC software rendering",
            !status.vnc_running, true, "start_vnc_nogpu"});
        view.items.push_back({"Stop VNC", "Stop VNC server",
            status.vnc_running, true, "stop_vnc"});
        view.items.push_back({"Restart VNC", "Restart VNC session",
            status.vnc_running, true, "restart_vnc"});
        view.items.push_back({"Start Termux:X11", "Launch via Termux:X11",
            !status.tx11_running, true, "start_tx11"});
        view.items.push_back({"Stop Termux:X11", "Stop Termux:X11",
            status.tx11_running, true, "stop_tx11"});
        view.items.push_back({"Forward Display", "Forward session to remote IP",
            true, true, "forward_display"});

        view.frame = {2, 2, app.screen.cols() - 4, app.screen.rows() - 4};
        view.title = "Display — VNC:" + std::string(status.vnc_running ? "RUNNING" : "STOPPED")
                   + " X11:" + std::string(status.tx11_running ? "RUNNING" : "STOPPED");
        view.border = true;
    }
};

// ─── GPU view ───────────────────────────────────────────────────────────────

struct GPUView {
    ListView view;
    App& app;

    GPUView(App& a) : app(a) { refresh(); }

    void refresh() {
        auto info = app.gpu.probe();
        view.items.clear();
        view.items.push_back({"GPU Model", info.model + " (" + std::to_string(info.cores) + " cores)",
            false, true, "header"});
        view.items.push_back({"Architecture", info.arch, false, true, "arch"});
        view.items.push_back({"Frequency",
            std::to_string(info.cur_freq_khz / 1000) + " / " + std::to_string(info.max_freq_khz / 1000) + " MHz",
            false, true, "freq"});
        view.items.push_back({"Utilization", std::to_string(info.utilization) + "%",
            false, true, "util"});
        view.items.push_back({"DRM Support", info.drm_available ? "Yes" : "No (software only)",
            false, true, "drm"});
        view.items.push_back({"VirGL", info.virgl_available ? "Available" : "N/A",
            false, true, "virgl"});
        view.items.push_back({"Vulkan", info.vulkan_available ? "Available" : "N/A",
            false, true, "vulkan"});
        view.items.push_back({""});
        view.items.push_back({"Available Drivers:", "", false, false, ""});
        for (const auto& d : info.available_drivers) {
            view.items.push_back({d.name, d.type + " — " + std::string(d.available ? "available" : "unavailable"),
                false, d.available, d.name});
        }
        view.items.push_back({""});
        view.items.push_back({"Switch to VirGL", "Best for Mali GPUs",
            true, true, "switch_virgl"});
        view.items.push_back({"Switch to Zink", "OpenGL over Vulkan",
            true, true, "switch_zink"});
        view.items.push_back({"Benchmark GPU", "Quick GPU load test",
            true, true, "benchmark"});

        view.frame = {2, 2, app.screen.cols() - 4, app.screen.rows() - 4};
        view.title = "GPU Monitor — " + info.model;
        view.border = true;
    }
};

// ─── AppStore view ───────────────────────────────────────────────────────────

struct AppStoreView {
    ListView view;
    std::vector<ListItem> allItems;
    App& app;
    std::string filterQuery;

    AppStoreView(App& a) : app(a) { rebuild(); }

    void rebuild() {
        allItems.clear();
        allItems.push_back({"", "== Internet ==", false, false, ""});
        for (auto& a : app.appstore.getApps(AppCategory::INTERNET)) {
            allItems.push_back({a.name, a.description + " [" + std::string(a.installed ? "installed" : "not installed") + "]",
                !a.installed, a.installed, a.id});
        }
        allItems.push_back({"", "== Multimedia ==", false, false, ""});
        for (auto& a : app.appstore.getApps(AppCategory::MULTIMEDIA)) {
            allItems.push_back({a.name, a.description + " [" + std::string(a.installed ? "installed" : "not installed") + "]",
                !a.installed, a.installed, a.id});
        }
        allItems.push_back({"", "== Development ==", false, false, ""});
        for (auto& a : app.appstore.getApps(AppCategory::DEVELOPMENT)) {
            allItems.push_back({a.name, a.description + " [" + std::string(a.installed ? "installed" : "not installed") + "]",
                !a.installed, a.installed, a.id});
        }
        allItems.push_back({"", "== Utility ==", false, false, ""});
        for (auto& a : app.appstore.getApps(AppCategory::UTILITY)) {
            allItems.push_back({a.name, a.description + " [" + std::string(a.installed ? "installed" : "not installed") + "]",
                !a.installed, a.installed, a.id});
        }
        allItems.push_back({"", "== System ==", false, false, ""});
        for (auto& a : app.appstore.getApps(AppCategory::SYSTEM)) {
            allItems.push_back({a.name, a.description + " [" + std::string(a.installed ? "installed" : "not installed") + "]",
                !a.installed, a.installed, a.id});
        }
        allItems.push_back({"", "== Wine ==", false, false, ""});
        allItems.push_back({"Install Wine (stock)", "Stable Wine for Linux apps", true, true, "wine_stock"});
        allItems.push_back({"Install Wine (Hangover)", "Run Windows .exe files", true, true, "wine_hangover"});

        view.items.clear();
        for (auto& item : allItems) {
            if (!filterQuery.empty()) {
                std::string l = item.label;
                std::string q = filterQuery;
                std::transform(l.begin(), l.end(), l.begin(), ::tolower);
                std::transform(q.begin(), q.end(), q.begin(), ::tolower);
                if (l.find(q) == std::string::npos) continue;
            }
            view.items.push_back(item);
        }
        view.frame = {2, 2, app.screen.cols() - 4, app.screen.rows() - 4};
        view.title = "App Store" + (filterQuery.empty() ? "" : " [filter: " + filterQuery + "]");
        view.border = true;
    }
};

// ─── Main loop ────────────────────────────────────────────────────────────────

static void drawSep(Screen& scr, int y) {
    scr.drawHLine(0, y, scr.cols(), WHITE);
}

static int runMain(App& app) {
    int view = 0;
    MainMenu mainMenu(app);
    DisplayView dispView(app);
    GPUView gpuView(app);
    AppStoreView appStoreView(app);

    while (app.running) {
        app.screen.refresh();
        app.screen.clear();
        int W = app.screen.cols();
        int H = app.screen.rows();

        renderInfoBar(app.screen, app);
        drawSep(app.screen, 1);

        if (view == 0) {
            app.screen.renderListView(mainMenu.view);
            app.screen.drawText(0, H - 1, "  [j/k] Navigate  [Enter] Select  [q] Quit", BRIGHT_WHITE, DEFAULT);
        }
        else if (view == 1) {
            app.screen.renderListView(dispView.view);
            app.screen.drawText(0, H - 1, "  [j/k] Navigate  [Enter] Select  [r] Refresh  [q] Back", BRIGHT_WHITE, DEFAULT);
        }
        else if (view == 2) {
            app.screen.renderListView(gpuView.view);
            app.screen.drawText(0, H - 1, "  [j/k] Navigate  [Enter] Select  [r] Refresh  [q] Back", BRIGHT_WHITE, DEFAULT);
        }
        else if (view == 3) {
            app.screen.renderListView(appStoreView.view);
            app.screen.drawText(0, H - 1, "  [j/k] Navigate  [Enter] Install  [r] Refresh  [q] Back", BRIGHT_WHITE, DEFAULT);
        }

        app.screen.drawText(W - 20, H - 1, " tkde-app v2.1 ", BRIGHT_BLACK, DEFAULT);

        if (!app.screen.kbhit()) {
            usleep(50000);
            continue;
        }

        int ch = app.screen.getch_noblock();
        if (ch < 0) {
            usleep(50000);
            continue;
        }

        if (view == 0) {
            app.screen.handleInput(ch, mainMenu.view);
            if (ch == 'q') break;
            int sel = mainMenu.view.selected;
            if (ch == '\n' || ch == 13) {
                if (sel == 0) view = 1;
                else if (sel == 1) view = 2;
                else if (sel == 3) view = 3;
            }
        }
        else if (view == 1) {
            app.screen.handleInput(ch, dispView.view);
            if (ch == 'q') view = 0;
            if (ch == 'r') dispView.refresh();
            if (ch == '\n' || ch == 13) {
                auto& item = dispView.view.items[dispView.view.selected];
                auto tag = item.tag;
                if (tag == "start_vnc_gpu") app.display.startVNC(true);
                else if (tag == "start_vnc_nogpu") app.display.startVNC(false);
                else if (tag == "stop_vnc") app.display.stopVNC(false);
                else if (tag == "restart_vnc") app.display.restartVNC(true);
                else if (tag == "start_tx11") app.display.startTX11(true);
                else if (tag == "stop_tx11") app.display.stopTX11(false);
                dispView.refresh();
            }
        }
        else if (view == 2) {
            app.screen.handleInput(ch, gpuView.view);
            if (ch == 'q') view = 0;
            if (ch == 'r') gpuView.refresh();
            if (ch == '\n' || ch == 13) {
                auto& item = gpuView.view.items[gpuView.view.selected];
                auto tag = item.tag;
                if (tag == "switch_virgl") app.gpu.switchToDriver("virgl");
                else if (tag == "switch_zink") app.gpu.switchToDriver("zink");
                else if (tag == "benchmark") app.gpu.benchmark(3.0f);
                gpuView.refresh();
            }
        }
        else if (view == 3) {
            app.screen.handleInput(ch, appStoreView.view);
            if (ch == 'q') view = 0;
            if (ch == 'r') appStoreView.rebuild();
            if (ch == '\n' || ch == 13) {
                auto& item = appStoreView.view.items[appStoreView.view.selected];
                if (item.tag == "wine_stock") app.appstore.installWine("stock");
                else if (item.tag == "wine_hangover") app.appstore.installWine("wine_hangover");
                else if (!item.tag.empty()) app.appstore.installApp(item.tag);
                appStoreView.rebuild();
            }
        }
    }

    return 0;
}

} // namespace app
} // namespace tkde

// ─── Entry point ────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    using namespace tkde;

    bool daemon = false;
    bool no_tui = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--daemon") daemon = true;
        else if (arg == "-t" || arg == "--no-tui") no_tui = true;
        else if (arg == "-h" || arg == "--help") {
            std::printf("Usage: tkde-app [options]\n"
                        "  -d, --daemon    Run as background daemon\n"
                        "  -t, --no-tui    Run without TUI (headless)\n"
                        "  -h, --help      Show this help\n");
            return 0;
        }
    }

    g_log.open("/data/data/com.termux/files/home/.cache/tkde-app/tkde-app.log");
    g_log.setMinLevel(LogLevel::INFO);
    g_log.setTermuxColor(true);

    TKDE_INFO("tkde-app v2.1.0 starting");
    TKDE_INFO("Termux Desktop KDE Integration App");

    app::App app;
    app.running = true;
    app.config.load();

    if (!app.gpu.init()) {
        TKDE_WARN("GPU monitor init failed");
    }

    TKDE_INFO("tkde-app ready");

    int exitCode = 0;
    if (no_tui || daemon) {
        if (daemon && fork() > 0) return 0;
        while (app.running) {
            sleep(30);
            auto s = app.display.getStatus();
            TKDE_INFO("Heartbeat VNC:%s X11:%s GPU:%d%%",
                s.vnc_running ? "on" : "off", s.tx11_running ? "on" : "off", s.gpu_utilization);
        }
    } else {
        if (!app.screen.init()) {
            TKDE_ERROR("Failed to initialize screen");
            return 1;
        }
        exitCode = app::runMain(app);
        app.screen.shutdown();
    }

    TKDE_INFO("tkde-app exiting (code=%d)", exitCode);
    g_log.close();
    return exitCode;
}
