/*
|| Final Project CSE 1502 - RPG World
|| Creator: Tamim Saleem
|| Version 1.4 (May 8, 2026) - C++, Raylib
|| Overview: C++ RPG program that includes character creation, world exploration, and battling with enemies
*/

// Includes and defines
#include "raylib.h"
#include <math.h>
#include <string>
#include <vector>
#include <iostream>
#define MOUSE_LEFT_BUTTON   MOUSE_BUTTON_LEFT
#define MOUSE_RIGHT_BUTTON  MOUSE_BUTTON_RIGHT
#define MOUSE_MIDDLE_BUTTON MOUSE_BUTTON_MIDDLE

using namespace std;

// ===================== GAME STATES ENUM =====================
// Incomplete, make sure to finish BATTLE state and add more states as needed (SHOP, INVENTORY, etc)
enum GameState {
    CHARACTER_CREATION,
    WORLD,
    BATTLE,
    WIN,
    GAME_OVER
};

// ===================== PLAYER DIRECTION ENUM =====================
enum Direction {
    DOWN,
    UP,
    LEFT,
    RIGHT
};

// ===================== CHARACTER / PLAYER CLASSES =====================

// Struct for abilities/powers
struct Ability {
    string name;
    int damage;
    int energyCost;
    int magicCost;
};

// Class for any character, including enemies
class Character {
protected:
    // Stats for every character here
    string name;
    string fighterType; // e.g., "Warrior", "Elemental Mage", etc.
    int health;
    int maxHealth;
    int attack;
    int defense; // reduces incoming damage
    int speedStat; // probably determines who goes first in battle
    int magic; // stat that determines how many spells you can cast

public:
    // Constructor to initialize character stats
    Character(string n, string fT, int hp, int atk, int def, int spd, int mp)
        : name(n), fighterType(fT), health(hp), maxHealth(hp),
        attack(atk), defense(def), speedStat(spd), magic(mp) {
    }

    // Getting hit function (reduces health when the player is attacked by the enemy)
    void hit(int dmg) {
        // Damage taken is reduced by the defense stat of the player (similar to armor)
        int actualDamage = dmg - defense;

        if (actualDamage < 1) {
            actualDamage = 1; // always at least 1 damage
        }

        // Calculates the health by reducing the current health from the amount of damage taken
        health -= actualDamage;

        if (health < 0) {
            health = 0;
        }
    }
    string getName() const {
        return name;
    };

    int getAttack() const {
        return attack;
    }

    string getFighterType() const {
        return fighterType;
    };


    // Health accessors to avoid using void-returning functions in expressions
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
};

// Subclass for just the user's player
class Player : public Character {
public:
    Player(string n, string fT, int hp, int atk, int def, int spd, int mp)
        : Character(n, fT, hp, atk, def, spd, mp) {
    }

    void showStats() {
        // Prints all of the stats of the user in order
        DrawText(TextFormat("Name: %s", name.c_str()), 20, 20, 20, WHITE); // Example: this prints the name
        DrawText(TextFormat("FighterType: %s", fighterType.c_str()), 20, 50, 20, WHITE);
        DrawText(TextFormat("Health: %d / %d", health, maxHealth), 20, 80, 20, WHITE);
        DrawText(TextFormat("Attack: %d", attack), 20, 110, 20, WHITE);
        DrawText(TextFormat("Defense: %d", defense), 20, 140, 20, WHITE);
        DrawText(TextFormat("Speed: %d", speedStat), 20, 170, 20, WHITE);
        DrawText(TextFormat("Magic: %d", magic), 20, 200, 20, WHITE);
    }
    Ability abilities[3]; // allow multiple abilities later
    int abilityCount = 0;
};

// Subclass for enemy
class Enemy : public Character {
public:
    Enemy(string n, string fT, int hp, int atk, int def, int spd, int mp)
        : Character(n, fT, hp, atk, def, spd, mp) {
    }
};

