// headers
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

// macros
#define WIDTH 1600
#define HEIGHT 800
#define NORMALBALLOONSNUM 4
#define SPAWNPOINTS 5
#define MAXBALLOONS 14
#define MAXPOPUPS 5
#define MAXARROWPOPUPS 5
#define MAXLEADERBOARD 5

// enum for gamestate
typedef enum
{
    GAME_MENU,
    GAME_PLAYING,
    GAME_HOWTOPLAY,
    GAME_LEADERBOARD,
    GAME_CREDITS
} GameState;

// structs for arrow, ballloons and popups
typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool active;
} Arrow;

typedef struct
{
    Vector2 position;
    float radius;
    float speed;
    bool active;
    bool gold;
    bool danger;
    bool mustpop;
    int index;
} Balloon;

typedef struct
{
    Vector2 position;
    int value;
    float visibletime;
    bool active;
} ScorePopUp;

typedef struct
{
    Vector2 position;
    int value;
    float visibletime;
    bool active;
} ArrowPopUp;

// struct holding every variable that needs to persist across frames
typedef struct
{
    // game state
    GameState currentstate;

    // audio
    Music bgmusic;
    Music menumusic;
    Sound shootsound;
    Sound popsound;
    Sound gameoversound;
    Sound clicksound;
    Sound minusarrowssound;

    // textures and font
    Texture2D menubackground;
    Texture2D background;
    Texture2D gameovertexture;
    Texture2D bowimage;
    Texture2D arrowimage;
    Texture2D arrowballoon;
    Texture2D dangerballoon;
    Texture2D mustpopballoon;
    Texture2D normalballoons[NORMALBALLOONSNUM];
    Texture2D boytexture;
    Font customfont;

    // spawnpoints, arrow, balloons
    Vector2 spawnpoints[SPAWNPOINTS];
    Arrow arrow;
    Balloon balloons[MAXBALLOONS];
    ScorePopUp scorepopup[MAXPOPUPS];
    ArrowPopUp arrowpopup[MAXARROWPOPUPS];
    int leaderboard[MAXLEADERBOARD];

    // game variables
    int score;
    int arrowsleft;
    float gravity;
    float currenttimer;
    float spawninterval;
    float pulldistance;
    float launchspeed;
    bool gameover;
    bool ismuted;

    // balloon sizes
    float normalballoonwidth;
    float normalballoonheight;
    float balloonradius;

    float dangerwidth;
    float dangerheight;
    float dangerradius;

    float mustpopwidth;
    float mustpopheight;
    float mustpopradius;

    // set when the player clicks EXIT from the main menu (native builds only)
    bool requestexit;
} GameContext;

// scorepopup and arrowpopup functions
void ScorePopup(ScorePopUp scorepopup[], Vector2 pos, int score)
{
    for (int i = 0; i < MAXPOPUPS; i++)
    {
        if (scorepopup[i].active == false)
        {
            scorepopup[i].active = true;
            scorepopup[i].position = pos;
            scorepopup[i].value = score;
            scorepopup[i].visibletime = 1.0f;
            break;
        }
    }
}

void ArrowPopup(ArrowPopUp arrowpopup[], Vector2 pos, int arrows)
{
    for (int i = 0; i < MAXARROWPOPUPS; i++)
    {
        if (arrowpopup[i].active == false)
        {
            arrowpopup[i].active = true;
            arrowpopup[i].position = pos;
            arrowpopup[i].value = arrows;
            arrowpopup[i].visibletime = 1.0f;
            break;
        }
    }
}

// saving leaderboard scores
void SaveLeaderBoardScores(int leaderboard[])
{
    FILE *savefile = fopen("leaderboard.txt", "w");
    if (savefile != NULL)
    {
        for (int i = 0; i < MAXLEADERBOARD; i++)
        {
            fprintf(savefile, "%d\n", leaderboard[i]);
        }
        fclose(savefile);
    }
}

// sorting scores in leaderboard
void SortingScores(int leaderboard[], int newscore)
{
    if (newscore <= leaderboard[MAXLEADERBOARD - 1])
        return;
    int i = MAXLEADERBOARD - 1;
    while (i > 0 && newscore > leaderboard[i - 1])
    {
        leaderboard[i] = leaderboard[i - 1];
        i--;
    }
    leaderboard[i] = newscore;
}

