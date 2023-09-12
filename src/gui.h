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
} WindowTheme;

typedef struct Window {
    WindowTheme *theme;
    LIST(DrawCommand) commands;
    
    void (*update)(struct Window *self);
    
    Rectangle rect;
    
    char *title;
    
    // @todo: Add these
    // bool closed;
    // bool closable;
    
    bool mousePresent;
    Vector2 mousePos;
    bool mousePressed;
} Window;

DrawCommand DrawCommandRect(Rectangle rect, Color color);
DrawCommand DrawCommandString(Vector2 pos, char *text, Font *font, int fontSize, Color color);
void DrawCommandsDraw(LIST(DrawCommand) commands);

Window WindowNew(Vector2 pos, char *title, WindowTheme *theme, void (*update)(Window *window));
LIST(DrawCommand) WindowsUpdate(LIST(Window *) windows, bool *mouseEnabled);
bool WindowButton(Window *window, char *text);

#endif