// global battle enemy
Enemy* currentEnemy = nullptr;
bool battleInit = false;

// random enemy generator
Enemy* GenerateEnemy() {
    // Randomizer with rolling into three different categories of enemies
    int roll = rand() % 3;

    if (roll == 0) {
        return new Enemy("Slime", "Enemy", 40, 8, 2, 5, 0);
    }
    else if (roll == 1) {
        return new Enemy("Golem", "Enemy", 60, 12, 4, 10, 0);
    }
    else {
        return new Enemy("Dark Wizard", "Enemy", 50, 10, 3, 8, 30);
    }
}

// Initialize player
Player player("", "", 0, 0, 0, 0, 0);

// Create the basic rectangular shape for the buttons as well as the ability to be clicked
struct Button {
    Rectangle rect;
    string label;
    bool pressed = false;
};

// Room IDs; many are placeholders to be later used
enum RoomID {
    START,
    FOREST_1,
    FOREST_2,
    VILLAGE_N,
    VILLAGE_S,
    VILLAGE_E,
    VILLAGE_W
};

// Blocks in rooms (collisions)
// START room
vector<Rectangle> startBlocks;

// FOREST room
vector<Rectangle> forest1Blocks;

// Interactable objects
Rectangle interactable = { 775, 210, 50, 50 };
bool showText = false;

// Textbox function
void TextB(Font font, const std::string& text, Vector2 pos,
    float fontSize, float spacing, float maxWidth, Color color)
{
    // Initialize lines and words to be later used
    std::string line = "";
    std::string word = "";
    float yOffset = 0;

    for (size_t i = 0; i <= text.size(); i++) {
        char c = (i < text.size()) ? text[i] : ' '; // force flush last word
        // Adds characters
        if (c != ' ') {
            word += c;
        }
        else {
            std::string testLine = line + word + " ";
            // Measure the width of the line to add the next word
            float width = MeasureTextEx(font, testLine.c_str(), fontSize, spacing).x;

            if (width > maxWidth) {
                // draw current line
                DrawTextEx(font, line.c_str(), { pos.x, pos.y + yOffset }, fontSize, spacing, color);
                yOffset += fontSize + 4;

                // start new line
                line = word + " ";
            }
            else {
                line = testLine;
            }

            word = "";
        }
    }

    // draw last line
    if (!line.empty()) {
        DrawTextEx(font, line.c_str(), { pos.x, pos.y + yOffset }, fontSize, spacing, color);
    }
}

// Global location variable
string loc;

// Global class variable
string selectedCls = "";

// Global battle entry variable
static bool enteredBattle = false;

// Global battle ended variable
bool battleEnded = false;

GameState state = CHARACTER_CREATION;

