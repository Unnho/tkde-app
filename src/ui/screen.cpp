#include "ui.hpp"
#include "logger.hpp"
#include <algorithm>
#include <csignal>
#include <cstring>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

namespace tkde {
namespace ui {

// ─── Signal handling ──────────────────────────────────────────────────────────
static volatile sig_atomic_t g_sigwinch = 0;
static void sigwinchHandler(int) { g_sigwinch = 1; }

// ─── Terminal helpers ─────────────────────────────────────────────────────────
static struct termios origTerm;
static bool tty_raw_mode = false;

static void cleanupTty() {
    if (tty_raw_mode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTerm);
        tty_raw_mode = false;
    }
    write(STDOUT_FILENO, "\033[?1003l\r\n", 10); // disable mouse
    write(STDOUT_FILENO, "\033[?25h", 6);        // show cursor
    write(STDOUT_FILENO, "\033[0m", 4);          // reset colors
}

static void enterRawMode() {
    struct termios raw;
    if (tcgetattr(STDIN_FILENO, &origTerm) != 0) return;
    raw = origTerm;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    tty_raw_mode = true;
    atexit(cleanupTty);
}

// ─── Screen ────────────────────────────────────────────────────────────────────

Screen::Screen() {}

Screen::~Screen() { shutdown(); }

bool Screen::init() {
    signal(SIGWINCH, sigwinchHandler);
    enterRawMode();

    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        width_ = ws.ws_col;
        height_ = ws.ws_row;
    }
    if (width_ == 0) { width_ = 120; height_ = 40; }

    clear();
    write(STDOUT_FILENO, "\033[?25l", 6);  // hide cursor
    write(STDOUT_FILENO, "\033[?1003h", 8); // enable mouse
    return true;
}

void Screen::shutdown() {
    cleanupTty();
}

void Screen::clear() {
    write(STDOUT_FILENO, "\033[2J\033[H", 8);
}

void Screen::refresh() {
    if (g_sigwinch) {
        g_sigwinch = 0;
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
            width_ = ws.ws_col;
            height_ = ws.ws_row;
        }
    }
    write(STDOUT_FILENO, "\033[0m", 4);
}

// ─── Primitives ─────────────────────────────────────────────────────────────────

void Screen::drawRect(const Rect& r, Color bg) {
    if (bg == DEFAULT) return;
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "\033[48;5;%dm", bg);
    write(STDOUT_FILENO, buf, n);
    for (int row = r.y; row < r.y + r.h; ++row) {
        char move[16];
        snprintf(move, sizeof(move), "\033[%d;%dH", row + 1, r.x + 1);
        write(STDOUT_FILENO, move, strlen(move));
        char fill[16];
        snprintf(fill, sizeof(fill), "%.*s", r.w, "");
        for (int i = 0; i < r.w; ++i) write(STDOUT_FILENO, " ", 1);
    }
}

void Screen::drawText(int x, int y, const std::string& s, Color fg, Color bg, bool bold) {
    char buf[64];
    int n = 0;
    n += snprintf(buf + n, sizeof(buf) - n, "\033[%d;%dH", y + 1, x + 1);
    if (bg != DEFAULT) n += snprintf(buf + n, sizeof(buf) - n, "\033[48;5;%dm", bg);
    if (fg != DEFAULT) n += snprintf(buf + n, sizeof(buf) - n, "\033[38;5;%dm", fg);
    if (bold) n += snprintf(buf + n, sizeof(buf) - n, "\033[1m");
    write(STDOUT_FILENO, buf, n);
    write(STDOUT_FILENO, s.c_str(), s.size());
}

void Screen::drawHLine(int x, int y, int len, Color color) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "\033[%d;%dH\033[38;5;%dm", y + 1, x + 1, color);
    write(STDOUT_FILENO, buf, n);
    for (int i = 0; i < len; ++i) write(STDOUT_FILENO, "\u2500", 4); // ┄
}

void Screen::drawVLine(int x, int y, int len, Color color) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "\033[%d;%dH\033[38;5;%dm", y + 1, x + 1, color);
    write(STDOUT_FILENO, buf, n);
    for (int i = 0; i < len; ++i) {
        write(STDOUT_FILENO, "\u2502", 4); // │
        write(STDOUT_FILENO, "\033[1B", 3);
        write(STDOUT_FILENO, "\033[1D", 3);
    }
}

