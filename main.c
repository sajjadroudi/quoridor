#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_UI_SIZE 17
#define MAX_ARRAY_SIZE (2*MAX_UI_SIZE+1)

typedef struct Player {
    Vector2 boardPos;
    Vector2 realPos;
    int wallCount;
    int mode;
    Color color;
} Player;

typedef struct Button {
    Rectangle rec;
    char text[10];
    Color color;
} Button;

int const menuWidth = 500;
int const menuHeight = 720;

int width = 1280, height = 720;
int squareSize = 70;
int thickness = 12;
int uiSize = 9;
int arraySize = 19;
int turnPlayer = 0;
int wallCount = 10;
int resolution = 2;
bool isFullScreen = false;

int pageCode = 0;
int savedGamesCounter = 0;
bool isSoundOn = true;
bool isDuo;

/*
 *  what each value of "pageCode" means:
 *  1: new game page (start func)
 *  2: load saved games page (loadGame func)
 *  3: settings page (settings func)
 *  4: exit ("return" to close the game compeletely)
 *  5: a saved game has been opened
 *  6: user wants to go back from menu to current game (continue) so initGame() must not be called
   7: user wants to switch from menu to game to start a saved or new game
*/

int map[MAX_ARRAY_SIZE][MAX_ARRAY_SIZE] = { 0 };
int mapCopy[MAX_ARRAY_SIZE][MAX_ARRAY_SIZE] = { 0 };
int mapUndo[MAX_ARRAY_SIZE][MAX_ARRAY_SIZE] = { 0 };
const int resolutions[5][2] = {{1024,576}, {1280,720}, {1366, 768}, {1600,900}, {1920,1080} };
char text[4][10] = { 0 };
char savedGames[10][20] = { 0 };

Player player[4] = { 0 };
Vector2 offset = { 0 };

// core functions
void newGame(); 
void initGame();
void drawMap();
void play();
void AI();
void randomPlay();
bool move(int x1, int y1, int x2, int y2, bool showPossibleMotion);
bool putWall(int xWall, int yWall);
bool checkWall(int xWall, int yWall, int playerCode);
int checkFinish();
void findPath(int xCur,int yCur,int xDest,int yDest,bool *isThereAnyWay);
void turn();

// non-core functions
void delay(float time);
void undo();
void sound(int actionCode);
void settingsFile(int actionCode);
void saveGame(int actionCode, char name[]);
void copy(int actionCode, int arr[][MAX_ARRAY_SIZE]);
void initArray(int arr[][MAX_ARRAY_SIZE]);
bool isAnyGameStarted();
void swap(int *a, int *b);

// menu
void menu();
void startNewGame();
void loadGame();
void settings();

// GUI Controls
void DrawDialog(int *value, Rectangle rec, int unit, int min, int max);
bool DrawButton(char *text, Rectangle rec, int fontSize, int thickness, bool centerX, bool centerY, Color color1, Color color2);
void DrawInputBox(char text[], Rectangle textBox,int fontSize, int maxInput, Color textColor,Color bgColor, Color borderColor);
bool IsAnyKeyPressed();

int main() {
    newGame();
}

void newGame() {
    //sound initialization
    sound(1);
    
    //load saved settings
    settingsFile(2); 
    
    // main loop
    while(true) {
        static bool isFullScreenTemp = false;
        if(isFullScreenTemp) {
            ToggleFullscreen();
            isFullScreenTemp = false;
        }
        InitWindow(menuWidth,menuHeight,"menu");
        SetTargetFPS(60);
        
        // menu loop
        while(true) {
            if(WindowShouldClose()) return;
            BeginDrawing();
            ClearBackground(RAYWHITE);
            if(pageCode == 0) menu(); // menu page
            else if (pageCode == 1) startNewGame(); // start a new game
            else if (pageCode == 2) loadGame(); // manage saved games page
            else if (pageCode == 3) settings(); // settings page
            else if (pageCode == 4) return; //exit
            else {
                CloseWindow();
                break;
            }    
            EndDrawing();
        } // end menu loop
        
        if(pageCode != 6) initGame();
        
        if(isFullScreen && !isFullScreenTemp) {
            ToggleFullscreen();
            isFullScreenTemp = true;
        }
        
        InitWindow(width, height, "Quoridor");
        SetTargetFPS(60);
        
        // game loop
        while(true) {
            if(WindowShouldClose()) return;
            static int winner = 0;
            static bool isSavePage = false;
            static char savedGameName[20] = { 0 };
            if(winner == 0) {
                if(isSavePage) {
                    BeginDrawing();
                    ClearBackground(RAYWHITE);
                    
                    DrawRectangleLinesEx( (Rectangle) {width/2 - 205, height/4 - 20, 410, 190}, 7, LIGHTGRAY);
                    DrawInputBox(savedGameName, (Rectangle) {width/2 - 185, height/4, 370, 70}, 40, 19, BLACK, LIGHTGRAY, BLACK);
                    
                    if(DrawButton("Cancel", (Rectangle) {width/2 - 185, height/4 + 80, 180, 70}, 30,4, true, true, BLACK, MAROON)) isSavePage = false;
                    if(DrawButton("Save", (Rectangle) {width/2 + 5, height/4 + 80, 180, 70}, 30,4, true, true, BLACK, MAROON)) {
                        saveGame(1, savedGameName);
                        isSavePage = false;
                    }
                    EndDrawing();
                } else {
                    BeginDrawing();
                    ClearBackground(RAYWHITE);
                    if(DrawButton("menu", (Rectangle) {offset.x/2 - 55 , height/2 - 60, 110, 50}, 25, 3, true, true, BLACK, DARKGRAY)) {
                        pageCode = 0;
                        CloseWindow();
                        break;
                    }
                    if(DrawButton("save", (Rectangle) {offset.x/2 - 55 , height/2, 110, 50}, 25, 3, true, true, BLACK, DARKGRAY)) isSavePage = true;
                    if(DrawButton("undo", (Rectangle) {offset.x/2 - 55 , height/2 + 60, 110, 50}, 25, 3, true, true, BLACK, DARKGRAY)) undo();
                    if(DrawButton("sound", (Rectangle) {offset.x/2 - 55 , height/2 + 120, 110, 50}, 25, 3, true, true, isSoundOn?BLACK:LIGHTGRAY, DARKGRAY)) {
                        isSoundOn = isSoundOn?false:true;
                    }
                    drawMap();
                    EndDrawing();
                    play();
                    winner = checkFinish();
                }
            } else { // there is a winner
                BeginDrawing();
                ClearBackground(RAYWHITE);
                if(DrawButton("menu", (Rectangle) {width/2 - 50,height/2 - 150 , 120, 60}, 30, 5, true, true, BLACK, MAROON)) {
                    pageCode = 0;
                    winner = 0;
                    CloseWindow();
                    break;
                }
                DrawButton((char*) FormatText("Player %d Wins!",winner), (Rectangle) {0,0,width,height}, 50, 0, true, true, player[winner-1].color,player[winner-1].color);
                EndDrawing();
            }
        } // end game loop
    } // end main loop
}

