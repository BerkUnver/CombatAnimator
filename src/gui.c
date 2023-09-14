#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "raylib.h"
#include "raygui.h"

#include "list.h"
#include "string_buffer.h"

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
    
    if (window->rect.width < textWidth) window->rect.width = textWidth + window->theme->margin * 2; 
    window->rect.height += window->theme->fontSize + window->theme->margin * 2;
    
    Color colorRect = window->theme->elementColor;
    Color colorText = window->theme->fontColor;
    bool pressed = false;

    WindowManager *m = window->manager;
    if (m->mousePresent && CheckCollisionPointRec(m->mousePos, rect)) {
        if (m->mousePressed || m->mouseDown) { 
            colorRect = window->theme->elementClickedColor;
            colorText = window->theme->fontClickedColor;
            if (m->mousePressed) pressed = true;
        } else {
            colorRect = window->theme->elementHoveredColor;
            colorText = window->theme->fontHoveredColor;
        }
    }
    
    LIST_ADD(&m->commands, DrawCommandRect(rect, colorRect));
    LIST_ADD(&m->commands, DrawCommandString(textPos, text, window->theme->font, window->theme->fontSize, colorText));
    
    return pressed;
}

/*
void WindowTextField(Window *window, StringBuffer *buffer, bool *enabled) {
    Color elementColor;
    Color fieldColor;
    Color fontColor;
 
    if (*enabled) {
        char c = window->charPressed;
        if (' ' <= c && c <= '~') {
            StringBufferAddChar(buffer, c);
        } else if (c == 127 && buffer->length > 0) { // delete
            StringBufferRemoveChar(buffer);
        }  
    }

    int textWidth = MeasureText(buffer->raw, window->theme->fontSize) + window->theme->margin * 2;
    int textHeight = window->theme->fontSize + window->theme->margin * 2;

    Rectangle rect = {
        window->rect.x + window->theme->margin,
        window->rect.y + window->rect.height,
        textWidth,
        textHeight
    };
    
    if (window->mousePresent && window->mousePressed && CheckCollisionPointRec(window->mousePos, rect)) {
        if (window->mousePressed) {
            elementColor = window->theme->elementClickedColor;
            fieldColor = window->theme->entryFieldClickedColor;
            fontColor = window->theme->entryFieldClickedFontColor;
        } else {
            elementColor = window->theme->elementHoveredColor;
            fieldColor = window->theme->entryFieldHoveredColor;
            fontColor = window->theme->entryFieldClickedFontColor;
        }
    } else {
        elementColor = window->theme->elementColor;
        fieldColor = window->theme->entryFieldColor;
        fontColor = window->theme->entryFieldFontColor;
    }

    if (enabled) {
        if (window->mousePresent && window->mousePressed && !CheckCollisionPointRec(window->mousePos, rect)) {
            *enabled = false;
        }
    } else {
        if (window->mousePresent && window->mousePressed && CheckCollisionPointRec(window->mousePos, rect)) {
            *enabled = true;
        } 
    }

    Vector2 textPos = {
        window->rect.x + window->theme->margin * 2,
        window->rect.y + window->theme->margin + window->rect.height
    };

    Rectangle fieldRect = {
        textPos.x,
        window->rect.y + window->theme->margin,
        textWidth,
        window->theme->fontSize
    };
    
    DrawCommand textCmd = DrawCommandString(
        textPos, 
        buffer->raw, // This is sus as hell 
        window->theme->font, 
        window->theme->fontSize,
        fontColor
    );

    LIST_ADD(&window->commands, DrawCommandRect(rect, elementColor));
    LIST_ADD(&window->commands, DrawCommandRect(fieldRect, fieldColor));
    LIST_ADD(&window->commands, textCmd);
}
*/

WindowManager WindowManagerNew() {
    return (WindowManager) {
        .windows = LIST_NEW(Window),
        .commands = LIST_NEW(DrawCommand),
        .commandsAll = LIST_NEW(DrawCommand)
    };
}

void WindowManagerFree(WindowManager *manager) {
    LIST_FREE(manager->windows);
    LIST_FREE(manager->commands);
    LIST_FREE(manager->commandsAll);
}

void WindowManagerAddWindow(WindowManager *manager, char *title, WindowTheme *theme, Vector2 pos, int id) {
    Window window = {
        .id = id,
        .title = title,
        .manager = manager,
        .theme = theme,
        .rect = (Rectangle) {pos.x, pos.y, 0, 0},
    };
    LIST_ADD(&manager->windows, window);
}

void WindowManagerStart(WindowManager *manager) {
    LIST_SHRINK(manager->commands, 0);
    LIST_SHRINK(manager->commandsAll, 0);

    manager->windowIdx = 0;

    manager->mousePresent = true;
    manager->mousePos = GetMousePosition();
    manager->mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    manager->mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    manager->charPressed = GetCharPressed();
}

Window *WindowManagerNext(WindowManager *manager) {
    if (manager->windowIdx != 0) {
        Window *windowPrevious = manager->windows + manager->windowIdx - 1;
        if (CheckCollisionPointRec(manager->mousePos, windowPrevious->rect)) {
            manager->mousePresent = false;
        }

        Rectangle titleRect = {
            windowPrevious->rect.x, 
            windowPrevious->rect.y, 
            windowPrevious->rect.width, 
            windowPrevious->theme->margin * 2 + windowPrevious->theme->fontSize
        };
        
        Vector2 titlePos = {
            windowPrevious->rect.x + windowPrevious->theme->margin, 
            windowPrevious->rect.y + windowPrevious->theme->margin
        };
        
        Rectangle bodyRect = {
            windowPrevious->rect.x, 
            windowPrevious->rect.y + titleRect.height, 
            windowPrevious->rect.width, 
            windowPrevious->rect.height - titleRect.height
        };
        
        LIST_ADD(&manager->commandsAll, DrawCommandRect(titleRect, windowPrevious->theme->titleColor));
        LIST_ADD(&manager->commandsAll, DrawCommandString(titlePos, windowPrevious->title, windowPrevious->theme->font, windowPrevious->theme->fontSize, windowPrevious->theme->fontColor));
        LIST_ADD(&manager->commandsAll, DrawCommandRect(bodyRect, windowPrevious->theme->bodyColor));
        LIST_ADD_MANY(&manager->commandsAll, manager->commands);

        LIST_SHRINK(manager->commands, 0);
    }

    if (manager->windowIdx >= LIST_COUNT(manager->windows)) return NULL;
    
    Window *window = manager->windows + manager->windowIdx;
    manager->windowIdx++;
    window->rect.width = 0;
    window->rect.height = window->theme->margin * 2 + window->theme->fontSize;

    return window;
}

void WindowManagerEnd(WindowManager *manager) {
    for (int i = 0; i < LIST_COUNT(manager->commandsAll); i++) {
        DrawCommand *command = manager->commandsAll + i;
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