void Screen::drawBorder(const Rect& r, Color color) {
    if (r.w < 2 || r.h < 2) return;
    char esc[16];
    snprintf(esc, sizeof(esc), "\033[38;5;%dm", color);

    // Corners
    drawText(r.x, r.y, "\u250C", color, DEFAULT);              // ┌
    drawText(r.x + r.w - 1, r.y, "\u2510", color, DEFAULT);     // ┐
    drawText(r.x, r.y + r.h - 1, "\u2514", color, DEFAULT);     // └
    drawText(r.x + r.w - 1, r.y + r.h - 1, "\u2518", color, DEFAULT); // ┘

    // Sides
    drawHLine(r.x + 1, r.y, r.w - 2, color);
    drawHLine(r.x + 1, r.y + r.h - 1, r.w - 2, color);
    drawVLine(r.x, r.y + 1, r.h - 2, color);
    drawVLine(r.x + r.w - 1, r.y + 1, r.h - 2, color);
}

// ─── Widget renderers ─────────────────────────────────────────────────────────

void Screen::renderLabel(const Label& l) {
    if (!l.visible) return;
    int tx = l.center ? l.frame.x + (l.frame.w - static_cast<int>(l.text.size())) / 2 : l.frame.x;
    if (tx < l.frame.x) tx = l.frame.x;
    drawText(tx, l.frame.y, l.text, l.fg, l.bg, l.bold);
}

void Screen::renderButton(const Button& b) {
    if (!b.visible) return;
    Color bgCol = b.selected ? CYAN : DEFAULT;
    Color fgCol = b.selected ? BLACK : (b.fg == DEFAULT ? WHITE : b.fg);
    if (b.frame.w > 0 && b.frame.h > 0) {
        Rect br = b.frame;
        drawRect({br.x, br.y, static_cast<int>(b.text.size()) + 2, 1}, bgCol);
        drawText(br.x, br.y, " " + b.text + " ", fgCol, bgCol, b.selected);
    }
}

void Screen::renderProgressBar(const ProgressBar& p) {
    if (!p.visible) return;
    Rect r = p.frame;
    int barW = r.w - 2;
    int fill = static_cast<int>(p.value * barW);

    drawText(r.x, r.y, "[", WHITE, DEFAULT);
    for (int i = 0; i < barW; ++i) {
        Color c = (i < fill) ? p.fill : p.emptyBg;
        std::string ch = (i < fill) ? "\u2588" : "\u2591";
        drawText(r.x + 1 + i, r.y, ch, c, DEFAULT);
    }
    drawText(r.x + r.w - 1, r.y, "]", WHITE, DEFAULT);
    if (!p.label.empty()) {
        drawText(r.x + 1, r.y, p.label, WHITE, DEFAULT);
    }
}

void Screen::renderListView(const ListView& lv) {
    if (!lv.visible) return;
    Rect r = lv.frame;

    if (lv.border) drawBorder(r, BLUE);

    // Title
    if (!lv.title.empty()) {
        drawText(r.x + 2, r.y, "\u2502 " + lv.title + " \u2502", BRIGHT_BLUE, DEFAULT, true);
    }

    // Filter hint
    if (!lv.filter.empty()) {
        drawText(r.x + 1, r.y + r.h - 1, "Filter: " + lv.filter, WHITE, DEFAULT);
    }

    // Items
    int visRows = r.h - 2 - (!lv.filter.empty() ? 1 : 0);
    if (visRows <= 0) return;

    for (int i = 0; i < visRows && (i + lv.scroll_offset) < static_cast<int>(lv.items.size()); ++i) {
        int idx = i + lv.scroll_offset;
        const auto& item = lv.items[idx];
        int row = r.y + 1 + i;
        bool sel = (idx == lv.selected);

        Color fg = DEFAULT, bg = DEFAULT;
        if (!item.enabled) fg = BRIGHT_BLACK;
        if (sel) { fg = BLACK; bg = CYAN; }

        std::string prefix = sel ? "\u276F " : "  ";
        std::string line = prefix + item.label;
        if (!item.sublabel.empty()) line += " \u2502 " + item.sublabel;

        if (bg != DEFAULT) {
            for (int c = r.x; c < r.x + r.w; ++c) {
                drawText(c, row, " ", DEFAULT, bg);
            }
        }
        drawText(r.x + 1, row, line, fg, bg, sel);
    }

    // Scrollbar
    if (lv.items.size() > static_cast<size_t>(visRows)) {
        int sbPos = r.y + 1 + (lv.selected * visRows / lv.items.size());
        if (sbPos > r.y + r.h - 2) sbPos = r.y + r.h - 2;
        drawText(r.x + r.w - 1, sbPos, "\u25BC", BLUE, DEFAULT, true);
    }
}