void initGame() {
    if(pageCode != 5) settingsFile(2);
    
    //size and window initialization
    arraySize = 2*uiSize + 1;
    width = resolutions[resolution-1][0];
    height = resolutions[resolution-1][1];
    offset.x = width / 2 - squareSize * ((float) uiSize / 2);
    offset.y = height / 2 - squareSize * ((float) uiSize / 2);
            
    //color initialization
    player[0].color = MAROON;
    player[1].color = ORANGE;
    player[2].color = BLUE;
    player[3].color = GREEN;
    
    if(pageCode != 5) {
        //arrays initialization
        initArray(map);
        initArray(mapCopy);
        initArray(mapUndo);
        
        //player position initialization
        map[arraySize-2][arraySize/2] = 1;
        map[1][arraySize/2] = 2;
        if(!isDuo) {
            map[arraySize/2][arraySize-2] = 3;        
            map[arraySize/2][1] = 4;
        }
        
        //player wallCount initialization
        for(int i = 0; i < 4; i++) player[i].wallCount = wallCount;
    }
}

void drawMap(void) {   
    //draw turn
    DrawButton((char*)FormatText("Player %d",turnPlayer + 1),
    (Rectangle) {offset.x/2 - 55, height/2 + 180, 110, 50},
    20, 3, true, true,
    player[turnPlayer].color,
    player[turnPlayer].color);

    //draw rest wall number for each player
    for(int i = 0; i < 4; i++) {
        if(player[i].mode != 0) {
            DrawCircle(offset.x/2 - 40,height/2 + i * 30 - 220,10,player[i].color);
            DrawRectangle(offset.x/2 - 20, height/2 + i * 30 - 222.5, 25, 5, BROWN);
            DrawText(FormatText("x %d", player[i].wallCount), offset.x/2 + 12, height/2 + i * 30 - 230, 20, BLACK);
        }
    }
    
    //finding the position of players
    for(int j = 1; j < arraySize - 1; j += 2) {
        for(int i = 1; i < arraySize - 1; i += 2) {
            if(map[j][i] >= 1 && map[j][i] <= 4) {
                player[map[j][i]-1].boardPos.x = (i+1)/2;
                player[map[j][i]-1].boardPos.y = (j+1)/2;
            }
        }
    }        
    
    //drawing players
    for(int i = 0; i < 4; i++) {
        player[i].realPos.x = (offset.x + squareSize * (player[i].boardPos.x - 0.5));
        player[i].realPos.y = (offset.y + squareSize * (player[i].boardPos.y - 0.5));
        if(player[i].mode != 0) DrawCircleV((Vector2) player[i].realPos , squareSize/2 - thickness, player[i].color);
    }
    
    //drawing map
    for (int i = 0; i <= uiSize ; i++) {
        // horizontal lines
        DrawLineEx(
        (Vector2){offset.x - thickness/2, squareSize*i + offset.y},
        (Vector2){width - offset.x + thickness/2, squareSize*i + offset.y},
        thickness,
        LIGHTGRAY);
        
        // vertical lines
        DrawLineEx(
        (Vector2){offset.x + squareSize*i, offset.y},
        (Vector2){offset.x + squareSize*i, height - offset.y},
        thickness,
        LIGHTGRAY);
    }
    
    //drawing temp walls
    for(int j = 1; j < uiSize; j++) {
        for(int i = 1; i < uiSize; i++) {
            //horizontal temp walls
            if(CheckCollisionPointRec((Vector2) GetMousePosition(),
            (Rectangle) {(offset.x + thickness/2) + squareSize*(i-1),(offset.y - thickness/2) + squareSize*j,
            squareSize - thickness, thickness})
            && checkWall(2*i-1, 2*j, 0) && checkWall(2*i-1, 2*j, 1)
            && checkWall(2*i-1, 2*j, 2) && checkWall(2*i-1, 2*j, 3)
            && map[2*j][2*i-1] == 0 && map[2*j][2*i] == 0 && map[2*j][2*i+1] == 0) {
                DrawRectangleRec((Rectangle){offset.x + thickness/2 + squareSize*(i-1), offset.y - thickness/2 + squareSize*j, 2*squareSize - thickness, thickness}, BEIGE);
            }
            //vertical temp walls
            if(CheckCollisionPointRec((Vector2) GetMousePosition(),
            (Rectangle) {(offset.x - thickness/2) + squareSize*i,(offset.y + thickness/2) + squareSize*(j-1),
            thickness, squareSize - thickness}) 
            && checkWall(2*i, 2*j-1, 0) && checkWall(2*i, 2*j-1, 1)
            && checkWall(2*i, 2*j-1, 2) && checkWall(2*i, 2*j-1, 3)
            && map[2*j-1][2*i] == 0 && map[2*j][2*i] == 0 && map[2*j+1][2*i] == 0) {
                DrawRectangleRec((Rectangle){(offset.x - thickness/2) + squareSize*i,(offset.y + thickness/2) + squareSize*(j-1), thickness, 2*squareSize - thickness}, BEIGE);
            }
        }
    }
    
    //drawing main Horizontal walls
    for(int j = 2; j <= arraySize-3; j += 2) { // 2,16
        for(int i = 1; i <= arraySize-4; i += 2) { // 1,15
            if(map[j][i] == 8 && map[j][i+1] == 8 && map[j][i+2] == 8) {
                DrawRectangle(
                offset.x + squareSize*((int)(i/2)) + thickness/2,
                offset.y + squareSize*((int)(j/2)) - thickness/2,
                2*squareSize - thickness, 
                thickness,
                BROWN
                );
            }
        }
    }
    
    //drawing main Vertical walls
    for(int i = 2; i <= arraySize - 3; i += 2) { // 2,16
        for(int j = 1; j <= arraySize - 4; j += 2) { // 1,15
            if(map[j][i] == 8 && map[j+1][i] == 8 && map[j+2][i] == 8) {
                DrawRectangle(
                offset.x + squareSize*((int)(i/2)) - thickness/2,
                offset.y + squareSize*((int)(j/2)) + thickness/2,
                thickness,
                2*squareSize - thickness,
                BROWN
                );
            }
        }
    }
    
    //draw places that the players can go
    for(int j = -2; j <= 2; j++) {
        for(int i = -2; i <= 2; i++) {
            if(move(player[turnPlayer].boardPos.x, player[turnPlayer].boardPos.y,
            player[turnPlayer].boardPos.x + i, player[turnPlayer].boardPos.y + j, true) && player[turnPlayer].mode == 1) {
                DrawCircle(player[turnPlayer].realPos.x + i * squareSize, player[turnPlayer].realPos.y + j * squareSize,
                squareSize/2 - thickness, (Color) {200,200,200,100});
            }
        }
    }
}

