#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>
#include <iostream>
using namespace std;
class Event {
    protected:
        sf::Vector2i mouse;
    public:
        enum Click {
            UNCLICK,
            CLICK,
            CLICKED,
        };
        enum Key {
            Left,
            Right,
            BackSpace,
            Enter,
        };
        template <typename T>
        static Event::Click onclick(T& area, sf::Event& event, sf::RenderWindow& win);
        template <typename T>
        static bool is_focus(T& area, sf::Event& event, sf::RenderWindow& win, bool& focus);
        static char get_char(sf::Event& event, sf::RenderWindow& win);
        Event() = default;
};
template <typename T>
Event::Click Event::onclick(T& area, sf::Event& event, sf::RenderWindow& win)
{
    sf::Vector2f m;

    if (event.type != sf::Event::MouseButtonPressed)
        return UNCLICK;
    m = (sf::Vector2f)sf::Mouse::getPosition(win);
    if (area.getGlobalBounds().contains(m))
        return CLICKED;
    return CLICK;
}
template <typename T>
bool Event::is_focus(T& area, sf::Event& event, sf::RenderWindow& win, bool& focus)
{
    Event::Click c = onclick(area, event, win);

    if ((focus && c != CLICK) || (c == CLICKED))
        return true;
    return false;
}
char Event::get_char(sf::Event& event, sf::RenderWindow& win)
{
    char c = 0;
    bool ctrl = false;
    bool shift = false;

    if (event.type == sf::Event::TextEntered){
        c = event.text.unicode;
        if (c >= 32 && c <= 126)
            return c;
    }
    if (event.type == sf::Event::KeyPressed){
        ctrl = event.key.control;
        shift = event.key.shift;
        if (event.key.code == sf::Keyboard::Left)
            return Left + 1;
        if (event.key.code == sf::Keyboard::Right)
            return Right + 1;
        if (event.key.code == sf::Keyboard::BackSpace)
            return BackSpace + 1;
    }
    return c;
}

class Glyph {
    public:
        typedef struct Metrics {
            float advance;
            float bearingX;
            float bearingY;
            float width;
            float height;
            char c;
        } metrics_t;
        metrics_t glyphs[256];
        float kernings[256][256];
        void init(sf::Font& font, int size, int outline){
            sf::Glyph gl;

            for (int i = 0; i < 256; i++){
                gl = font.getGlyph((unsigned char)(i), size, false, outline);
                glyphs[i].advance = gl.advance;
                glyphs[i].bearingX = gl.bounds.left;
                glyphs[i].bearingY = gl.bounds.top;
                glyphs[i].c = (char)i;
                glyphs[i].height = gl.bounds.height;
                glyphs[i].width = gl.bounds.width;
                for (int j = 0; j < 256; j++)
                    kernings[i][j] = font.getKerning(i, j, size);
            }
        }
        Glyph(){}
        float get_width(const string& str){
            float x = 0;
            unsigned char c = 0;
            unsigned char p = 0;

            for (int i = 0; i < str.size(); i++){
                c = str[i];
                x += glyphs[c].advance;
                x += kernings[p][c];
                p = c;
            }
            return x;
        }
        vector<float> get_prefix_sum(const string& str){
            vector<float> prefix(static_cast<int>(str.size()));
            float w = 0;

            if (str.empty())
                return prefix;
            prefix[0] = (glyphs[(int)str[0]].advance);
            for (int i = 1; i < str.size(); i++){
                w = glyphs[(int)str[i]].advance + kernings[str[i - 1]][str[i]];
                prefix[i] = (prefix[i - 1] + w);
            }
            return prefix;
        }
};

class Input {
    private:
        int width;
        int height;
        sf::Color o_color;
        sf::Color i_color;
        sf::Color s_color;
        sf::Color p_color;
        string value;
        string placeholder;
        int off_begin;
        int off_end;
        int offset;
        int index;
        int size;
        bool show;
        sf::RectangleShape caret;
        sf::Text text;
        sf::Font font;
        sf::Vector2f position;
        sf::Clock clock;
        static constexpr int WIDTH = 200;
        static constexpr int HEIGHT = 30;
        static constexpr int SIZE = 20;
        static const sf::Color OUT_COLOR;
        static const sf::Color IN_COLOR;
        static const sf::Color STR_COLOR;
        static const sf::Color P_COLOR;
        static constexpr int HID_CARET = 500;
        static constexpr int SHOW_CARET = 500;
        Glyph glyph;
    public:
        bool focus;
        sf::RectangleShape field;
        vector<float> prefix;
        Input(sf::Vector2f position, string placeholder = "Type text...", string font_s = "./arial.ttf", int height = HEIGHT, int width = WIDTH, int size = SIZE, sf::Color o_color = OUT_COLOR, sf::Color i_color = IN_COLOR, sf::Color s_color = STR_COLOR, sf::Color p_color = P_COLOR);
        void draw(sf::RenderWindow& win);
        void handle_event(sf::Event &event, sf::RenderWindow& win);
        void update_offset();
};