void Screen::renderTextBox(const TextBox& t) {
    if (!t.visible) return;
    Rect r = t.frame;
    if (t.border) drawBorder(r, WHITE);

    std::string display = t.text;
    if (display.size() > static_cast<size_t>(r.w - 2)) {
        display = display.substr(display.size() - (r.w - 4));
    }
    drawText(r.x + 1, r.y + 1, display, t.fg, t.bg);
}

void Screen::renderGauge(const Gauge& g) {
    if (!g.visible) return;
    Rect r = g.frame;
    int barW = r.w - static_cast<int>(g.label.size() + g.unit.size() + 8);
    int filled = (g.value * barW) / 100;
    if (filled < 0) filled = 0;
    if (filled > barW) filled = barW;

    drawText(r.x, r.y, g.label, WHITE, DEFAULT, true);
    drawText(r.x + r.w - static_cast<int>(g.unit.size()) - 4, r.y,
        std::to_string(g.value) + "% " + g.unit, WHITE, DEFAULT);

    for (int i = 0; i < barW; ++i) {
        Color c = (i < filled) ? g.color : BRIGHT_BLACK;
        drawText(r.x + r.w - barW + i, r.y + 1, "\u2588", c, DEFAULT);
    }
}

void Screen::renderPanel(const Panel& p) {
    if (!p.visible) return;
    Rect r = p.frame;

    if (p.border) drawBorder(r, WHITE);

    // Title bar
    if (!p.title.empty()) {
        drawRect({r.x, r.y, r.w, 1}, BLUE);
        std::string title = " " + p.title + " ";
        drawText(r.x, r.y, title, WHITE, BLUE, true);
    }

    for (const auto& l : p.labels) renderLabel(l);
    for (const auto& b : p.buttons) renderButton(b);
    for (const auto& g : p.gauges) renderGauge(g);
}

// ─── Input ──────────────────────────────────────────────────────────────────────

int Screen::kbhit() {
    struct timeval tv = {0, 0};
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    return select(STDIN_FILENO + 1, &rfds, nullptr, nullptr, &tv) > 0 ? 1 : 0;
}

int Screen::getch_noblock() {
    char c;
    if (read(STDIN_FILENO, &c, 1) > 0) return static_cast<unsigned char>(c);
    return -1;
}

bool Screen::handleInput(int ch, ListView& lv) {
    int n = static_cast<int>(lv.items.size());
    if (n == 0) return false;

    switch (ch) {
        case 'j': case 'S' + 1:
            lv.selected++;
            if (lv.selected >= n) lv.selected = 0;
            return true;
        case 'k': case 'W' + 1:
            lv.selected--;
            if (lv.selected < 0) lv.selected = n - 1;
            return true;
        case 'g':
            lv.selected = 0;
            return true;
        case 'G':
            lv.selected = n - 1;
            return true;
        case '\n': case 13: case 'l':
            if (lv.onSelect) lv.onSelect(lv.items[lv.selected], lv.selected);
            return true;
        case 'q': case 27:
            return false;
        default:
            return false;
    }
}

// ─── Composite panels ──────────────────────────────────────────────────────────

Panel Screen::makeStatusPanel(int x, int y, int w, int h) {
    Panel p;
    p.frame = {x, y, w, h};
    p.title = " System Status ";
    p.border = true;
    return p;
}

Panel Screen::makeGPUPanel(int x, int y, int w, int h) {
    Panel p;
    p.frame = {x, y, w, h};
    p.title = " GPU Monitor ";
    p.border = true;
    return p;
}

Panel Screen::makeSessionPanel(int x, int y, int w, int h) {
    Panel p;
    p.frame = {x, y, w, h};
    p.title = " Session Manager ";
    p.border = true;
    return p;
}

Panel Screen::makeMenuPanel(int x, int y, int w, int h) {
    Panel p;
    p.frame = {x, y, w, h};
    p.title = " Menu ";
    p.border = true;
    return p;
}

Panel Screen::makeAppStorePanel(int x, int y, int w, int h) {
    Panel p;
    p.frame = {x, y, w, h};
    p.title = " App Store ";
    p.border = true;
    return p;
}

void Screen::setActivePanel(const std::string& title) {
    for (auto& p : panels_) {
        if (p.title == title) activePanel_ = &p;
    }
}

Panel* Screen::getPanel(const std::string& title) {
    for (auto& p : panels_) {
        if (p.title == title) return &p;
    }
    return nullptr;
}

} // namespace ui
} // namespace tkde