void play() {
    static int x2, y2;
    
    // put wall
    for(int j = 1; j < uiSize; j++) {
        for(int i = 1; i < uiSize; i++) {
            //horizontal wall
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec((Vector2) GetMousePosition(),
            (Rectangle) {(offset.x + thickness/2) + squareSize*(i-1),(offset.y - thickness/2) + squareSize*j,
            squareSize - thickness, thickness})) {
                putWall(2*i-1,2*j);
                return;
            }
            //vertical wall
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec((Vector2) GetMousePosition(),
            (Rectangle) {(offset.x - thickness/2) + squareSize*i,(offset.y + thickness/2) + squareSize*(j-1),
            thickness, squareSize - thickness})) {
                putWall(2*i,2*j-1);
                return;
            }
        }
    }
    
    x2 = ((GetMouseX() - offset.x) / squareSize) + 1;
    y2 = ((GetMouseY() - offset.y) / squareSize) + 1;
    
    // move
    if(player[turnPlayer].mode == 2) randomPlay();
    else if(player[turnPlayer].mode == 3) AI();
    else if(GetMouseX() > offset.x
    && GetMouseX() < width - offset.x
    && GetMouseY() > offset.y
    && GetMouseY() < height - offset.y
    && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        move((int) player[turnPlayer].boardPos.x, (int) player[turnPlayer].boardPos.y, x2, y2, false);
    }
}

bool move(int x1, int y1, int x2, int y2, bool showPossibleMotion) {
    // (x1,y1) first pos | (x2,y2) second pos | these coordinates are based on the board
    // xMiddle and yMiddle are places between those two coordinates
    static int xMiddle, yMiddle, x1Array ,y1Array ,x2Array ,y2Array ,deltaX ,deltaY;
    
    x1Array = 2*x1 - 1;
    y1Array = 2*y1 - 1; 
    x2Array = 2*x2 - 1;
    y2Array = 2*y2 - 1;
    
    deltaX = x2 - x1;
    deltaY = y2 - y1;
    
    xMiddle = (x1Array + x2Array) / 2;
    yMiddle = (y1Array + y2Array) / 2;

    if( ( (deltaX == 1 && deltaY == 0)
        || (deltaX == -1 && deltaY == 0)
        || (deltaX == 0 && deltaY == 1)
        || (deltaX == 0 && deltaY == -1) )
        && ( (deltaY != 0 && map[yMiddle][x1Array] == 0)
        || (deltaX != 0 && map[y1Array][xMiddle] == 0) )
        && map[y2Array][x2Array] == 7);
    else if ( ((deltaX == 2 && deltaY == 0) || (deltaX == -2 && deltaY == 0)
        || (deltaX == 0 && deltaY == 2) || (deltaX == 0 && deltaY == -2))
        && map[y1Array + deltaY/2][x1Array + deltaX/2] == 0
        && map[y1Array + deltaY][x1Array + deltaX] != 7
        && map[y1Array + 3*(deltaY/2)][x1Array + 3*(deltaX/2)] == 0
        && isDuo);
    else if (isDuo && (deltaX == 1 || deltaX == -1) && (deltaY == 1 || deltaY == -1)
        && ( (map[y1Array][x1Array+deltaX] != 8 && map[y1Array][x1Array+2*deltaX] != 7 && map[y1Array][x1Array+3*deltaX] == 8)
        || (map[y1Array+deltaY][x1Array] != 8 && map[y1Array+2*deltaY][x1Array] != 7 && map[y1Array+3*deltaY][x1Array] == 8) )
        && ( (player[0].boardPos.x == player[1].boardPos.x && map[y2Array][(x1Array + x2Array)/2] == 0)
        || (player[0].boardPos.y == player[1].boardPos.y && map[(y1Array + y2Array)/2][x2Array] == 0) ) );
    else return false;
    
    if(showPossibleMotion) return true;
    copy(1,mapUndo);
    swap(&map[y2Array][x2Array] , &map[y1Array][x1Array]);
    turn();
    sound(3);
    return true;
} 

bool putWall(int xWall, int yWall) {
    if(player[turnPlayer].wallCount == 0) return false;
    for(int i = 0; i < 4; i++) if(checkWall(xWall, yWall, i) == false) return false;
     
    if(map[yWall][xWall] == 0
        && map[yWall][xWall + 1] == 0
        && map[yWall][xWall + 2] == 0 ) {
        copy(1,mapUndo);
        for(int i = 0; i < 3; i++) map[yWall][xWall+i] = 8;
    } else if (map[yWall][xWall] == 0
        && map[yWall + 1][xWall] == 0
        && map[yWall + 2][xWall] == 0 ) {
        copy(1,mapUndo);
        for(int i = 0; i < 3; i++) map[yWall+i][xWall] = 8;                 
    } else return false;
    player[turnPlayer].wallCount -= 1;   
    turn();
    sound(4);
    return true;
}