// Battle function
void BattleSystem(Player& player, bool& resetFlag)
{
    // Initilization variables turned to zero
    static bool initialized = false;
    static int turn = 0;
    static float enemyDelay = 0.0f;
    static string battleLog = "";

    if (resetFlag) {
        initialized = false;
        resetFlag = false;
    }

    if (!initialized)
    {
        // Deletes the existing enemy and uses the function to generate a new enemy
        if (currentEnemy) delete currentEnemy;
        currentEnemy = GenerateEnemy();

        // Also resets other battle variables
        turn = 0;
        enemyDelay = 0;
        battleLog = "A wild " + currentEnemy->getName() + " appears!";
        initialized = true;
    }

    // ================= INPUT + LOGIC ONLY =================

    if (turn == 0)
    {
        if (IsKeyPressed(KEY_A))
        {
            // When the A key is pressed, the player will attack the enemy
            currentEnemy->hit(player.getAttack());
            battleLog = "You attacked for " + to_string(player.getAttack()) + " dmg!";
            turn = 1;
            enemyDelay = 0.8f;
        }

        if (IsKeyPressed(KEY_P))
        {
            // When the P key is pressed, the player can flee; the player can move down and press P to officially leave the fight in this version
            battleLog = "You escaped!";
            initialized = false;
            state = WORLD;
        }
    }
    else
    {
		// Enemy's turn, which is delayed by a set amount of time to give the player a chance to read the battle log
        enemyDelay -= GetFrameTime();
        if (enemyDelay <= 0)
        {
            int enemyAtk = currentEnemy->getAttack();
            player.hit(enemyAtk);
            battleLog = currentEnemy->getName() + " attacks for " + to_string(enemyAtk) + " dmg!";
            turn = 0;
        }
    }

    // ================= END CONDITIONS =================

    if (currentEnemy->getHealth() <= 0)
    {
        initialized = false;
        state = WIN;
        return;
    }

    if (player.getHealth() <= 0)
    {
        initialized = false;
        state = GAME_OVER;
        return;
    }

    // ================= DRAW ONLY (NO Begin/EndDrawing) =================

    ClearBackground(DARKGRAY);

    DrawText("BATTLE MODE", 320, 20, 30, RED);

    // --- Player stats panel ---
    player.showStats();

    // --- Player HP bar ---
    int pBarW = 200;
    int pBarH = 18;
    int pBarX = 20;
    int pBarY = 230;
    float pRatio = (float)player.getHealth() / (float)player.getMaxHealth();
    DrawRectangle(pBarX, pBarY, pBarW, pBarH, DARKGRAY);
    DrawRectangle(pBarX, pBarY, (int)(pBarW * pRatio), pBarH, GREEN);
    DrawRectangleLinesEx({ (float)pBarX, (float)pBarY, (float)pBarW, (float)pBarH }, 2, WHITE);
    DrawText("HP", pBarX, pBarY - 18, 16, WHITE);

    // --- Enemy panel ---
    DrawRectangle(580, 30, 280, 160, { 40, 40, 40, 200 });
    DrawRectangleLinesEx({ 580, 30, 280, 160 }, 2, RED);

    DrawText(TextFormat("Enemy: %s", currentEnemy->getName().c_str()), 595, 45, 20, WHITE);
    DrawText(TextFormat("HP: %d / %d", currentEnemy->getHealth(), currentEnemy->getMaxHealth()), 595, 75, 18, LIGHTGRAY);

    // Enemy HP bar
    int eBarW = 240;
    int eBarH = 18;
    int eBarX = 595;
    int eBarY = 105;
    float eRatio = (float)currentEnemy->getHealth() / (float)currentEnemy->getMaxHealth();
    DrawRectangle(eBarX, eBarY, eBarW, eBarH, DARKGRAY);
    DrawRectangle(eBarX, eBarY, (int)(eBarW * eRatio), eBarH, RED);
    DrawRectangleLinesEx({ (float)eBarX, (float)eBarY, (float)eBarW, (float)eBarH }, 2, WHITE);

    // Turn indicator
    const char* turnText = (turn == 0) ? ">> YOUR TURN" : "   Enemy thinking...";
    DrawText(turnText, 595, 140, 18, (turn == 0) ? YELLOW : ORANGE);

    // Battle log
    DrawRectangle(30, 340, 840, 50, { 0, 0, 0, 160 });
    DrawText(battleLog.c_str(), 45, 355, 20, YELLOW);

    // Controls
    DrawText("[ A ] Attack        [ P ] Flee", 300, 440, 20, WHITE);
}

// Function intended to begin the battle by setting to initial variables (nullptr which does nothing for now)
void StartBattle() {
    if (currentEnemy != nullptr) {
        delete currentEnemy;
        currentEnemy = nullptr;
    }

    // Generates enemy and sets it to the current enemy that the user is fighting
    currentEnemy = GenerateEnemy();

    // Battle has begun, but has not ended yet
    battleInit = true;
    battleEnded = false;
}

