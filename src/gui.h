#ifndef GUI_H
#define GUI_H

#include "raylib.h"

#include "list.h"

typedef struct ButtonTheme {
    Font *font;
    int fontSize;
    Color colorIdleRect;
    Color colorIdleText;
    Color colorHoveredRect;
    Color colorHoveredText;
    Color colorClickedRect;
    Color colorClickedText;
} ButtonTheme;

typedef struct DrawCommand {
    // Ideally this would be vertex information and texture handles but we'll just do this for now
    enum {
        DRAW_COMMAND_RECT,
        DRAW_COMMAND_STRING
    } type;

    union {
        struct {
            Vector2 pos;
            char *text;
            Font *font;
            int fontSize;
            Color color;
        } string;

        struct {
            Rectangle bounds;
            Color color;
        } rect;
    };
} DrawCommand;

typedef struct WindowTheme {
    Font *font;
    int fontSize;
    int margin;
    
    Color titleColor;
    Color bodyColor;

    Color fontColor;
    Color fontHoveredColor;
    Color fontClickedColor;

    Color elementColor;
    Color elementHoveredColor;
    Color elementClickedColor;

    Color entryFieldColor;
    Color entryFieldHoveredColor;
    Color entryFieldClickedColor;
    
    Color entryFieldFontColor;
    Color entryFieldClickedFontColor;
    Color entryFieldHoveredFontColor;
} WindowTheme;

struct Window;

typedef struct WindowManager {
    LIST(struct Window) windows;
    LIST(DrawCommand) commands; // window's current commands
    LIST(DrawCommand) commandsAll; // all commands

    bool mousePresent;
    Vector2 mousePos;
    bool mousePressed;
    bool mouseDown;
    char charPressed;
    
    int windowIdx;
} WindowManager;

typedef struct Window {
    int id;
    char *title;
    
    WindowTheme *theme;
    WindowManager *manager;
    
    Rectangle rect; 
    
    // @todo: Add these
    // bool closed;
    // bool closable;
} Window;

DrawCommand DrawCommandRect(Rectangle rect, Color color);
DrawCommand DrawCommandString(Vector2 pos, char *text, Font *font, int fontSize, Color color);
void DrawCommandsDraw(LIST(DrawCommand) commands);

bool WindowButton(Window *window, char *text);
void WindowTextField(Window *window, StringBuffer *buffer, bool *enabled);

WindowManager WindowManagerNew();
void WindowManagerFree(WindowManager *manager);
void WindowManagerAddWindow(WindowManager *manager, char *title, WindowTheme *theme, Vector2 pos, int id);
void WindowManagerStart(WindowManager *manager);
Window *WindowManagerNext(WindowManager *manager);
void WindowManagerEnd(WindowManager *manager);

#endif