void findPath(int xCur,int yCur,int xDest,int yDest,bool *isThereAnyWay) {
    mapCopy[yCur][xCur] = 6;
    
    if(xCur == xDest || yCur == yDest) {
        (*isThereAnyWay) = true;
        return;
    }
    
    if(isDuo) {
        if(mapCopy[yCur][xCur + 1] != 8 && (mapCopy[yCur][xCur + 2] == 7 || mapCopy[yCur][xCur + 2] == 1 || mapCopy[yCur][xCur + 2] == 2)) { //right
            findPath(xCur+2,yCur,xDest,yDest,isThereAnyWay);
        }
        if(mapCopy[yCur + 1][xCur] != 8 && (mapCopy[yCur + 2][xCur] == 7 || mapCopy[yCur + 2][xCur] == 1 || mapCopy[yCur + 2][xCur] == 2)) { //down
            findPath(xCur,yCur+2,xDest,yDest,isThereAnyWay);
        }
        if(mapCopy[yCur][xCur - 1] != 8 && (mapCopy[yCur][xCur - 2] == 7 || mapCopy[yCur][xCur - 2] == 1 || mapCopy[yCur][xCur - 2] == 2)) { //left
            findPath(xCur-2,yCur,xDest,yDest,isThereAnyWay);
        }
        if(mapCopy[yCur - 1][xCur] != 8 && (mapCopy[yCur - 2][xCur] == 7 || mapCopy[yCur - 2][xCur] == 1 || mapCopy[yCur - 2][xCur] == 2)) { //up
            findPath(xCur,yCur-2,xDest,yDest,isThereAnyWay);
        }
    } else {
        if(mapCopy[yCur][xCur + 1] == 0 && mapCopy[yCur][xCur + 2] == 7) { //right
            findPath(xCur+2,yCur,xDest,yDest,isThereAnyWay);
        }
        if(mapCopy[yCur + 1][xCur] == 0 && mapCopy[yCur + 2][xCur] == 7) { //down
            findPath(xCur,yCur+2,xDest,yDest,isThereAnyWay);
        }
        if(mapCopy[yCur][xCur - 1] == 0 && mapCopy[yCur][xCur - 2] == 7) { //left
            findPath(xCur-2,yCur,xDest,yDest,isThereAnyWay);
        }
        if(mapCopy[yCur - 1][xCur] == 0 && mapCopy[yCur - 2][xCur] == 7) { //up
            findPath(xCur,yCur-2,xDest,yDest,isThereAnyWay);
        }
    }
    
}

bool checkWall(int xWall, int yWall, int playerCode) {
    static int xDest, yDest, xStart, yStart;
    static bool isThereAnyWay;
    xDest = -1, yDest = -1;
    
    if(playerCode >= 2 && isDuo) return true;
    
    xStart = 2 * player[playerCode].boardPos.x - 1;
    yStart = 2 * player[playerCode].boardPos.y - 1;
    
    if (playerCode == 0) yDest = 1;
    else if(playerCode == 1) yDest = arraySize - 2;
    else if (playerCode == 2) xDest = 1;
    else xDest = arraySize - 2;
    
    copy(1,mapCopy);  //copy map to mapCopy
    
    if(map[yWall][xWall] == 0
        && map[yWall][xWall + 1] == 0
        && map[yWall][xWall + 2] == 0 ) {
        for(int i = xWall; i < xWall + 3; i++) {
            mapCopy[yWall][i] = 8;
        }
    } else if (map[yWall][xWall] == 0
        && map[yWall + 1][xWall] == 0
        && map[yWall + 2][xWall] == 0 ) {
        for(int i = yWall; i < yWall + 3; i++) {
            mapCopy[i][xWall] = 8;
        }
    }
    isThereAnyWay = false;
    findPath(xStart, yStart, xDest, yDest, &isThereAnyWay);
    return isThereAnyWay;
}

int checkFinish() {
    for(int i = 1; i <= arraySize - 2; i += 2) {
        if(map[1][i] == 1) return 1;
    } 
    for(int i = 1; i <= arraySize - 2; i += 2) {
        if(map[arraySize - 2][i] == 2) return 2;
    }
    for(int i = 1; i <= arraySize - 2; i += 2) {
        if(map[i][1] == 3) return 3;
    }
    for(int i = 1; i <= arraySize - 2; i += 2) {
        if(map[i][arraySize - 2] == 4) return 4;
    }
    return 0;
}

void sound(int actionCode) {
    static Sound move;
    static Sound wall;
    if(!isSoundOn) return;
    if(actionCode == 1) {
        InitAudioDevice();
        move = LoadSound("move.mp3");
        wall = LoadSound("wall.mp3");
    } else if (actionCode == 3) {
        PlaySound(move);
    } else if (actionCode == 4) {
        PlaySound(wall);
    } else if (actionCode == 5) {
        UnloadSound(move);
        UnloadSound(wall);
        CloseAudioDevice();
    }
}

void undo(void) {
    static int lastWallCount[4] = { 0 };
    static bool sw1;
    static bool sw2 = true;
    sw1 = false;
    
    if(sw2) {
        for(int i = 0; i < 4; i++) lastWallCount[i] = wallCount;
        sw2 = false;
    }
    
    for(int j = 0; j < arraySize; j++) {
        for(int i = 0; i < arraySize; i++) {
            if(mapUndo[j][i] != map[j][i]) {
                sw1 = true;
                break;
            }
            if(sw1) break;
        }
    }
    if(sw1) {
        copy(2,mapUndo);
        turnPlayer--;
        if(turnPlayer == -1) {
            if(isDuo) turnPlayer = 1;
            else turnPlayer = 3;
        }
    }
    if(lastWallCount[turnPlayer] > player[turnPlayer].wallCount) (player[turnPlayer].wallCount)++;
    for(int i = 0; i < 4; i++) lastWallCount[i] = player[i].wallCount;
}