void ResetBattle() {
    if (currentEnemy != nullptr)
    {
        delete currentEnemy;
        currentEnemy = nullptr;
    }

    currentEnemy = GenerateEnemy();
    battleEnded = false;
}

// Main function
int main() {
    // Raylib window initialization
    InitWindow(900, 500, "RPG WORLD");

    SetTargetFPS(60);

    // ===================== CHARACTER CREATION =====================
    // Buttons for class selection
    Button warriorBtn = { {50, 100, 200, 50}, "Warrior" };
    Button mageBtn = { {50, 160, 200, 50}, "Elemental Mage" };
    Button techBtn = { {50, 220, 200, 50}, "Tech Master" };
    Button rogueBtn = { {50, 280, 200, 50}, "Rogue" };
    Button clericBtn = { {50, 340, 200, 50}, "Cleric" };

    Button finishBtn = { {50, 420, 200, 50}, "FINISH" };

    // Loading the file that includes every possible direction of the character spritesheet
    Texture2D playerSheet = LoadTexture("a.png");

    const int frameCount = 24;
    const int frameWidth = playerSheet.width / frameCount;
    const int frameHeight = playerSheet.height;

    // User is set initially at the middle of the screen, but can move
    Vector2 pos = { 450, 250 };
    Vector2 velocity = { 0, 0 };
    float speed = 180.0f;

    // Variables for the direction of the player
    Direction dir = DOWN;
    Direction prevDir = DOWN;

    // Frame mechanics
    int frame = 0;
    float animTimer = 0.0f;
    float animSpeed = 0.12f;

    // ===================== BATTLE PLACEHOLDER =====================

    // Tile system - a 1 is a block and a 0 is empty
    const int tileSize = 50;

    // Grid system that for RPG mechanics (creates blocks and is later used for collision mechanics)
    int caveMap[10][18] = {
    {1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,1},
    {1,0,0,1,1,0,1,1,1,1,1,0,0,0,1,1,1,1},
    {1,0,0,0,1,1,1,0,0,0,1,0,0,0,1,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,1,1,1,0,1,0,0,1},
    {1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,1},
    {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    };

    // Grid for second room (forest)
    int forest1Map[10][18] = {
    {1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1},
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1},
    {1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
    {1,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,1},
    {1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1},
    {1,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,1},
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
    {1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1},
    {1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1},
    };

    // Builds cave map
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 18; x++) {
            if (caveMap[y][x] == 1) {
                startBlocks.push_back(Rectangle{ (float)(x * tileSize), (float)(y * tileSize), (float)tileSize, (float)tileSize });
            }
        }
    }

    // Builds forest map
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 18; x++) {
            if (forest1Map[y][x] == 1) {
                forest1Blocks.push_back(Rectangle{ (float)(x * tileSize), (float)(y * tileSize), (float)tileSize, (float)tileSize });
            }
        }
    }

    bool resetBattleFlag = false;

    while (!WindowShouldClose()) {
        // Character creation screen
        if (state == CHARACTER_CREATION) {

            static int menuState = 0;
            // 0 = start menu
            // 1 = name input
            // 2 = class select
            // 3 = stat specialization (your old system)
            // 4 = location input

            static string playerName = "";
            static string location = "";
            static string classType = "";
            static int statChoice = 0;

            struct ClassButton {
                Rectangle rect;
                string name;
                string desc;
                bool hovered = false;
            };

            static ClassButton classes[5] = {
                {{50, 120, 220, 40}, "Warrior", "A disciplined frontline fighter trained in swordsmanship and combat tactics. Warriors rely on raw strength, heavy armor, and endurance to overwhelm enemies in close combat."},
                {{50, 170, 220, 40}, "Elemental Mage", "A powerful spellcaster who channels the forces of nature. Elemental Mages specialize in fire, ice, wind, and lightning to deal high magical damage from a distance."},
                {{50, 220, 220, 40}, "Tech Master", "A tactical fighter who uses advanced gadgets, mechanical tools, and precision engineering. Tech Masters adapt to any situation with balanced offense and utility."},
                {{50, 270, 220, 40}, "Rogue", "A fast and silent assassin who strikes before enemies can react. Rogues rely on agility, stealth, and critical hits to eliminate targets quickly and escape unseen."},
                {{50, 320, 220, 40}, "Cleric", "A sacred support class devoted to healing and protection. Clerics keep allies alive through healing magic, buffs, and defensive abilities during battle."}
            };

            Vector2 mouse = GetMousePosition();

            // Background color; begins at white
            BeginDrawing();
            ClearBackground(GRAY);

            // Start menu
            if (menuState == 0) {
                DrawText("RPG WORLD", 320, 120, 40, RED);
                DrawText("Press ENTER to start character creation", 220, 200, 20, BLACK);

                if (IsKeyPressed(KEY_ENTER)) {
                    menuState = 1;
                }
            }

            // Name input
            else if (menuState == 1) {
                DrawText("Enter Your Name:", 50, 100, 25, BLACK);
                DrawRectangle(50, 150, 300, 40, LIGHTGRAY);
                DrawText(playerName.c_str(), 60, 160, 20, BLACK);

                DrawText("Press ENTER to continue", 50, 220, 20, DARKGRAY);

                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125) && playerName.size() < 20) {
                        playerName += (char)key;
                    }
                    key = GetCharPressed();
                }

                // Backspace will remove characters that were entered
                if (IsKeyPressed(KEY_BACKSPACE) && !playerName.empty()) {
                    playerName.pop_back();
                }

                // If the player has inputted a name to the program, they can press the enter key to continue
                if (IsKeyPressed(KEY_ENTER) && !playerName.empty()) {
                    menuState = 2;
                }
            }

            // Class selection
            else if (menuState == 2) {
                DrawText(("Welcome, " + playerName).c_str(), 50, 30, 25, BLACK);
                DrawText("Select Your Class:", 50, 70, 25, BLACK);

                static int selectedClass = -1;

                for (int i = 0; i < 5; i++) {
                    bool hover = CheckCollisionPointRec(mouse, classes[i].rect);

                    // Click detection
                    if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        selectedClass = i;
                        classType = classes[i].name;
                    }

                    // Decide color BEFORE drawing
                    Color color = LIGHTGRAY;

                    if (selectedClass == i) {
                        color = YELLOW;          // stays selected
                    }
                    else if (hover) {
                        color = SKYBLUE;         // temporary hover
                    }

                    // Draw button
                    DrawRectangleRec(classes[i].rect, color);
                    DrawText(classes[i].name.c_str(), classes[i].rect.x + 10, classes[i].rect.y + 10, 18, BLACK);

                    // Show description ONLY for selected OR hovered
                    if (selectedClass == i || hover) {
                        Rectangle box = { 400, 115, 400, 272 };

                        DrawRectangleRec(box, LIGHTGRAY);
                        DrawRectangleLinesEx(box, 2, DARKGRAY);

                        // Draws the textbox using the previously declared function
                        TextB(
                            GetFontDefault(),
                            classes[i].desc,
                            { box.x + 10, box.y + 25 },
                            26,     // font size
                            2.2,    // spacing
                            400,    // max width of box
                            DARKGRAY
                        );
                    }
                }

                // Finish button
                Rectangle finishBtn = { 50, 400, 200, 50 };
                bool hoverFinish = CheckCollisionPointRec(mouse, finishBtn);

                DrawRectangleRec(finishBtn, hoverFinish ? GREEN : DARKGREEN);
                DrawText("FINISH", 110, 415, 20, WHITE);

                if (hoverFinish && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !classType.empty()) {
                    menuState = 3;
                    statChoice = -1;
                }

                selectedCls = selectedClass;

                if (selectedCls == "Warrior") {
                    // player.abilities.name("Punch");
                }
            }
            // Stat specialization, adds uniqueness
            else if (menuState == 3) {
                DrawText(("Class: " + classType).c_str(), 50, 50, 25, BLACK);
                DrawText("Choose Stat Specialization:", 50, 100, 25, BLACK);
                const char* options[3] = { "Strength", "Speed", "Magic" };

                for (int i = 0; i < 3; i++) {
                    Rectangle r = { 50, 160 + i * 60, 200, 40 };
                    bool hover = CheckCollisionPointRec(mouse, r);

                    if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        statChoice = i;
                    }

                    Color color = GRAY;

                    if (statChoice == i) {
                        color = YELLOW;      // selected
                    }
                    else if (hover) {
                        color = SKYBLUE;     // hover
                    }

                    DrawRectangleRec(r, color);
                    DrawRectangleLinesEx(r, 2, DARKGRAY);
                    DrawText(options[i], r.x + 20, r.y + 10, 20, BLACK);
                }
                Rectangle finishBtn = { 50, 400, 200, 50 };
                bool hoverFinish = CheckCollisionPointRec(mouse, finishBtn);

                DrawRectangleRec(finishBtn, hoverFinish ? GREEN : DARKGREEN);
                DrawText("FINISH", 110, 415, 20, WHITE);

                if (hoverFinish && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !classType.empty()) {
                    menuState = 4;
                    // Initialize player

                    // Create player
                    player = Player(
                        playerName,
                        classType,
                        100,
                        (statChoice == 0 ? 15 : 10), // strength
                        (statChoice == 1 ? 15 : 5),  // speed
                        (statChoice == 2 ? 15 : 8),  // magic
                        5
                    );

                    player.abilities[0] = { "Strike", player.getAttack(), 0, 0 };

                }
            }
            // Location input
            else if (menuState == 4) {
                DrawText("Enter World Location:", 50, 100, 25, BLACK);

                DrawRectangle(50, 150, 300, 40, LIGHTGRAY);
                DrawText(location.c_str(), 60, 160, 20, BLACK);

                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125) && location.size() < 25) {
                        location += (char)key;
                    }
                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE) && !location.empty()) {
                    location.pop_back();
                }

                DrawText("Press ENTER to begin adventure", 50, 220, 20, DARKGRAY);

                if (IsKeyPressed(KEY_ENTER) && !location.empty()) {
                    loc = location;
                    state = WORLD;
                }
            }

            EndDrawing();

            // ===================== WORLD MOVEMENT =====================
        }
        else if (state == WORLD) {
            float dt = GetFrameTime();

            // ================= INPUT =================
            Vector2 input = { 0, 0 };

            // Uses WASD or arrow keys for movement input
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    input.y -= 1;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  input.y += 1;
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  input.x -= 1;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) input.x += 1;

            // normalize (prevents faster diagonal movement)
            float len = sqrtf(input.x * input.x + input.y * input.y);
            if (len > 0.0f) {
                input.x /= len;
                input.y /= len;
            }

            Vector2 initialPosition = { 450, 250 };

            // Reset position by pressing R
            if (IsKeyDown(KEY_R)) {
                pos = initialPosition;
                pos.y -= 5.0f;
            }

            // ================= DIRECTION =================
            // only update facing direction if there is movement intent
            if (len > 0.0f) {
                if (fabsf(input.x) > fabsf(input.y)) {
                    dir = (input.x > 0) ? RIGHT : LEFT;
                }
                else {
                    dir = (input.y > 0) ? DOWN : UP;
                }
            }

            // ================= ROOM =================
            static RoomID currentRoom = START;

            vector<Rectangle>* currentBlocks = &startBlocks;
            if (currentRoom == FOREST_1) currentBlocks = &forest1Blocks;

            // ================= MOVE =================
            Vector2 oldPos = pos;

            // movement speed (consistent scaling)
            float moveStep = speed * dt;

            // ================= COLLISION SIZE =================
            // Hitbox size (x, y)
            const float hitW = 57.0f; // x
            const float hitH = 67.0f; // y

            // ================= MOVE X =================
            pos.x += input.x * speed * dt;

            Rectangle playerRect = {
                pos.x - hitW * 0.5f,
                pos.y - hitH * 0.5f,
                hitW,
                hitH
            };

            for (const Rectangle& b : *currentBlocks) {
                if (CheckCollisionRecs(playerRect, b)) {
                    pos.x = oldPos.x; // undo X movement
                    break;
                }
            }

            // ================= MOVE Y =================
            pos.y += input.y * speed * dt;

            // rebuild hitbox AFTER Y update
            playerRect.x = pos.x - hitW * 0.5f;
            playerRect.y = pos.y - hitH * 0.5f;

            for (const Rectangle& b : *currentBlocks) {
                if (CheckCollisionRecs(playerRect, b)) {
                    pos.y = oldPos.y; // undo Y movement
                    break;
                }
            }

            // =============== ANIMATIONS ================
            static Direction lastDir = DOWN;

            // ================= FRAME RANGES =================
            int startFrame = 0;
            int frameCountAnim = 3;

            switch (dir) {
            case DOWN:
                startFrame = 0;
                frameCountAnim = 3;
                break;

            case RIGHT:
                startFrame = 3;
                frameCountAnim = 9;
                break;

            case LEFT:
                startFrame = 12;
                frameCountAnim = 9;
                break;

            case UP:
                startFrame = 21;
                frameCountAnim = 3;
                break;
            }

            // ================= RESET ON DIRECTION CHANGE =================
            if (dir != lastDir) {
                lastDir = dir;
                animTimer = 0;
                frame = startFrame;
            }

            // ================= MOVING CHECK =================
            bool moving = (len > 0);

            // ================= ANIMATION =================
            if (moving) {
                animTimer += dt;

                if (animTimer >= animSpeed) {
                    animTimer = 0;

                    frame++;

                    if (frame >= startFrame + frameCountAnim) {
                        frame = startFrame;
                    }
                }
            }
            else {
                animTimer = 0;
                frame = startFrame;
            }

            // Find all object hitboxes with H 
            static bool showHitboxes = false;
            if (IsKeyPressed(KEY_H)) {
                showHitboxes = !showHitboxes;
            }
            if (showHitboxes) {
                DrawRectangleLinesEx(playerRect, 2, RED);
            }
            if (showHitboxes) {
                for (const Rectangle& b : *currentBlocks) {
                    DrawRectangleLinesEx(b, 2, BLUE);
                }
            }
            if (showHitboxes) {
                DrawRectangleLinesEx(interactable, 2, GREEN);
            }

            // Drawing for WORLD state
            BeginDrawing();
            ClearBackground({ 25, 25, 30, 255 });

            Rectangle source = {
                frame * (float)frameWidth,
                0,
                (float)frameWidth,
                (float)frameHeight
            };

            Rectangle dest = {
                pos.x,
                pos.y,
                frameWidth * 3,
                frameHeight * 3
            };

            Vector2 origin = {
                frameWidth * 1.5f,
                frameHeight * 1.5f
            };

            DrawTexturePro(playerSheet, source, dest, origin, 0, WHITE);

            // Room name display
            DrawText(TextFormat("Room: %d", currentRoom), 650, 20, 20, BLACK);

            // ==== START ROOM ====
            Color oreColor = { 212, 175, 55, 255 };
            Rectangle ore = { 800, 0, 50, 150 };
            if (currentRoom == START)
                DrawRectangleRec(ore, oreColor);

            // Switching between rooms
            if (currentRoom == START && pos.y < 0) {
                currentRoom = FOREST_1;
                pos.y = 440; // spawn at bottom of next room
            }

            if (currentRoom == FOREST_1 && pos.y > 501) {
                currentRoom = START;
                pos.y = 10; // spawn at top of next room
            }

            if (currentRoom == FOREST_1 && pos.y < 0) {
                StartBattle();
                state = BATTLE; // first battle
            }

            if (currentRoom == START) {
                currentBlocks = &startBlocks;
            }
            else if (currentRoom == FOREST_1) {
                currentBlocks = &forest1Blocks;
            }

            if (currentRoom == FOREST_1) {
                ClearBackground({ 20, 100, 40, 255 });
                currentBlocks = &forest1Blocks;
            }

			// Draw blocks for the current room
            for (Rectangle block : *currentBlocks) {
                if (currentRoom == START) {
                    DrawRectangleRec(block, GRAY);
                }
                else if (currentRoom == FOREST_1) {
                    DrawRectangleRec(block, { 10, 50, 25, 255 });
                }
            }

            Rectangle playerRectInteract = {
                pos.x - frameWidth * 1.5f,
                pos.y - frameHeight * 1.5f,
                frameWidth * 3,
                frameHeight * 3
            };

            // Expand interaction range slightly
            Rectangle interactRange = interactable;
            interactRange.x -= 10;
            interactRange.y -= 10;
            interactRange.width += 20;
            interactRange.height += 20;

            bool inRange = CheckCollisionRecs(playerRectInteract, interactRange);

            if (currentRoom == START && inRange && IsKeyPressed(KEY_SPACE)) {
                showText = !showText; // toggle on/off
            }

            Rectangle chestPiece = { 796.5, 255, 7, 12 };

            if (currentRoom == START) {
                DrawRectangleRec(interactable, DARKBROWN);
                DrawRectangleRec(chestPiece, YELLOW);
            }

            if (!inRange) {
                showText = false;
            }

            if (showText) {
                DrawRectangle(250, 350, 400, 60, Fade(BLACK, 0.7f));
                DrawText("There appears to be nothing here.", 270, 370, 20, WHITE);
            }

            DrawText(("World Mode: " + loc).c_str(), 20, 20, 20, WHITE);

            // TEMPORARY battle trigger (Press B)
            if (IsKeyPressed(KEY_B)) {
                state = BATTLE;
            }

            EndDrawing();
        }
        // ===================== BATTLE STATE PLACEHOLDER =====================

        else if (state == BATTLE)
        {
            BeginDrawing();
            BattleSystem(player, resetBattleFlag);
            EndDrawing();
        }
        else if (state == WIN)
        {
            BeginDrawing();
            ClearBackground(DARKGREEN);

            DrawText("YOU WIN!", 300, 150, 60, YELLOW);
            DrawText(TextFormat("You defeated the %s!", currentEnemy ? currentEnemy->getName().c_str() : "enemy"), 260, 240, 22, WHITE);
            DrawText("Press ENTER to continue", 280, 310, 22, WHITE);

            if (IsKeyPressed(KEY_ENTER))
            {
				// Resets battle state and returns to world mode
                
                ResetBattle();
                resetBattleFlag = true;
                state = WORLD;
            }

            EndDrawing();
        }
        else if (state == GAME_OVER)
        {
            // Game over screen

            BeginDrawing();
            ClearBackground(DARKPURPLE);

            DrawText("GAME OVER", 290, 150, 60, RED);
            DrawText("You were defeated...", 310, 240, 22, LIGHTGRAY);
            DrawText("Press ENTER to continue", 280, 310, 22, WHITE);

            if (IsKeyPressed(KEY_ENTER))
            {
                // Restore player health but keep name and class
                player = Player(player.getName(), player.getFighterType(), 100, player.getAttack(), 5, 10, 5);
                player.abilities[0] = { "Strike", player.getAttack(), 0, 0 };

                ResetBattle();
                resetBattleFlag = true;
                state = WORLD;
            }

            EndDrawing();
        }
    }

    // Cleanup for the end of the program
    UnloadTexture(playerSheet);
    CloseWindow();

    return 0;
}

