This is a simple, game engine-agnostic tool for assigning hitbox and hurtbox data to sprite animations.
I do not recommend this for general usage, as it is a personal tool for my game.
As such, this readme may not be up-to-date.

<img src="screenshot.png">

Building: Linux only. Install cJson 1.7.15, raylib 4.2, and their respective dependencies to `/usr/local/include`. Run `make build` to build.

Usage: cac [path to image file (with .png extension.)]<br>
Saves are written to a .json file with the same name as the image file.

|        Action         |            Key             |
|:---------------------:|:--------------------------:|
|         Save          |          Ctrl + S          |
|         Undo          |          Ctrl + Z          |
|         Redo          |      Ctrl + Shift + Z      |
|          Pan          |     Left Mouse Button      |
|         Zoom          |        Mouse Wheel         |
|       Add frame       |          Alt + N           |
|     Remove frame      |       Alt + Backspace      |
|    Play animation     |           Enter            |
|      New circle       |          Ctrl + 1          |
|      New square       |          Ctrl + 2          |
|     New rectangle     |          Ctrl + 3          |
|      Remove shape     |      Ctrl + Backspace      |
|      New hurtbox      |   Shape Shortcut + Shift   |
|  Edit frame duration  | Ctrl + D, Enter to confirm |
| Move through timeline |         Arrow Keys         |
| Toggle selected value |          Space Bar         |