void AI() {
    static int chosen, xCur, yCur;
    static bool result;
    
    xCur = player[turnPlayer].boardPos.x;
    yCur = player[turnPlayer].boardPos.y;
    
    delay(1);
    
    result = false;
    while(!result) {
        chosen = GetRandomValue(1,5);
        if(chosen != 5) { //move
            while(!result) {
                static int dir;
                do dir = GetRandomValue(-1,1);   
                while(!dir);
                if(turnPlayer == 0) {
                    do dir = GetRandomValue(-2,2);   
                    while(!dir);
                    result = move(xCur, yCur, xCur, yCur - 2, false);
                    if(!result) result = move(xCur, yCur, xCur, yCur - 1, false);
                    if(!result) result = move(xCur, yCur, xCur + dir, yCur, false);
                    do dir = GetRandomValue(-1,1);   
                    while(!dir);
                    if(!result) result = move(xCur, yCur, xCur + dir, yCur, false);
                    if(!result) result = move(xCur, yCur, xCur, yCur + 1, false);
                    if(!result) result = move(xCur, yCur, xCur, yCur + 2, false);
                } else if(turnPlayer == 1) {
                    do dir = GetRandomValue(-2,2);   
                    while(!dir);
                    result = move(xCur, yCur, xCur, yCur + 2, false);
                    if(!result) result = move(xCur, yCur, xCur, yCur + 1, false);
                    if(!result) result = move(xCur, yCur, xCur + dir, yCur, false);
                    do dir = GetRandomValue(-1,1);   
                    while(!dir);
                    if(!result) result = move(xCur, yCur, xCur + dir, yCur, false);
                    if(!result) result = move(xCur, yCur, xCur, yCur - 1, false);
                    if(!result) result = move(xCur, yCur, xCur, yCur - 2, false);
                } else if(turnPlayer == 2) {
                    do dir = GetRandomValue(-1,1);   
                    while(!dir);
                    result = move(xCur, yCur, xCur - 1, yCur, false);
                    if(!result) result = move(xCur, yCur, xCur, yCur + dir, false);
                    if(!result) result = move(xCur, yCur, xCur + 1, yCur, false);
                } else {
                    do dir = GetRandomValue(-1,1);   
                    while(!dir);
                    result = move(xCur, yCur, xCur + 1, yCur, false);
                    if(!result) result = move(xCur, yCur, xCur, yCur + dir, false);
                    if(!result) result = move(xCur, yCur, xCur - 1, yCur, false);
                }                              
            }
        } else { //put wall
            static int playerCode;
            playerCode = GetRandomValue(0,3);
            if(playerCode == 0 && turnPlayer != 0) {
                result = putWall(2*(player[0].boardPos.x - 1) + 1,2 * (player[0].boardPos.y - 1));
                if(!result) result = putWall(2*(player[0].boardPos.x - 2) + 1,2*(player[0].boardPos.y - 1));                        
            } else if(playerCode == 1 && turnPlayer != 1) {
                result = putWall(2*(player[1].boardPos.x - 1) + 1,2*player[1].boardPos.y);
                if(result == false) result = putWall(2*(player[1].boardPos.x - 2) + 1,2*player[1].boardPos.y);   
            } else if (playerCode ==  2 && turnPlayer != 2 && !isDuo){
                result = putWall(2 * (player[2].boardPos.x - 1),2*(player[2].boardPos.y - 1) + 1);
                if(!result) result = putWall(2*(player[2].boardPos.x - 1),2*(player[2].boardPos.y - 2) + 1);
            } else if(turnPlayer != 3 && turnPlayer != 3 && !isDuo) {
                result = putWall(2*player[3].boardPos.x ,2*(player[3].boardPos.y - 1) + 1);
                if(!result) result = putWall(2*player[3].boardPos.x ,2*(player[3].boardPos.y - 2) + 1);   
            }
            if(result) return;
            if(playerCode == 0 && turnPlayer != 0) {
                result = putWall(2 * (player[0].boardPos.x - 1),2*(player[0].boardPos.y - 1) + 1);
                if(!result) result = putWall(2*(player[0].boardPos.x - 1),2*(player[0].boardPos.y - 2) + 1);
                if(!result) result = putWall(2*player[0].boardPos.x ,2*(player[0].boardPos.y - 1) + 1);
                if(!result) result = putWall(2*player[0].boardPos.x ,2*(player[0].boardPos.y - 2) + 1);
            } else if (playerCode ==  1 && turnPlayer != 1){    
                result = putWall(2 * (player[1].boardPos.x - 1),2*(player[1].boardPos.y - 1) + 1);
                if(!result) result = putWall(2*(player[1].boardPos.x - 1),2*(player[1].boardPos.y - 2) + 1);
                if(!result) result = putWall(2*player[1].boardPos.x ,2*(player[1].boardPos.y - 1) + 1);
                if(!result) result = putWall(2*player[1].boardPos.x ,2*(player[1].boardPos.y - 2) + 1);
            } else if(playerCode == 2 && turnPlayer != 2 && !isDuo){
                result = putWall(2*(player[2].boardPos.x - 1) + 1,2 * (player[2].boardPos.y - 1));
                if(!result) result = putWall(2*(player[2].boardPos.x - 2) + 1,2*(player[2].boardPos.y - 1));
                if(!result) result = putWall(2*(player[2].boardPos.x - 1) + 1,2*player[2].boardPos.y);
                if(!result) result = putWall(2*(player[2].boardPos.x - 2) + 1,2*player[2].boardPos.y); 
            } else if(playerCode == 3 && turnPlayer != 3 && !isDuo) {
                result = putWall(2*(player[3].boardPos.x - 1) + 1,2 * (player[3].boardPos.y - 1));
                if(!result) result = putWall(2*(player[3].boardPos.x - 2) + 1,2*(player[3].boardPos.y - 1));
                if(!result) result = putWall(2*(player[3].boardPos.x - 1) + 1,2*player[3].boardPos.y);
                if(!result) result = putWall(2*(player[3].boardPos.x - 2) + 1,2*player[3].boardPos.y); 
            }
        }
    }
}

void randomPlay() {
    static int chosen, x,y, xCur, yCur;
    static bool result;
    
    xCur = player[turnPlayer].boardPos.x;
    yCur = player[turnPlayer].boardPos.y;
    
    delay(1);
    
    result = false;
    while(!result) {
        chosen = GetRandomValue(1,5);
        if(chosen != 5) { //move
            while(!result) {
                x = GetRandomValue(-2,2);
                y = GetRandomValue(-2,2);
                result = move(xCur, yCur, xCur + x, yCur + y, false);                
            }
        } else {
            x = GetRandomValue(0,(arraySize-4)/2);
            y = GetRandomValue(0,(arraySize-4)/2);
            chosen = GetRandomValue(1,2);
            if(chosen == 1) result = putWall(2*x + 1,2 * (y + 1));
            else result = putWall(2 * (x + 1),2*y + 1);
        }        
    }
}


