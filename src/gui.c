#include <stdbool.h>
#include "raylib.h"
#include "raygui.h"

void GuiFlagEditor(Rectangle rect, unsigned int *flags) {
    int width = rect.width / 8;
    int height = rect.height / 4;
    for (int x = 0; x < 8; x++)
    for (int y = 0; y < 4; y++) {
        Rectangle rectButton = {
            .x = rect.x + x * width,
            .y = rect.y + y * height,
            .width = width,
            .height = height,
        };
        char name[3];
        sprintf(name, "%i");
        unsigned int mask = 1 << (x * 4 + y);
        if (DrawButton(rectButton, name) && (flags & (1 << mask))) {
            flags |= mask; 
        } else {
            flags &= ~mask;
        }
    }
}

void GuiTextEditor(Rectangle rect, StringBuffer *buffer, bool *active) {
    Vector2 mouse = GetMousePosition();
    if (*active) {

    Rectangle rectInner = rect;
    
    rectInner.x += 2;
    rectInner.y += 2;
    rectInner.width = rectInner.width <= 2 ? 0 : rectInner.width - 2;
    rectInner.height = rectInner.height <= 2 ? 0 : rectInner.height - 2;
    
    DrawRectangleRec(rect, (Color) {97, 51, 45, 255});       // rust color
    if (!active) {
        DrawRectangleRec(rectInner, (Color) {146, 73, 65, 255}); // dull red
        if (CheckCollisionPointRect(rect, mouse)) *active = true;
    
    } else {
        bool clickedOutside = !CheckCollisionPointRect(rect, mouse) && !IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (clickedOutside || IsKeyPressed(KEY_ESC)) {
            *active = false;

        } else if (c != 0) {
            int c = GetCharPressed();
            if (' ' <= c && c <= '~') { // Normal ascii characters
                StringBufferAddChar(buffer, c);
            } else if (c == 127) { // Ascii code for delete
                StringBufferRemoveChar(buffer, c);
            }
        }
    }
    
    int fontAscent = GetFontDefault().baseSize;

    int textX = rect.y + (rect.height - fontAscent) / 2;
    int textY = 4;
    
    DrawText(buffer.raw, textX, textY, fontAscent, RAYWHITE);
}