void UpdateDrawFrame(void *arg)
{
    GameContext *ctx = (GameContext *)arg;

    // per-frame input and music update
    float dt = GetFrameTime();
    if (ctx->currentstate == GAME_PLAYING)
        UpdateMusicStream(ctx->bgmusic);
    else
        UpdateMusicStream(ctx->menumusic);
    Vector2 mouseposition = GetMousePosition();

    // game menu state
    if (ctx->currentstate == GAME_MENU)
    {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        Rectangle startbutton = {WIDTH / 2.0f - 120.0f, 350.0f, 260.0f, 50.0f};
        Rectangle howtoplaybutton = {WIDTH / 2.0f - 120.0f, 420.0f, 380.0f, 50.0f};
        Rectangle leaderboardbutton = {WIDTH / 2.0f - 120.0f, 490.0f, 420.0f, 50.0f};
        Rectangle creditsbutton = {WIDTH / 2.0f - 100.0f, 560.0f, 260.0f, 50.0f};
        Rectangle exitbutton = {WIDTH / 2.0f - 90.0f, 630.0f, 200.0f, 50.0f};
        Rectangle mutebutton = {WIDTH - 190.0f, 30.0f, 160.0f, 50.0f};

        if (CheckCollisionPointRec(mouseposition, startbutton) ||
            CheckCollisionPointRec(mouseposition, howtoplaybutton) ||
            CheckCollisionPointRec(mouseposition, leaderboardbutton) ||
            CheckCollisionPointRec(mouseposition, creditsbutton) ||
            CheckCollisionPointRec(mouseposition, exitbutton) ||
            CheckCollisionPointRec(mouseposition, mutebutton))
        {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (CheckCollisionPointRec(mouseposition, startbutton))
            {
                PlaySound(ctx->clicksound);
                ctx->currentstate = GAME_PLAYING;
                StopMusicStream(ctx->menumusic);
                PlayMusicStream(ctx->bgmusic);
            }
            else if (CheckCollisionPointRec(mouseposition, exitbutton))
            {
                PlaySound(ctx->clicksound);
                ctx->requestexit = true;
                return;
            }
            else if (CheckCollisionPointRec(mouseposition, howtoplaybutton))
            {
                PlaySound(ctx->clicksound);
                ctx->currentstate = GAME_HOWTOPLAY;
            }
            else if (CheckCollisionPointRec(mouseposition, leaderboardbutton))
            {
                PlaySound(ctx->clicksound);
                ctx->currentstate = GAME_LEADERBOARD;
            }
            else if (CheckCollisionPointRec(mouseposition, creditsbutton))
            {
                PlaySound(ctx->clicksound);
                ctx->currentstate = GAME_CREDITS;
            }
            else if (CheckCollisionPointRec(mouseposition, mutebutton))
            {
                PlaySound(ctx->clicksound);
                ctx->ismuted = !ctx->ismuted;
                SetMasterVolume(ctx->ismuted ? 0.0f : 1.0f);
            }
        }
    }
    // gameplay state
    else if (ctx->currentstate == GAME_PLAYING)
    {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        // mute button
        Rectangle mutebutton = {WIDTH - 190.0f, HEIGHT - 80.0f, 160.0f, 50.0f};
        bool mutehovered = CheckCollisionPointRec(mouseposition, mutebutton);
        if (mutehovered)
        {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mutehovered)
        {
            PlaySound(ctx->clicksound);
            ctx->ismuted = !ctx->ismuted;
            SetMasterVolume(ctx->ismuted ? 0.0f : 1.0f);
        }

        // bow and arrow settings
        Vector2 arrowpivot = {280.0f, 590.0f};
        float aimangle = 0.0f;
        Vector2 aimdirection = {1.0f, 0.0f};
        float maxpulldistance = 120.0f;
        float minarrowspeed = 500.0f;
        float maxarrowspeed = 2400.0f;
        float pullspeed = 100.0f;

        // aiming, pulling back and shooting arrow
        if (ctx->arrowsleft > 0 && ctx->gameover == false && mutehovered == false)
        {
            float mousepointerangle = atan2f(mouseposition.y - arrowpivot.y, mouseposition.x - arrowpivot.x);
            if (mousepointerangle > PI / 4.0)
            {
                aimangle = PI / 4.0;
            }
            else if (mousepointerangle < -PI / 4.0)
            {
                aimangle = -PI / 4.0;
            }
            else
            {
                aimangle = mousepointerangle;
            }
            aimdirection = (Vector2){cosf(aimangle), sinf(aimangle)};

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && ctx->arrow.active == false)
            {
                ctx->pulldistance += pullspeed * dt;
                if (ctx->pulldistance > maxpulldistance)
                {
                    ctx->pulldistance = maxpulldistance;
                }
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && ctx->arrow.active == false && ctx->pulldistance > 15.0f)
            {
                float pullratio = ctx->pulldistance / maxpulldistance;
                float arrowspeed = minarrowspeed + pullratio * (maxarrowspeed - minarrowspeed);

                ctx->arrow.position = arrowpivot;
                ctx->arrow.velocity = Vector2Scale(aimdirection, arrowspeed);
                ctx->arrow.radius = 17.0f;
                ctx->arrow.active = true;
                ctx->arrowsleft--;

                PlaySound(ctx->shootsound);
                ctx->launchspeed = arrowspeed;
                ctx->pulldistance = 0.0f;
            }
        }

        // projectile formula
        if (ctx->arrow.active == true)
        {
            ctx->arrow.velocity.y += dt * ctx->gravity;
            ctx->arrow.position = Vector2Add(ctx->arrow.position, Vector2Scale(ctx->arrow.velocity, dt));
            if (ctx->arrow.position.x > WIDTH + 50 || ctx->arrow.position.y > HEIGHT + 50)
            {
                ctx->arrow.active = false;
            }
        }

        // spawning balloons
        float basespeed = 150.0f;
        float speedincrease = 0.75f;
        if (ctx->gameover == false)
        {
            ctx->currenttimer += dt;
            if (ctx->currenttimer >= ctx->spawninterval)
            {
                ctx->currenttimer = 0.0f;
                for (int i = 0; i < MAXBALLOONS; i++)
                {
                    if (ctx->balloons[i].active == false)
                    {
                        ctx->balloons[i].position = ctx->spawnpoints[GetRandomValue(0, 4)];
                        ctx->balloons[i].speed = basespeed + ctx->score * speedincrease;
                        ctx->balloons[i].active = true;

                        int dangerroll = 0;
                        if (ctx->score >= 100)
                        {
                            dangerroll = 10 + (ctx->score - 100) / 10;
                            if (dangerroll > 30)
                                dangerroll = 30;
                        }
                        int mustpoproll = (ctx->score >= 80 && ctx->arrowsleft > 2) ? 20 : 0;
                        int roll = GetRandomValue(1, 100);

                        if (roll <= dangerroll)
                        {
                            ctx->balloons[i].danger = true;
                            ctx->balloons[i].mustpop = false;
                            ctx->balloons[i].gold = false;
                            ctx->balloons[i].radius = ctx->dangerradius;
                        }
                        else if (roll <= dangerroll + mustpoproll)
                        {
                            ctx->balloons[i].danger = false;
                            ctx->balloons[i].mustpop = true;
                            ctx->balloons[i].gold = false;
                            ctx->balloons[i].radius = ctx->mustpopradius;
                        }
                        else
                        {
                            ctx->balloons[i].danger = false;
                            ctx->balloons[i].mustpop = false;
                            ctx->balloons[i].gold = (GetRandomValue(1, 10) <= 3);
                            ctx->balloons[i].radius = ctx->balloonradius;
                        }

                        ctx->balloons[i].index = GetRandomValue(0, 3);
                        break;
                    }
                }
            }
        }

        // scorepopups and arrowpopups update
        for (int i = 0; i < MAXPOPUPS; i++)
        {
            if (ctx->scorepopup[i].active)
            {
                ctx->scorepopup[i].visibletime -= dt;
                ctx->scorepopup[i].position.y -= 20.0f * dt;
                if (ctx->scorepopup[i].visibletime <= 0)
                {
                    ctx->scorepopup[i].active = false;
                }
            }
        }
        for (int i = 0; i < MAXARROWPOPUPS; i++)
        {
            if (ctx->arrowpopup[i].active)
            {
                ctx->arrowpopup[i].visibletime -= dt;
                ctx->arrowpopup[i].position.y -= 20.0f * dt;
                if (ctx->arrowpopup[i].visibletime <= 0)
                {
                    ctx->arrowpopup[i].active = false;
                }
            }
        }

        // balloon movement & collision physics
        for (int i = 0; i < MAXBALLOONS; i++)
        {
            if (ctx->balloons[i].active == false)
                continue;

            ctx->balloons[i].position.y -= dt * ctx->balloons[i].speed;

            if (ctx->balloons[i].position.y < -100.0f)
            {
                ctx->balloons[i].active = false;
                if (ctx->balloons[i].mustpop == true)
                {
                    ctx->arrowsleft -= 2;
                    if (ctx->arrowsleft < 0)
                        ctx->arrowsleft = 0;
                    PlaySound(ctx->minusarrowssound);
                    ArrowPopup(ctx->arrowpopup, (Vector2){ctx->balloons[i].position.x, 40.0f}, -2);
                }
            }

            if (ctx->arrow.active == true && CheckCollisionCircles(ctx->arrow.position, ctx->arrow.radius, ctx->balloons[i].position, ctx->balloons[i].radius) == true)
            {
                ctx->balloons[i].active = false;

                if (ctx->balloons[i].danger == true)
                {
                    ctx->gameover = true;
                    StopMusicStream(ctx->bgmusic);
                    PlaySound(ctx->gameoversound);
                    SortingScores(ctx->leaderboard, ctx->score);
                    SaveLeaderBoardScores(ctx->leaderboard);
                }
                else if (ctx->balloons[i].gold == true)
                {
                    ctx->arrowsleft += 2;
                    ctx->score += 10;
                    PlaySound(ctx->popsound);
                    ScorePopup(ctx->scorepopup, ctx->balloons[i].position, 10);
                    ArrowPopup(ctx->arrowpopup, (Vector2){ctx->balloons[i].position.x, ctx->balloons[i].position.y - 40.0f}, 2);
                }
                else
                {
                    ctx->score += 10;
                    ScorePopup(ctx->scorepopup, ctx->balloons[i].position, 10);
                    PlaySound(ctx->popsound);
                }
            }
        }

        // gameover and highscore saving
        if (ctx->arrowsleft == 0 && ctx->arrow.active == false && ctx->gameover == false)
        {
            ctx->gameover = true;
            StopMusicStream(ctx->bgmusic);
            PlaySound(ctx->gameoversound);

            SortingScores(ctx->leaderboard, ctx->score);
            SaveLeaderBoardScores(ctx->leaderboard);
        }

        // restart and back to menu logic
        if (ctx->gameover == true)
        {
            Rectangle menubutton = {WIDTH - 270.0f, 40.0f, 220.0f, 100.0f};
            bool ishovered = CheckCollisionPointRec(mouseposition, menubutton);

            if (ishovered)
            {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            }
            if (IsKeyPressed(KEY_R) == true)
            {
                ctx->gameover = false;
                ctx->arrowsleft = 10;
                ctx->score = 0;
                ctx->currenttimer = 0.0f;
                ctx->launchspeed = 0.0f;
                ctx->pulldistance = 0.0f;
                ctx->arrow.active = false;

                for (int i = 0; i < MAXBALLOONS; i++)
                {
                    ctx->balloons[i].active = false;
                }

                StopSound(ctx->gameoversound);
                PlayMusicStream(ctx->bgmusic);
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && ishovered)
            {
                PlaySound(ctx->clicksound);
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);

                ctx->gameover = false;
                ctx->arrowsleft = 10;
                ctx->score = 0;
                ctx->currenttimer = 0.0f;
                ctx->launchspeed = 0.0f;
                ctx->pulldistance = 0.0f;
                ctx->arrow.active = false;

                for (int i = 0; i < MAXBALLOONS; i++)
                {
                    ctx->balloons[i].active = false;
                }

                StopSound(ctx->gameoversound);
                PlayMusicStream(ctx->menumusic);
                ctx->currentstate = GAME_MENU;
            }
        }
    }
    // how to play state
    else if (ctx->currentstate == GAME_HOWTOPLAY)
    {
        Rectangle crossbutton = {WIDTH / 2.0f + 340.0f, HEIGHT / 2.0f - 240.0f, 40.0f, 40.0f};

        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        if (CheckCollisionPointRec(mouseposition, crossbutton))
        {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseposition, crossbutton))
        {
            PlaySound(ctx->clicksound);
            ctx->currentstate = GAME_MENU;
        }
    }
    // leaderboard state
    else if (ctx->currentstate == GAME_LEADERBOARD)
    {
        Rectangle crossbutton = {WIDTH / 2.0f + 340.0f, HEIGHT / 2.0f - 240.0f, 40.0f, 40.0f};

        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        if (CheckCollisionPointRec(mouseposition, crossbutton))
        {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseposition, crossbutton))
        {
            PlaySound(ctx->clicksound);
            ctx->currentstate = GAME_MENU;
        }
    }
    // credits state
    else if (ctx->currentstate == GAME_CREDITS)
    {
        Rectangle crossbutton = {WIDTH / 2.0f + 390.0f, HEIGHT / 2.0f - 340.0f, 40.0f, 40.0f};

        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        if (CheckCollisionPointRec(mouseposition, crossbutton))
        {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseposition, crossbutton))
        {
            PlaySound(ctx->clicksound);
            ctx->currentstate = GAME_MENU;
        }
    }

    // all drawings
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (ctx->currentstate == GAME_MENU)
    {
        // draw menu background
        Rectangle menusource = {0.0f, 0.0f, (float)ctx->menubackground.width, (float)ctx->menubackground.height};
        Rectangle menudest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
        DrawTexturePro(ctx->menubackground, menusource, menudest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

        // menu option rectangles
        Rectangle startbutton = {WIDTH / 2.0f - 120.0f, 350.0f, 260.0f, 50.0f};
        Rectangle howtoplaybutton = {WIDTH / 2.0f - 120.0f, 420.0f, 380.0f, 50.0f};
        Rectangle leaderboardbutton = {WIDTH / 2.0f - 105.0f, 490.0f, 420.0f, 50.0f};
        Rectangle creditsbutton = {WIDTH / 2.0f - 85.0f, 560.0f, 260.0f, 50.0f};
        Rectangle exitbutton = {WIDTH / 2.0f - 90.0f, 630.0f, 200.0f, 50.0f};

        // hover colors
        Color startcolor = CheckCollisionPointRec(mouseposition, startbutton) ? GOLD : BLACK;
        Color howtoplaycolor = CheckCollisionPointRec(mouseposition, howtoplaybutton) ? GOLD : BLACK;
        Color highestscorecolor = CheckCollisionPointRec(mouseposition, leaderboardbutton) ? GOLD : BLACK;
        Color creditscolor = CheckCollisionPointRec(mouseposition, creditsbutton) ? GOLD : BLACK;
        Color exitcolor = CheckCollisionPointRec(mouseposition, exitbutton) ? GOLD : BLACK;

        // text coordinates
        DrawTextEx(ctx->customfont, "PLAY", (Vector2){WIDTH / 2.0f - 50.0f, 350.0f}, 48, 2, startcolor);
        DrawTextEx(ctx->customfont, "HOW TO PLAY", (Vector2){WIDTH / 2.0f - 120.0f, 420.0f}, 48, 2, howtoplaycolor);
        DrawTextEx(ctx->customfont, "LEADERBOARD", (Vector2){WIDTH / 2.0f - 115.0f, 490.0f}, 48, 2, highestscorecolor);
        DrawTextEx(ctx->customfont, "CREDITS", (Vector2){WIDTH / 2.0f - 80.0f, 560.0f}, 48, 2, creditscolor);
        DrawTextEx(ctx->customfont, "EXIT", (Vector2){WIDTH / 2.0f - 50.0f, 630.0f}, 48, 2, exitcolor);

        // mute/unmute button
        Rectangle mutebutton = {WIDTH - 190.0f, 30.0f, 160.0f, 50.0f};
        bool mutehovered = CheckCollisionPointRec(mouseposition, mutebutton);
        Color mutebuttoncolor = mutehovered ? GOLD : RAYWHITE;
        DrawRectangleRounded(mutebutton, 0.3f, 4, (Color){0, 0, 0, 150});
        DrawRectangleRoundedLines(mutebutton, 0.3f, 16, mutebuttoncolor);
        DrawTextEx(ctx->customfont, ctx->ismuted ? "UNMUTE" : "MUTE", (Vector2){mutebutton.x + 15.0f, mutebutton.y + 11.0f}, 28, 2, mutebuttoncolor);
    }
    else if (ctx->currentstate == GAME_HOWTOPLAY)
    {
        // background
        Rectangle menusource = {0.0f, 0.0f, (float)ctx->menubackground.width, (float)ctx->menubackground.height};
        Rectangle menudest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
        DrawTexturePro(ctx->menubackground, menusource, menudest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

        // dim overlay and panel
        Color backgrounddim = {0, 0, 0, 150};
        DrawRectangle(0, 0, WIDTH, HEIGHT, backgrounddim);
        Rectangle howtoplaypanel = {WIDTH / 2 - 400, HEIGHT / 2 - 250, 800, 500};
        DrawRectangleRounded(howtoplaypanel, 0.05f, 8, (Color){245, 235, 210, 255});

        // close button
        Rectangle crossbutton = {WIDTH / 2.0f + 340.0f, HEIGHT / 2.0f - 240.0f, 40.0f, 40.0f};
        Color crossbuttoncolor = CheckCollisionPointRec(mouseposition, crossbutton) ? GOLD : BLACK;
        DrawTextEx(ctx->customfont, "X", (Vector2){WIDTH / 2.0f + 350.0f, HEIGHT / 2.0f - 235.0f}, 32, 2, crossbuttoncolor);

        // instructions text
        const char *instructions[] = {
            "AIM your bow by moving the mouse.",
            "Click and HOLD to pull back the string.",
            "RELEASE to fire your arrow!",
            "",
            "Each balloon is worth 10 points.",
            "Watch out for special balloons:",
            "GOLD balloons give bonus +2 arrows.",
            "POP balloons must be popped before they escape, otherwise",
            "you will lose arrows!",
            "DANGER balloons end your game instantly!",
            "You start with 10 arrows. Best of luck!"};
        int numberoflines = sizeof(instructions) / sizeof(instructions[0]);
        for (int i = 0; i < numberoflines; i++)
        {
            float linepositiony = HEIGHT / 2.0f - 200.0f + (i * 40.0f);
            Color textcolor = (Color){60, 38, 22, 255};
            DrawTextEx(ctx->customfont, instructions[i], (Vector2){WIDTH / 2.0f - 380.0f, linepositiony}, 32, 2, textcolor);
        }
    }
    else if (ctx->currentstate == GAME_LEADERBOARD)
    {
        // background
        Rectangle menusource = {0.0f, 0.0f, (float)ctx->menubackground.width, (float)ctx->menubackground.height};
        Rectangle menudest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
        DrawTexturePro(ctx->menubackground, menusource, menudest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

        // dim overlay and panel
        Color backgrounddim = {0, 0, 0, 150};
        DrawRectangle(0, 0, WIDTH, HEIGHT, backgrounddim);
        Rectangle highestscorepanel = {WIDTH / 2 - 400, HEIGHT / 2 - 250, 800, 500};
        DrawRectangleRounded(highestscorepanel, 0.05f, 8, (Color){245, 235, 210, 255});

        // close button
        Rectangle crossbutton = {WIDTH / 2.0f + 340.0f, HEIGHT / 2.0f - 240.0f, 40.0f, 40.0f};
        Color crossbuttoncolor = CheckCollisionPointRec(mouseposition, crossbutton) ? GOLD : BLACK;
        DrawTextEx(ctx->customfont, "X", (Vector2){WIDTH / 2.0f + 350.0f, HEIGHT / 2.0f - 235.0f}, 32, 2, crossbuttoncolor);

        // leaderboard title and score list
        const char *titletext = "LEADERBOARD";
        DrawTextEx(ctx->customfont, titletext, (Vector2){WIDTH / 2.0f - 160.0f, HEIGHT / 2.0f - 150.0f}, 54, 2, (Color){60, 38, 22, 255});

        for (int i = 0; i < MAXLEADERBOARD; i++)
        {
            const char *ranktext = TextFormat("%d. %d", i + 1, ctx->leaderboard[i]);
            DrawTextEx(ctx->customfont, ranktext, (Vector2){WIDTH / 2.0f - 100.0f, HEIGHT / 2.0f - 60.0f + i * 55.0f}, 42, 2, (Color){60, 38, 22, 255});
        }
    }
    else if (ctx->currentstate == GAME_CREDITS)
    {
        // background
        Rectangle menusource = {0.0f, 0.0f, (float)ctx->menubackground.width, (float)ctx->menubackground.height};
        Rectangle menudest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
        DrawTexturePro(ctx->menubackground, menusource, menudest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

        // dim overlay and panel
        Color backgrounddim = {0, 0, 0, 150};
        DrawRectangle(0, 0, WIDTH, HEIGHT, backgrounddim);
        Rectangle creditspanel = {WIDTH / 2 - 450, HEIGHT / 2 - 350, 900, 700};
        DrawRectangleRounded(creditspanel, 0.05f, 8, (Color){245, 235, 210, 255});

        // close button
        Rectangle crossbutton = {WIDTH / 2.0f + 390.0f, HEIGHT / 2.0f - 340.0f, 40.0f, 40.0f};
        Color crossbuttoncolor = CheckCollisionPointRec(mouseposition, crossbutton) ? GOLD : BLACK;
        DrawTextEx(ctx->customfont, "X", (Vector2){WIDTH / 2.0f + 400.0f, HEIGHT / 2.0f - 335.0f}, 32, 2, crossbuttoncolor);

        // credits text lines
        const char *creditslines[] = {
            "CREDITS",
            "",
            "Music: Menu Theme - Nintendo Land Soundtrack",
            "Music: Gameplay Theme - Sounds Monarch on YouTube",
            "",
            "SFX: Bow Shot - Freesound.org",
            "SFX: Balloon Pop - Freesound.org",
            "SFX: Game Over - Freesound.org",
            "SFX: Button Click - Freesound.org",
            "",
            "Font: Carnivalee Freakshow font by Chris Hansen",
            "",
            "Art: Bow, Arrow, Balloons, Boy -opengameart.org",
            "Background images: Gemini + Canva",
            "",
            "Special Thanks: to our Supervisor Zim Mim Siddiqee Sir",
            "",
            "Made by AKAID ADNAN & SHAFQAT NAVEED",
        };
        int creditslinecount = sizeof(creditslines) / sizeof(creditslines[0]);
        for (int i = 0; i < creditslinecount; i++)
        {
            float linepositiony = HEIGHT / 2.0f - 320.0f + (i * 36.0f);
            Color textcolor = (Color){60, 38, 22, 255};
            DrawTextEx(ctx->customfont, creditslines[i], (Vector2){WIDTH / 2.0f - 400.0f, linepositiony}, 28, 2, textcolor);
        }
    }
    else if (ctx->currentstate == GAME_PLAYING)
    {
        // background
        Rectangle bgsource = {0.0f, 0.0f, (float)ctx->background.width, (float)ctx->background.height};
        Rectangle bgdest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
        DrawTexturePro(ctx->background, bgsource, bgdest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

        // draw boy sprite
        float boywidth = 396.0f;
        float boyheight = 528.0f;
        Vector2 boypos = {200.0f, 670.0f};
        Rectangle boysource = {0.0f, 0.0f, (float)ctx->boytexture.width, (float)ctx->boytexture.height};
        Rectangle boydest = {boypos.x, boypos.y, boywidth, boyheight};
        DrawTexturePro(ctx->boytexture, boysource, boydest, (Vector2){boywidth / 2.0f, boyheight / 2.0f}, 0.0f, WHITE);

        // bow, arrow drawing variables
        Vector2 arrowpivot = {280.0f, 590.0f};
        float aimangle = 0.0f;
        Vector2 aimdirection = {1.0f, 0.0f};
        float mousepointerangle = atan2f(mouseposition.y - arrowpivot.y, mouseposition.x - arrowpivot.x);
        if (mousepointerangle > PI / 4.0)
            aimangle = PI / 4.0;
        else if (mousepointerangle < -PI / 4.0)
            aimangle = -PI / 4.0;
        else
            aimangle = mousepointerangle;
        aimdirection = (Vector2){cosf(aimangle), sinf(aimangle)};

        float bowwidth = 220.0f;
        float bowheight = 220.0f;
        Rectangle bowsource = {0.0f, 0.0f, (float)ctx->bowimage.width, (float)ctx->bowimage.height};
        Rectangle bowdest = {arrowpivot.x, arrowpivot.y, bowwidth, bowheight};
        Vector2 boworigin = {bowwidth / 2.0f, bowheight / 2.0f};
        DrawTexturePro(ctx->bowimage, bowsource, bowdest, boworigin, aimangle * RAD2DEG, WHITE);

        Vector2 bowstringtop = Vector2Add(arrowpivot, Vector2Rotate((Vector2){0.0, 90.0f}, aimangle));
        Vector2 bowstringbottom = Vector2Add(arrowpivot, Vector2Rotate((Vector2){0.0f, -90.0f}, aimangle));
        Vector2 pullpoint = Vector2Subtract(arrowpivot, Vector2Scale(aimdirection, ctx->pulldistance));
        DrawLineEx(bowstringbottom, pullpoint, 3.5f, LIGHTGRAY);
        DrawLineEx(bowstringtop, pullpoint, 3.5f, LIGHTGRAY);

        float arrowwidth = 110.0f;
        float arrowheight = 45.0f;
        if (ctx->gameover == false && ctx->arrow.active == false && ctx->arrowsleft > 0)
        {
            Vector2 pullpoint = Vector2Subtract(arrowpivot, Vector2Scale(aimdirection, ctx->pulldistance * 0.5f));
            Rectangle arrowsource = {0.0f, 0.0f, (float)ctx->arrowimage.width, (float)ctx->arrowimage.height};
            Rectangle arrowdest = {pullpoint.x, pullpoint.y, arrowwidth, arrowheight};
            Vector2 arroworigin = {arrowwidth / 2.0f, arrowheight / 2.0f};
            DrawTexturePro(ctx->arrowimage, arrowsource, arrowdest, arroworigin, aimangle * RAD2DEG, WHITE);
        }
        else if (ctx->gameover == false && ctx->arrow.active == true)
        {
            float arrowangle = atan2f(ctx->arrow.velocity.y, ctx->arrow.velocity.x);
            Rectangle arrowsource = {0.0f, 0.0f, (float)ctx->arrowimage.width, (float)ctx->arrowimage.height};
            Rectangle arrowdest = {ctx->arrow.position.x, ctx->arrow.position.y, arrowwidth, arrowheight};
            Vector2 arroworigin = {arrowwidth / 2.0f, arrowheight / 2.0f};
            DrawTexturePro(ctx->arrowimage, arrowsource, arrowdest, arroworigin, arrowangle * RAD2DEG, WHITE);
        }

        // balloon drawing
        for (int i = 0; i < MAXBALLOONS; i++)
        {
            if (ctx->balloons[i].active == true)
            {
                if (ctx->balloons[i].danger == true)
                {
                    Rectangle balloonsource = {0, 0, (float)ctx->dangerballoon.width, (float)ctx->dangerballoon.height};
                    Rectangle balloondest = {ctx->balloons[i].position.x, ctx->balloons[i].position.y, ctx->dangerwidth, ctx->dangerheight};
                    Vector2 balloonorigin = {ctx->dangerwidth / 2.0f, ctx->dangerheight / 2.0f};
                    DrawTexturePro(ctx->dangerballoon, balloonsource, balloondest, balloonorigin, 0.0f, WHITE);
                }
                else if (ctx->balloons[i].mustpop == true)
                {
                    Rectangle balloonsource = {0, 0, (float)ctx->mustpopballoon.width, (float)ctx->mustpopballoon.height};
                    Rectangle balloondest = {ctx->balloons[i].position.x, ctx->balloons[i].position.y, ctx->mustpopwidth, ctx->mustpopheight};
                    Vector2 balloonorigin = {ctx->mustpopwidth / 2.0f, ctx->mustpopheight / 2.0f};
                    DrawTexturePro(ctx->mustpopballoon, balloonsource, balloondest, balloonorigin, 0.0f, WHITE);
                }
                else
                {
                    Texture2D balloontexture;
                    if (ctx->balloons[i].gold)
                        balloontexture = ctx->arrowballoon;
                    else
                        balloontexture = ctx->normalballoons[ctx->balloons[i].index];

                    Rectangle balloonsource = {0, 0, (float)balloontexture.width, (float)balloontexture.height};
                    Rectangle balloondest = {ctx->balloons[i].position.x, ctx->balloons[i].position.y, ctx->normalballoonwidth, ctx->normalballoonheight};
                    Vector2 balloonorigin = {ctx->normalballoonwidth / 2.0f, ctx->normalballoonheight / 2.0f};
                    DrawTexturePro(balloontexture, balloonsource, balloondest, balloonorigin, 0.0f, WHITE);
                }
            }
        }

        // drawing texts
        DrawTextEx(ctx->customfont, TextFormat("SCORE: %d", ctx->score), (Vector2){32, 23}, 42, 2, BLACK);
        DrawTextEx(ctx->customfont, TextFormat("SCORE: %d", ctx->score), (Vector2){30, 25}, 42, 2, WHITE);
        DrawTextEx(ctx->customfont, TextFormat("ARROWS: %d", ctx->arrowsleft), (Vector2){32, 73}, 42, 2, BLACK);
        DrawTextEx(ctx->customfont, TextFormat("ARROWS: %d", ctx->arrowsleft), (Vector2){30, 75}, 42, 2, GOLD);
        float currentballoonspeed = 150.0f + ctx->score * 0.75f;
        DrawTextEx(ctx->customfont, TextFormat("BALLOON SPEED: %.2f", currentballoonspeed), (Vector2){32, 123}, 42, 2, BLACK);
        DrawTextEx(ctx->customfont, TextFormat("BALLOON SPEED: %.2f", currentballoonspeed), (Vector2){30, 125}, 42, 2, WHITE);

        DrawTextEx(ctx->customfont, TextFormat("ANGLE: %.2f", -(aimangle * RAD2DEG)), (Vector2){32, 671}, 42, 2, BLACK);
        DrawTextEx(ctx->customfont, TextFormat("ANGLE: %.2f", -(aimangle * RAD2DEG)), (Vector2){30, 673}, 42, 2, WHITE);
        DrawTextEx(ctx->customfont, TextFormat("LAUNCH SPEED: %.2f", ctx->launchspeed), (Vector2){32, 721}, 42, 2, BLACK);
        DrawTextEx(ctx->customfont, TextFormat("LAUNCH SPEED: %.2f", ctx->launchspeed), (Vector2){30, 723}, 42, 2, GOLD);

        // mute/unmute button
        Rectangle mutebutton = {WIDTH - 190.0f, HEIGHT - 80.0f, 160.0f, 50.0f};
        bool mutehovered = CheckCollisionPointRec(mouseposition, mutebutton);
        Color mutebuttoncolor = mutehovered ? GOLD : RAYWHITE;
        DrawRectangleRounded(mutebutton, 0.3f, 4, (Color){0, 0, 0, 150});
        DrawRectangleRoundedLines(mutebutton, 0.3f, 16, mutebuttoncolor);
        DrawTextEx(ctx->customfont, ctx->ismuted ? "UNMUTE" : "MUTE", (Vector2){mutebutton.x + 15.0f, mutebutton.y + 11.0f}, 28, 2, mutebuttoncolor);

        // drawing scorepopups
        for (int i = 0; i < MAXPOPUPS; i++)
        {
            if (ctx->scorepopup[i].active)
            {
                DrawTextEx(ctx->customfont, TextFormat("+%d", ctx->scorepopup[i].value), ctx->scorepopup[i].position, 65, 2, BLACK);
                DrawTextEx(ctx->customfont, TextFormat("+%d", ctx->scorepopup[i].value), (Vector2){ctx->scorepopup[i].position.x + 2, ctx->scorepopup[i].position.y + 2}, 65, 2, GOLD);
            }
        }

        // drawing arrowpopups
        for (int i = 0; i < MAXARROWPOPUPS; i++)
        {
            if (ctx->arrowpopup[i].active)
            {
                Color popupcolor = (ctx->arrowpopup[i].value < 0) ? RED : GREEN;
                const char *arrowpopuptext = (ctx->arrowpopup[i].value < 0) ? TextFormat("%d ARROWS", ctx->arrowpopup[i].value) : TextFormat("+%d ARROWS", ctx->arrowpopup[i].value);

                DrawTextEx(ctx->customfont, arrowpopuptext, ctx->arrowpopup[i].position, 50, 2, BLACK);
                DrawTextEx(ctx->customfont, arrowpopuptext, (Vector2){ctx->arrowpopup[i].position.x + 2, ctx->arrowpopup[i].position.y + 2}, 50, 2, popupcolor);
            }
        }

        // gameover screen
        if (ctx->gameover == true)
        {
            Rectangle gameoversource = {0.0f, 0.0f, (float)ctx->gameovertexture.width, (float)ctx->gameovertexture.height};
            Rectangle gameoverdest = {415.0f, 180.0f, ctx->gameovertexture.width, ctx->gameovertexture.height};
            Vector2 gameoverorigin = {0.0f, 0.0f};
            DrawTexturePro(ctx->gameovertexture, gameoversource, gameoverdest, gameoverorigin, 0.0f, WHITE);

            Rectangle textpanel = {540.0f, 480.0f, 560.0f, 250.0f};
            Color paneldim = {0, 0, 0, 140};
            DrawRectangleRounded(textpanel, 0.2f, 8, paneldim);

            const char *restarttext = "PRESS R TO RESTART";
            DrawTextEx(ctx->customfont, restarttext, (Vector2){600.0f, 500.0f}, 48.0f, 2, BLACK);
            DrawTextEx(ctx->customfont, restarttext, (Vector2){598.0f, 498.0f}, 48.0f, 2, GOLD);

            const char *scoretext = TextFormat("YOUR SCORE: %d", ctx->score);
            DrawTextEx(ctx->customfont, scoretext, (Vector2){600.0f, 600.0f}, 48.0f, 2, BLACK);
            DrawTextEx(ctx->customfont, scoretext, (Vector2){598.0f, 598.0f}, 48.0f, 2, WHITE);

            const char *highscoretext = TextFormat("HIGHEST SCORE: %d", ctx->leaderboard[0]);
            DrawTextEx(ctx->customfont, highscoretext, (Vector2){600.0f, 650.0f}, 48.0f, 2, BLACK);
            DrawTextEx(ctx->customfont, highscoretext, (Vector2){598.0f, 648.0f}, 48.0f, 2, GOLD);

            Rectangle menubutton = {WIDTH - 270.0f, 40.0f, 220.0f, 100.0f};
            bool ishovered = CheckCollisionPointRec(mouseposition, menubutton);
            Color buttoncolor = ishovered ? GOLD : RAYWHITE;
            DrawRectangleRounded(menubutton, 0.3f, 4, (Color){0, 0, 0, 150});
            DrawRectangleRoundedLines(menubutton, 0.3f, 16, buttoncolor);
            DrawTextEx(ctx->customfont, "MAIN MENU", (Vector2){menubutton.x + 20.0f, menubutton.y + 20.0f}, 40.0f, 2, buttoncolor);
        }
    }

    EndDrawing();
}

int main(void)
{
    // window and audio setup
    InitWindow(WIDTH, HEIGHT, "HIT 'EM ALL");
    InitAudioDevice();
    SetRandomSeed(time(NULL));
    SetTargetFPS(60);

    GameContext ctx = {0};

    // initialize game state
    ctx.currentstate = GAME_MENU;

    // loading all audio, textures, fonts
    ctx.bgmusic = LoadMusicStream("assets/audio/Game Window.mp3");
    ctx.menumusic = LoadMusicStream("assets/audio/Music (Menu Screen).mp3");
    ctx.shootsound = LoadSound("assets/audio/Gun shooting.ogg");
    ctx.popsound = LoadSound("assets/audio/Balloon Pop.mp3");
    ctx.gameoversound = LoadSound("assets/audio/game over.mp3");
    ctx.clicksound = LoadSound("assets/audio/Button Click.wav");
    ctx.minusarrowssound = LoadSound("assets/audio/Minus arrows.wav");

    PlayMusicStream(ctx.menumusic);

    ctx.menubackground = LoadTexture("assets/sprites/menuscreen.png");
    ctx.background = LoadTexture("assets/sprites/gamescreen.png");
    ctx.gameovertexture = LoadTexture("assets/sprites/gameover.png");
    ctx.bowimage = LoadTexture("assets/sprites/bow.png");
    ctx.arrowimage = LoadTexture("assets/sprites/arrow.png");
    ctx.arrowballoon = LoadTexture("assets/sprites/arrowballoon.png");
    ctx.dangerballoon = LoadTexture("assets/sprites/dangerballoon.png");
    ctx.mustpopballoon = LoadTexture("assets/sprites/mustpopballoon.png");
    for (int i = 0; i < NORMALBALLOONSNUM; i++)
    {
        ctx.normalballoons[i] = LoadTexture(TextFormat("assets/sprites/normalballoon%d.png", i + 1));
    }
    ctx.boytexture = LoadTexture("assets/sprites/boy.png");

    ctx.customfont = LoadFontEx("assets/fonts/Carnival Font.ttf", 96, NULL, 0);

    // init spawnpoints, arrow, balloons
    for (int i = 0; i < SPAWNPOINTS; i++)
    {
        ctx.spawnpoints[i] = (Vector2){1000.0f + i * 130.0f, HEIGHT + 50.0f};
    }

    // array init
    ctx.arrow = (Arrow){0};
    // ctx.balloons, ctx.scorepopup, ctx.arrowpopup, ctx.leaderboard already zeroed by GameContext ctx = {0};

    // init game variables
    ctx.score = 0;
    ctx.arrowsleft = 10;
    ctx.gravity = 1250.0f;
    ctx.currenttimer = 0.0f;
    ctx.spawninterval = 1.5f;
    ctx.pulldistance = 0.0f;
    ctx.launchspeed = 0.0f;
    ctx.gameover = false;
    ctx.ismuted = false;
    ctx.requestexit = false;

    // balloon sizes
    ctx.normalballoonwidth = 140.0f;
    ctx.normalballoonheight = 140.0f;
    ctx.balloonradius = ctx.normalballoonwidth * 0.4f;

    ctx.dangerwidth = 330.0f;
    ctx.dangerheight = 180.0f;
    ctx.dangerradius = ctx.dangerwidth * 0.25f;

    ctx.mustpopwidth = 180.0f;
    ctx.mustpopheight = 180.0f;
    ctx.mustpopradius = ctx.mustpopwidth * 0.4f;

    // reading scores from file to put in leaderboard
    FILE *leaderboardfile = fopen("leaderboard.txt", "r");
    if (leaderboardfile != NULL)
    {
        for (int i = 0; i < MAXLEADERBOARD; i++)
        {
            if (fscanf(leaderboardfile, "%d", &ctx.leaderboard[i]) != 1)
                break;
        }
        fclose(leaderboardfile);
    }

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop_arg(UpdateDrawFrame, &ctx, 0, 1);
#else
    while (!WindowShouldClose() && !ctx.requestexit)
    {
        UpdateDrawFrame(&ctx);
    }
#endif

#if !defined(PLATFORM_WEB)
    // all unloading
    UnloadTexture(ctx.menubackground);
    UnloadTexture(ctx.background);
    for (int i = 0; i < NORMALBALLOONSNUM; i++)
    {
        UnloadTexture(ctx.normalballoons[i]);
    }

    UnloadTexture(ctx.boytexture);
    UnloadTexture(ctx.arrowballoon);
    UnloadTexture(ctx.dangerballoon);
    UnloadTexture(ctx.mustpopballoon);
    UnloadTexture(ctx.gameovertexture);
    UnloadTexture(ctx.bowimage);
    UnloadTexture(ctx.arrowimage);
    UnloadFont(ctx.customfont);
    UnloadSound(ctx.shootsound);
    UnloadSound(ctx.popsound);
    UnloadSound(ctx.minusarrowssound);
    UnloadMusicStream(ctx.bgmusic);
    UnloadMusicStream(ctx.menumusic);
    UnloadSound(ctx.gameoversound);
    UnloadSound(ctx.clicksound);
    CloseAudioDevice();
    CloseWindow();
#endif

    return 0;
}