void settingsFile(int actionCode) {
    static FILE *settingsFile;
    if(actionCode == 1) { // save settings
        settingsFile = fopen("settings.dat", "wb");
        fwrite(&resolution, sizeof(int), 1, settingsFile);
        fwrite(&uiSize, sizeof(int), 1, settingsFile);
        fwrite(&squareSize, sizeof(int), 1, settingsFile);
        fwrite(&thickness, sizeof(int), 1, settingsFile);
        fwrite(&wallCount, sizeof(int), 1, settingsFile);
        fwrite(&isFullScreen, sizeof(bool), 1, settingsFile);
    } else if(actionCode == 2) { // load settings
        settingsFile = fopen("settings.dat", "rb");
        fread(&resolution, sizeof(int), 1, settingsFile);
        fread(&uiSize, sizeof(int), 1, settingsFile);
        fread(&squareSize, sizeof(int), 1, settingsFile);
        fread(&thickness, sizeof(int), 1, settingsFile);
        fread(&wallCount, sizeof(int), 1, settingsFile);
        fread(&isFullScreen, sizeof(bool), 1, settingsFile);
    }
    fclose(settingsFile);
}

void menu() {
    static int distance1, distance2;
    
    DrawButton("Quoridor", (Rectangle) {menuWidth/2 - 300, 90, 600, 80}, 40, 5, true, true, BLACK, BLACK);
    
    if(isAnyGameStarted()) {
        if(DrawButton("Continue", (Rectangle) {menuWidth/2 - 150 , 260, 300, 70}, 30, 5, true, true, BLACK, MAROON)) pageCode = 6;
        distance1 = 0;
    } else distance1 = 85;
    
    if(DrawButton("New Game", (Rectangle) {menuWidth/2 - 150 , 345 - distance1, 300, 70}, 30, 5, true, true, BLACK, MAROON)) pageCode = 1;
    if(DrawButton("Load Game", (Rectangle) {menuWidth/2 - 150 , 430 - distance1, 300, 70}, 30, 5, true, true, BLACK, MAROON)) pageCode = 2;
    
    if(!isAnyGameStarted()) {
        if(DrawButton("Settings", (Rectangle) {menuWidth/2 - 150 , 515 - distance1, 300, 70}, 30, 5, true, true, BLACK, MAROON)) pageCode = 3;
        distance2 = 0;
    } else distance2 = 85;
    
    if(DrawButton("Exit", (Rectangle) {menuWidth/2 - 150 , 600 - distance1 - distance2 , 300, 70}, 30, 5, true, true, BLACK, MAROON)) pageCode = 4;   
}

void startNewGame() {
    static int tempMode[4] = { 0 };
    DrawButton("Player Mode", (Rectangle) {menuWidth/2 - 300, 90, 600, 80}, 40, 5, true, true, BLACK, BLACK);
    if(DrawButton(text[0], (Rectangle) {menuWidth/2 - 150 , 180 + 80, 150, 70}, 30, 5, true, true, BLACK, MAROON)) {
        tempMode[0]++;
        if(tempMode[0] == 4) tempMode[0] = 0;
    }
    if(DrawButton(text[1], (Rectangle) {menuWidth/2 + 10 , 180 + 80, 150, 70}, 30, 5, true, true, BLACK, MAROON)){
        tempMode[1]++;
        if(tempMode[1] == 4) tempMode[1] = 0;
    }
    if(DrawButton(text[2], (Rectangle) {menuWidth/2 - 150  , 345, 150, 70}, 30, 5, true, true, BLACK, MAROON)){
        tempMode[2]++;
        if(tempMode[2] == 4) tempMode[2] = 0;
    }
    if(DrawButton(text[3], (Rectangle) {menuWidth/2 + 10  , 345, 150, 70}, 30, 5, true, true, BLACK, MAROON)){
        tempMode[3]++;
        if(tempMode[3] == 4) tempMode[3] = 0;
    }
    if(DrawButton("Back", (Rectangle) {menuWidth/2 - 150 , 500, 150, 70}, 30, 5, true, true, BLACK, MAROON)) pageCode = 0;
    if(DrawButton("Play", (Rectangle) {menuWidth/2 + 10 ,  500, 150, 70}, 30, 5, true, true, BLACK, MAROON)) {
        if( (tempMode[0] != 0 && tempMode[1] != 0 && tempMode[2] == 0 && tempMode[3] == 0)
        || (tempMode[0] != 0 && tempMode[1] != 0 && tempMode[2] != 0 && tempMode[3] != 0) ) {
            for(int i = 0; i < 4; i++) player[i].mode = tempMode[i];
            turnPlayer = 0;
            isDuo = (player[3].mode == 0);
            pageCode = 7;
        }
    }
    
    for(int i = 0; i < 4; i++) {
        if(tempMode[i] == 0) strcpy(text[i], "None");
        else if(tempMode[i] == 1) strcpy(text[i],"Human");
        else if(tempMode[i] == 2) strcpy(text[i], "Random");
        else strcpy(text[i], "AI");
    }
}

