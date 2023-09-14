#ifndef GUI_H
#define GUI_H

#include "raylib.h"

#include "list.h"

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

typedef struct ButtonTheme {
    Color color;
    Color clickedColor;
    Color hoveredColor; 

    Color fontColor;
    Color fontHoveredColor;
    Color fontClickedColor;
} ButtonTheme;

typedef struct WindowTheme {
    Font *font;
    int fontSize;
    int margin;
    
    Color titleColor;
    Color bodyColor;
    Color fontColor;
} WindowTheme;

typedef struct TextFieldTheme {
    int fieldWidthMin;
    int fieldMargin;

    Color color;
    Color hoveredColor;
    Color clickedColor;
    
    Color fieldColor;
    Color fieldHoveredColor;
    Color fieldClickedColor;

    Color fontColor;
    Color fontHoveredColor;
    Color fontClickedColor;
} TextFieldTheme;

struct Window;

typedef struct WindowManager {
    LIST(struct Window) windows;
    LIST(DrawCommand) commands; // window's current commands
    LIST(DrawCommand) commandsAll; // all commands

    bool mousePresent;
    Vector2 mousePos;
    bool mousePressed;
    bool mouseDown;

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

bool WindowButton(Window *window, char *text, ButtonTheme *theme);
void WindowTextField(Window *window, StringBuffer *buffer, bool *enabled, TextFieldTheme *theme);

WindowManager WindowManagerNew();
void WindowManagerFree(WindowManager *manager);
void WindowManagerAddWindow(WindowManager *manager, char *title, WindowTheme *theme, Vector2 pos, int id);
void WindowManagerStart(WindowManager *manager);
Window *WindowManagerNext(WindowManager *manager);
void WindowManagerEnd(WindowManager *manager);

#endif
