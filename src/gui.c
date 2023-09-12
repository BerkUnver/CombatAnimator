#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "raylib.h"
#include "raygui.h"
#include "list.h"

#include "gui.h"

DrawCommand DrawCommandRect(Rectangle rect, Color color) {
    return (DrawCommand) {
        .type = DRAW_COMMAND_RECT,
        .rect.bounds = rect,
        .rect.color = color
    };
}

DrawCommand DrawCommandString(Vector2 pos, char *text, Font *font, int fontSize, Color color) {
    return (DrawCommand) {
        .type = DRAW_COMMAND_STRING,
        .string.pos = pos,
        .string.text = text,
        .string.font = font,
        .string.fontSize = fontSize,
        .string.color = color
    };
}

void DrawCommandsDraw(LIST(DrawCommand) commands) {
    for (int i = 0; i < LIST_COUNT(commands); i++) {
        DrawCommand *command = commands + i;
        switch (command->type) {
            case DRAW_COMMAND_RECT:
                DrawRectangleRec(command->rect.bounds, command->rect.color);
                break;
            case DRAW_COMMAND_STRING:
                DrawTextEx(
                    *command->string.font,
                    command->string.text, 
                    command->string.pos, 
                    command->string.fontSize,
                    1.0f,
                    command->string.color
                );
                break;
        }
    }
}

Window WindowNew(Vector2 pos, char *title, WindowTheme *theme, void (*update)(Window *window)) {
    return (Window) {
        .rect = (Rectangle) {pos.x, pos.y, 0, 0},
        .theme = theme,
        .update = update,
        .title = title
    };
};

LIST(DrawCommand) WindowsUpdate(LIST(Window *) windows, bool *mouseEnabled) {
    Vector2 mouse = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    
    LIST(DrawCommand) commands = LIST_NEW(DrawCommand);
    LIST(DrawCommand) commandsAll = LIST_NEW(DrawCommand);

    for (int i = 0; i < LIST_COUNT(windows); i++) {
        LIST_SHRINK(commands, 0);
        
        Window *window = windows[i];
        window->commands = commands;

        window->rect.width = window->theme->margin;
        int titleHeight = window->theme->margin * 2 + window->theme->fontSize;
        window->rect.height = titleHeight;

        if (*mouseEnabled) {
            window->mousePresent = true;
            window->mousePos = mouse;
            window->mousePressed = mousePressed;
        } else {
            window->mousePresent = false;
        }
        
        if (window->update) {
            window->update(window);
            commands = window->commands; // in case it was reallocated because it was resized
        }
        
        int windowWidthMin = MeasureText(window->title, window->theme->fontSize) * window->theme->margin * 2;
        if (window->rect.width < windowWidthMin) window->rect.width = windowWidthMin;
        
        if (CheckCollisionPointRec(mouse, window->rect)) *mouseEnabled = false;
        
        Rectangle titleRect = {
            window->rect.x, 
            window->rect.y, 
            window->rect.width, 
            titleHeight
        };
        
        Vector2 titlePos = {
            window->rect.x + window->theme->margin, 
            window->rect.y + window->theme->margin
        };
        
        Rectangle bodyRect = {
            window->rect.x, 
            window->rect.y + titleRect.height, 
            window->rect.width, 
            window->rect.height - titleRect.height
        };
        
        LIST_ADD(&commandsAll, DrawCommandRect(titleRect, window->theme->titleColor));
        LIST_ADD(&commandsAll, DrawCommandString(titlePos, window->title, window->theme->font, window->theme->fontSize, window->theme->fontColor));
        LIST_ADD(&commandsAll, DrawCommandRect(bodyRect, window->theme->bodyColor));
        LIST_ADD_MANY(&commandsAll, commands);
    }
    
    LIST_FREE(commands);
   
    return commandsAll;
}


bool WindowButton(Window *window, char *text) {
    int textWidth = MeasureText(text, window->theme->fontSize) + window->theme->margin * 2;
    int textHeight = window->theme->fontSize + window->theme->margin * 2;
    
    Rectangle rect = { 
        window->rect.x + window->theme->margin, 
        window->rect.y + window->rect.height, 
        textWidth, 
        textHeight 
    };
    
    Vector2 textPos = {
        window->rect.x + window->theme->margin * 2,
        window->rect.y + window->rect.height + window->theme->margin 
    };
    
    if (window->rect.width < textWidth) window->rect.width = textWidth; 
    window->rect.height += window->theme->fontSize + window->theme->margin * 2;
    
    Color colorRect = window->theme->elementColor;
    Color colorText = window->theme->fontColor;
    bool pressed = false;

    if (window->mousePresent && CheckCollisionPointRec(window->mousePos, rect)) {
        if (window->mousePressed) {
            colorRect = window->theme->elementClickedColor;
            colorText = window->theme->fontClickedColor;
            pressed = true;
        } else {
            colorRect = window->theme->elementHoveredColor;
            colorText = window->theme->fontHoveredColor;
        }
    }
    
    LIST_ADD(&window->commands, DrawCommandRect(rect, colorRect));
    LIST_ADD(&window->commands, DrawCommandString(textPos, text, window->theme->font, window->theme->fontSize, colorText));
    
    return pressed;
}