void saveGame(int actionCode, char name[]) {
    static const int size = sizeof(savedGames) / sizeof(savedGames[0]);
    FILE *savedFile = NULL;
    
    if(actionCode == 1) { //save current game
        remove(FormatText("%s.dat", savedGames[0]));
        if(savedGamesCounter >= size) {
            for(int i = 1; i < size; i++) strcpy(savedGames[i-1],savedGames[i]);
            strcpy(savedGames[savedGamesCounter-1], name);        
        } else {
            strcpy(savedGames[savedGamesCounter], name);
            savedGamesCounter++;
        }
        savedFile = fopen(FormatText("%s.dat", name), "wb");
        fwrite(map, sizeof(int), MAX_ARRAY_SIZE*MAX_ARRAY_SIZE, savedFile);
        fwrite(mapUndo, sizeof(int), MAX_ARRAY_SIZE*MAX_ARRAY_SIZE, savedFile);
        fwrite(player, sizeof(Player), 4, savedFile);
        fwrite(&wallCount, sizeof(int), 1, savedFile);
        fwrite(&turnPlayer,sizeof(int), 1, savedFile);    
        fwrite(&squareSize,sizeof(int), 1, savedFile);   
        fwrite(&thickness,sizeof(int), 1, savedFile);   
        fwrite(&uiSize,sizeof(int), 1, savedFile);   
        fwrite(&resolution,sizeof(int), 1, savedFile);   
        fwrite(&isFullScreen,sizeof(bool), 1, savedFile);
        fclose(savedFile);
        
        FILE *savedListFile = fopen("savedListFile.dat", "wb");
        fwrite(&savedGamesCounter, sizeof(int), 1, savedListFile);
        fwrite(savedGames, sizeof(char), 200, savedListFile);
        fclose(savedListFile);
    } else if(actionCode == 2) { //load wanted game from saved games list
        savedFile = fopen(FormatText("%s.dat", name), "rb");
        fread(map, sizeof(int), MAX_ARRAY_SIZE*MAX_ARRAY_SIZE, savedFile);
        fread(mapUndo, sizeof(int), MAX_ARRAY_SIZE*MAX_ARRAY_SIZE, savedFile);
        fread(player, sizeof(Player), 4, savedFile);
        fread(&wallCount, sizeof(int), 1, savedFile);
        fread(&turnPlayer,sizeof(int), 1, savedFile);    
        fread(&squareSize,sizeof(int), 1, savedFile);   
        fread(&thickness,sizeof(int), 1, savedFile);   
        fread(&uiSize,sizeof(int), 1, savedFile);   
        fread(&resolution,sizeof(int), 1, savedFile);   
        fread(&isFullScreen,sizeof(bool), 1, savedFile);   
        fclose(savedFile);
    }
}

void loadGame() {
    //load list of saved games
    static FILE *savedListFile = NULL;
    if((savedListFile = fopen("savedListFile.dat", "rb")) != NULL) {
        fread(&savedGamesCounter, sizeof(int), 1, savedListFile);
        fread(savedGames, sizeof(char), 200, savedListFile);
        fclose(savedListFile);
    }
    
    if (savedGamesCounter == 0) DrawText("No Game Saved!", menuWidth / 2 - MeasureText("No game saved!",30)/2, menuHeight/2-40, 30, BLACK);
    else {
        for(int i = 0; i < savedGamesCounter; i++) {
            // open a seved game
            if(DrawButton(savedGames[i], (Rectangle) {75, 40 + 55*i, menuWidth - 200, 50}, 28, 3,true, true, BLACK, DARKGRAY)) {
                saveGame(2,savedGames[i]);
                pageCode = 5;
            }
            // remove a saved game
            if(DrawButton("x",(Rectangle) {menuWidth - 120, 40 + 55*i, 50, 50}, 40, 3, true, true, MAROON, DARKGRAY)) {
                remove(FormatText("%s.dat",savedGames[i]));
                for(int j = i+1; j < savedGamesCounter; j++) strcpy(savedGames[j-1],savedGames[j]);
                savedGamesCounter--;
                savedListFile = fopen("savedListFile.dat", "wb");
                fwrite(&savedGamesCounter, sizeof(int), 1, savedListFile);
                fwrite(savedGames, sizeof(char), 200, savedListFile);
                fclose(savedListFile);
            }
        }
    }
    if(DrawButton("Back", (Rectangle) {menuWidth/2 - 75 , 610, 150, 70}, 30, 5,true, true, BLACK, MAROON)) pageCode = 0;
}

void settings() {
    DrawDialog(&uiSize, (Rectangle) {280, 80, 155, 60}, 2, 5, MAX_UI_SIZE);
    DrawDialog(&thickness, (Rectangle) {280, 160, 155, 60}, 1, 5, 20);
    DrawDialog(&squareSize, (Rectangle) {280, 240, 155, 60}, 5, 20, 95);
    DrawDialog(&wallCount, (Rectangle) {280, 320, 155, 60}, 1, 1, 20);
    DrawDialog(&resolution, (Rectangle) {280, 400, 155, 60}, 1,1,5);
    
    if(resolution == 5) isFullScreen = true;
    
    if(isFullScreen) {
        if(DrawButton("YES", (Rectangle) {280, 480, 155, 60}, 30, 5, true, true, BLACK, MAROON)) isFullScreen = false;
    } else {
        if(DrawButton("NO", (Rectangle) {280, 480, 155, 60}, 30, 5, true, true, BLACK, MAROON)) isFullScreen = true;
    }
    
    DrawText(FormatText("Size (%ix%i)",uiSize,uiSize), 60, 95, 30, BLACK);
    DrawText("Thickness", 60, 175, 30, BLACK);
    DrawText("SquareSize", 60, 255, 30, BLACK);
    DrawText("WallCount", 60, 335, 30, BLACK);
    DrawText(FormatText("%ix%i",resolutions[resolution-1][0],resolutions[resolution-1][1]), 60, 415, 30, BLACK);
    DrawText("FullScreen", 60, 495, 30, BLACK);
    
    if(DrawButton("Back", (Rectangle) {45 , 600, 130, 70}, 30, 5, true, true, BLACK, MAROON)) pageCode = 0;
    if(DrawButton("Reset", (Rectangle) {185, 600, 130, 70}, 30, 5, true, true, BLACK, MAROON)) {
        remove("settings.dat");
        width = 1280, height = 720;
        squareSize = 70;
        thickness = 12;
        uiSize = 9;
        arraySize = 19;
        turnPlayer = 0;
        wallCount = 10;
        resolution = 2;
        isFullScreen = false;
    }
    if(DrawButton("Apply", (Rectangle) {325, 600, 130, 70}, 30, 5, true, true, BLACK, MAROON)) {
        offset.x = resolutions[resolution-1][0] / 2 - squareSize * ((float) uiSize / 2);
        offset.y = resolutions[resolution-1][1] / 2 - squareSize * ((float) uiSize / 2);
        if(offset.x > 0 && offset.y > 0) {
            settingsFile(1);
            pageCode = 0;
        }
    }
}

void swap (int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;    
}

