#ifndef TKDE_UI_HPP
#define TKDE_UI_HPP

#include <functional>
#include <string>
#include <vector>

namespace tkde {
namespace ui {

enum Color {
    BLACK = 0, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
    BRIGHT_BLACK = 8, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW,
    BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE,
    DEFAULT = -1
};

struct Rect { int x = 0, y = 0, w = 0, h = 0; };

struct Label {
    Rect frame;
    std::string text;
    Color fg = DEFAULT, bg = DEFAULT;
    bool bold = false;
    bool center = false;
    bool visible = true;
};

struct Button {
    Rect frame;
    std::string text;
    Color fg = DEFAULT, bg = DEFAULT;
    bool selected = false;
    bool visible = true;
    std::function<void()> onPress;
};

struct ProgressBar {
    Rect frame;
    float value = 0.0f;
    Color fill = CYAN, emptyBg = BLACK;
    std::string label;
    bool visible = true;
};

struct ListItem {
    std::string label;
    std::string sublabel;
    bool selected = false;
    bool enabled = true;
    std::string tag;
};

struct ListView {
    Rect frame;
    std::vector<ListItem> items;
    int selected = 0;
    int scroll_offset = 0;
    bool border = true;
    std::string title;
    std::string filter;
    bool visible = true;
    std::function<void(const ListItem&, int)> onSelect;
};

struct TextBox {
    Rect frame;
    std::string text;
    Color fg = DEFAULT, bg = DEFAULT;
    bool border = true;
    bool readonly = false;
    bool visible = true;
};

struct Gauge {
    Rect frame;
    std::string label;
    int value = 0;
    std::string unit;
    Color color = CYAN;
    bool visible = true;
};

struct Panel {
    Rect frame;
    std::string title;
    std::vector<Label> labels;
    std::vector<Button> buttons;
    std::vector<Gauge> gauges;
    bool border = true;
    bool visible = true;
};

// ─── Screen ─────────────────────────────────────────────────────────────────

class Screen {
    int width_ = 0, height_ = 0;
    std::vector<Panel> panels_;
    Panel* activePanel_ = nullptr;

public:
    Screen();
    ~Screen();

    bool init();
    void shutdown();
    void refresh();
    void clear();

    int cols() const { return width_; }
    int rows() const { return height_; }

    void addPanel(const Panel& p) { panels_.push_back(p); }
    void setActivePanel(const std::string& title);
    Panel* getPanel(const std::string& title);

    void drawRect(const Rect& r, Color bg);
    void drawText(int x, int y, const std::string& s, Color fg, Color bg, bool bold = false);
    void drawHLine(int x, int y, int len, Color color);
    void drawVLine(int x, int y, int len, Color color);
    void drawBorder(const Rect& r, Color color);

    void renderLabel(const Label& l);
    void renderButton(const Button& b);
    void renderProgressBar(const ProgressBar& p);
    void renderListView(const ListView& lv);
    void renderTextBox(const TextBox& t);
    void renderGauge(const Gauge& g);
    void renderPanel(const Panel& p);

    Panel makeStatusPanel(int x, int y, int w, int h);
    Panel makeGPUPanel(int x, int y, int w, int h);
    Panel makeSessionPanel(int x, int y, int w, int h);
    Panel makeMenuPanel(int x, int y, int w, int h);
    Panel makeAppStorePanel(int x, int y, int w, int h);

    bool handleInput(int ch, ListView& lv);
    int kbhit();
    int getch_noblock();
};

} // namespace ui
} // namespace tkde
#endif // TKDE_UI_HPP