void Input::update_offset()
{
    sf::FloatRect r(0, 0, 0, 0);
    string str = "";

    text.setFillColor((!focus) ? p_color : (!value.empty()) ? s_color : p_color);
    if (value.size() == 0){
        caret.setPosition(sf::Vector2f(position.x + 2.f, position.y + (height - caret.getSize().y) / 2.f));
        text.setString(placeholder);
        return;
    }
    if (!value.empty())
        caret.setPosition(sf::Vector2f(prefix[index - 1] + position.x, position.y));
    text.setString(value);
}

void Input::handle_event(sf::Event& event, sf::RenderWindow& win)
{
    char c = Event::get_char(event, win);

    focus = Event::is_focus<sf::RectangleShape>(field, event, win, focus);
    if (!focus)
        index = 0;
    if (c == 0)
        return;
    if (c == Event::Left + 1)
        index = max(0, index - 1);
    if (c == Event::Right + 1)
        index = min(static_cast<int>(value.size()), index + 1);
    if (c >= 32 && c <= 126){
        value.insert(value.begin() + index, c);
        prefix = glyph.get_prefix_sum(value);
        index++;
    }
    if (c == Event::BackSpace + 1 && index > 0){
        value.erase(value.begin() + index - 1);
        prefix = glyph.get_prefix_sum(value);
        index--;
    }
    update_offset();
}

const sf::Color Input::OUT_COLOR = sf::Color(60, 63, 69);
const sf::Color Input::IN_COLOR = sf::Color(43, 45, 49);
const sf::Color Input::STR_COLOR = sf::Color::White;
const sf::Color Input::P_COLOR = sf::Color(154, 160, 166);
Input::Input(sf::Vector2f position, string p, string font_s, int h, int w, int s, sf::Color o_c, sf::Color i_c, sf::Color s_c, sf::Color p_c):
position(position),
height(h),
width(w),
size((s <= (h * 5.f / 6.f) && s >= (h * 1.f / 3.f)) ? s : (h * 4.f / 6.f)),
o_color(o_c),
i_color(i_c),
s_color(s_c),
placeholder(p),
p_color(p_c),
value(""),
show(true),
focus(false),
off_begin(0),
off_end(0),
offset(0),
index(0)
{
    font.loadFromFile(font_s);
    field.setSize(sf::Vector2f(width, height));
    field.setOutlineColor(o_color);
    field.setFillColor(i_color);
    field.setOutlineThickness(3);
    field.setPosition(position);
    text.setFont(font);
    text.setPosition(position);
    text.setFillColor(p_color);
    text.setString(placeholder);
    text.setCharacterSize(size);
    caret.setSize(sf::Vector2f(2, height * 3.f / 4.f));
    caret.setFillColor(s_color);
    caret.setPosition(sf::Vector2f(position.x + 2, position.y + (height - caret.getSize().y) / 2.f));
    glyph.init(font, size, 0);
}

void Input::draw(sf::RenderWindow& win)
{
    int t = clock.getElapsedTime().asMilliseconds();

    win.draw(field);
    win.draw(text);
    if (show){
        if (t >= SHOW_CARET){
            show = false;
            clock.restart();
        }
    }
    else {
        if (t >= HID_CARET){
            show = true;
            clock.restart();
        }
    }
    if (focus && show)
        win.draw(caret);
}

int main(void)
{
    sf::RenderWindow window(sf::VideoMode(1920, 1080, 32), "Guess The hidden number");
    sf::Event event;
    Input input(sf::Vector2f(100, 100));

    while (window.isOpen()){
        while (window.pollEvent(event)){
            if (event.type == sf::Event::Closed)
                window.close();
            input.handle_event(event, window);
        }
        window.clear(sf::Color::Red);
        input.draw(window);
        window.display();
    }
    return (0);
}