void initArray(int arr[MAX_ARRAY_SIZE][MAX_ARRAY_SIZE]) {   
    for(int j = 0; j < MAX_ARRAY_SIZE; j++) {
        for(int i = 0; i < MAX_ARRAY_SIZE; i++) {
            if(i == 0 || i == arraySize - 1 || j == 0 || j == arraySize - 1) {
                arr[j][i] = 8;
            } else if(i % 2 == 1 && j % 2 == 1) {
                arr[j][i] = 7;
            } else {
                arr[j][i] = 0;
            }
        }
    }
}

void copy(int actionCode, int arr[MAX_ARRAY_SIZE][MAX_ARRAY_SIZE]) {
    if(actionCode == 1) {
        for(int j = 0; j < arraySize; j++) {
            for(int i = 0; i < arraySize; i++) {
                arr[j][i] = map[j][i];
            }
        }        
    } else if (actionCode == 2) {
        for(int j = 0; j < arraySize; j++) {
            for(int i = 0; i < arraySize; i++) {
                map[j][i] = arr[j][i];
            }
        }        
    }
}

void delay(float time) {
    float curTime = GetTime();
    while(curTime + time > GetTime());
}

bool isAnyGameStarted() {
    initArray(mapCopy);
    for(int j = 0; j < arraySize; j++) {
        for(int i = 0; i < arraySize; i++) {
            if((i == 0 || i == arraySize - 1 || j == 0 || j == arraySize - 1) && map[j][i] != 8) return false;
        }
    }
    for(int j = 1; j < arraySize - 1; j++) {
        for(int i = 1; i < arraySize - 1; i++) {
            if( (j == arraySize-2 && i == arraySize/2)
            || (j == 1 && i == arraySize/2)
            || (j == arraySize/2 && i == arraySize-2)
            || (j == arraySize/2 && i == 1) ) continue;
            if(mapCopy[j][i] != map[j][i]) return true;
        }
    }
    return false;
}

void turn() {
    // 1: Human | 2: Random | 3: AI
    turnPlayer++;
    if(isDuo && turnPlayer == 2) turnPlayer = 0; //turnPlayer is 0 or 1
    else if(!isDuo && turnPlayer == 4) turnPlayer = 0; //turnPlayer is 0 or 1 or 2 or 3
}

void DrawDialog(int *value,Rectangle rec, int unit, int min, int max) {
    Button plusBtn, minusBtn;
    
    minusBtn.rec = (Rectangle) {rec.x, rec.y, 50, rec.height};   
    plusBtn.rec = (Rectangle) {rec.x + rec.width - 50, rec.y, 50, rec.height};
    
    if(CheckCollisionPointRec( (Vector2) GetMousePosition(), plusBtn.rec)) {
        plusBtn.color = MAROON;
        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) *value += unit;
        if(*value > max) *value = min;
    } else plusBtn.color = BLACK;
    
    if(CheckCollisionPointRec( (Vector2) GetMousePosition(), minusBtn.rec)) {
        minusBtn.color = MAROON;
        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) *value -= unit;
        if(*value < min) *value = max;
    } else minusBtn.color = BLACK;
    
    DrawText(FormatText("%i", *value), rec.x + 55, rec.y + 10, 40, BLACK);
    DrawText("<", minusBtn.rec.x + 15, minusBtn.rec.y + 10, 50, minusBtn.color);
    DrawText(">", plusBtn.rec.x + 16, plusBtn.rec.y + 10, 50, plusBtn.color);
    
    DrawRectangleLinesEx(rec, 5, BLACK);
    DrawRectangleLinesEx(minusBtn.rec, 5, minusBtn.color);
    DrawRectangleLinesEx(plusBtn.rec, 5, plusBtn.color);
}

bool DrawButton(char *text, Rectangle rec, int fontSize, int thickness, bool centerX, bool centerY, Color color1, Color color2) {
    Vector2 center = MeasureTextEx(GetFontDefault(), text, fontSize, 1);
    if(centerX == false) center.x = 0;
    if(centerY == false) center.y = 0;
    Color color;
    if(CheckCollisionPointRec( (Vector2) GetMousePosition(), rec) ) color = color2;
    else color = color1;
    DrawText(text, rec.x + rec.width/2 - center.x/2, rec.y + rec.height/2 - center.y/2, fontSize, color);
    DrawRectangleLinesEx(rec, thickness, color);
    if(CheckCollisionPointRec( (Vector2) GetMousePosition(), rec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ) return true;
    else return false;
}

void DrawInputBox(char text[], Rectangle textBox,int fontSize, int maxInput, Color textColor,Color bgColor, Color borderColor) {
    static bool mouseOnText = false;
    static int letterCount = 0;;
    static int framesCounter = 0;

    if (CheckCollisionPointRec(GetMousePosition(), textBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) mouseOnText = true;
    else if(!CheckCollisionPointRec(GetMousePosition(), textBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) mouseOnText = false;
    if(MeasureText(text,fontSize) + MeasureText("A",fontSize) > textBox.width) mouseOnText = false;
    if (CheckCollisionPointRec(GetMousePosition(), textBox) && IsKeyPressed(KEY_BACKSPACE)) mouseOnText = true;
        
    if (mouseOnText) {
        int key = GetKeyPressed();
        if(letterCount < maxInput && key >= 32 && key <= 125) {
            text[letterCount] = (char) key;
            letterCount++;          
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            letterCount--;
            text[letterCount] = '\0';
            if (letterCount < 0) letterCount = 0;
        }
    }

    if (mouseOnText) framesCounter++;
    else framesCounter = 0;

    DrawRectangleRec(textBox, bgColor);
    
    if (mouseOnText) DrawRectangleLines(textBox.x, textBox.y, textBox.width, textBox.height, borderColor);
    else DrawRectangleLines(textBox.x, textBox.y, textBox.width, textBox.height, DARKGRAY);
    
    DrawText(text, textBox.x + 5, textBox.y + 8, fontSize, textColor);

    if (mouseOnText) {
        if (letterCount < maxInput) {
            if ( ( (framesCounter/20) % 2) == 0)
            DrawText("_", textBox.x + 8 + MeasureText(text, fontSize), textBox.y + 12, fontSize, textColor);
        }
    }
}

bool IsAnyKeyPressed() {
    bool keyPressed = false;
    int key = GetKeyPressed();
    if ((key >= 32) && (key <= 126)) keyPressed = true;
    return keyPressed;
}