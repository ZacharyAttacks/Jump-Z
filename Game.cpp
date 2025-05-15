#include "DirectX.h"
#include <ctime>
#include <vector>
#include <mmsystem.h>
#include <windows.h>
#include <algorithm>
#include <SpriteFont.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
std::unique_ptr<SpriteFont> spriteFontDXTK;
#include <SpriteBatch.h>
#include <DirectXHelpers.h>
std::unique_ptr<DirectX::SpriteFont> spriteFont;
#pragma comment(lib, "winmm.lib")
const float GRAVITY = 0.3f;
const float JUMP_CHARGE_SPEED = 0.5f;
const float MAX_JUMP_POWER = 12.0f;
const float MIN_JUMP_POWER = 10.0f;
const int PLAYER_SPEED = 2;
const int GRID_CELL_SIZE = 50;
const int GRID_COLS = 16;
const int GRID_ROWS = 12;
const int TARGET_FPS = 60;
const int FRAME_DELAY = 1000 / TARGET_FPS;
const int SCREEN_SWITCH_BUFFER = 45;
enum GameState { STATE_MENU, STATE_PLAY, STATE_PAUSE, STATE_WIN, STATE_CUSTOMSETUP };
GameState gameState = STATE_MENU;
bool gameover = false;
Model2D background, platformTile, throne;
Model2D playerIdle, playerWalkLeft1, playerWalkLeft2, playerWalkRight1, playerWalkRight2;
Model2D playerJumpLeft, playerJumpRight;
Model2D easyButton;
Model2D mediumButton;
Model2D hardButton;
Model2D nextButton;
Model2D legDayButton;
Model2D jumpChargeBars[11];
Model2D customButton;
Model2D returnButton;
std::vector<Model2D> platforms;
std::vector<Model2D> thrones;
int currentScreen = 0;
int totalScreens = 3;
int customScreens = 3;
bool facingLeft = false;
bool chargingJump = false;
bool isJumping = false;
bool onGround = false;
bool mouseClicked = false;
bool isChargingJump = false;
bool walkAnimToggle = false;
float jumpPower = 0;
float velocityX = 0;
float velocityY = 0;
float timer = 0.0f;
int lastPlatformIndex = -1;
struct MovingPlatform {
    int index;   
    int originalX;
    int direction;
    int offset;
};
std::vector<MovingPlatform> movingPlatforms;
const int MOVING_PLATFORM_RANGE = 70;
const int MOVING_PLATFORM_SPEED = 1.5;
int playerX = 400;
int playerY = 0;
RECT camera = { 0, 0, Width, Height };
clock_t startTime;
LPCWSTR jumpSoundPath = L"jump.wav";
LPCWSTR throneSoundPath = L"victory.wav";
wchar_t finalTimeStr[64] = L"";
float pausedTime = 0.0f;
clock_t pauseStart;
void GeneratePlatforms();
void ResetGame();
bool RectsCollide(const RECT& a, const RECT& b)
{
    return !(a.right < b.left || a.left > b.right || a.bottom < b.top || a.top > b.bottom);
}
bool Game_Init(HWND hwnd)
{
    srand((unsigned int)time(0));
    InitD3D(hwnd);
    InitInput(hwnd);
    InitSound();
    spriteBatch = std::make_unique<DirectX::SpriteBatch>(devcon);
    spriteFontDXTK = std::make_unique<SpriteFont>(dev, L"arial32.spritefont");
    background = CreateModel2D(L"brickbackground.png");
    platformTile = CreateModel2D(L"tile.png");
    platformTile.frame_width = 32;
    platformTile.frame_height = 32;
    throne = CreateModel2D(L"throne.png");
    playerIdle = CreateModel2D(L"player_idle.png");
    playerWalkLeft1 = CreateModel2D(L"player_walkingleft1.png");
    playerWalkLeft2 = CreateModel2D(L"player_walkingleft2.png");
    playerWalkRight1 = CreateModel2D(L"player_walkingright1.png");
    playerWalkRight2 = CreateModel2D(L"player_walkingright2.png");
    playerJumpLeft = CreateModel2D(L"player_jumpingleft.png");
    playerJumpRight = CreateModel2D(L"player_jumpingright.png");
    easyButton = CreateModel2D(L"Easy.png");
    mediumButton = CreateModel2D(L"Medium.png");
    hardButton = CreateModel2D(L"Hard.png");
    nextButton = CreateModel2D(L"Next.png");
    legDayButton = CreateModel2D(L"LegDay.png");
    customButton = CreateModel2D(L"Custom.png");
    returnButton = CreateModel2D(L"returnButton.png");
    for (int i = 0; i <= 10; i++)
    {
        std::wstring filename = L"jumpchargebar" + std::to_wstring(i) + L".png";
        jumpChargeBars[i] = CreateModel2D(filename.c_str());
        jumpChargeBars[i].frame_width = 42;
        jumpChargeBars[i].frame_height = 12;
    }
    return true;
}

void Game_Run()
{
    static DWORD frameStart = timeGetTime();
    DWORD frameTime = timeGetTime() - frameStart;
    if (frameTime < FRAME_DELAY) return;
    frameStart = timeGetTime();
    static bool wasEscapePressed = false;
    auto kb = keyboard->GetState();
    bool currentEscapePressed = kb.Escape;
    ClearScreen();
    spriteBatch->Begin();
    switch (gameState)
    {
    case STATE_MENU:
        DrawModel2D(background);
        for (int y = 0; y < Height; y += background.frame_height)
            for (int x = 0; x < Width; x += background.frame_width)
            {
                background.x = x;
                background.y = y;
                DrawModel2D(background, camera);
            }
        easyButton.x = Width / 2 - easyButton.frame_width / 2;
        easyButton.y = 150;
        DrawModel2D(easyButton);
        mediumButton.x = Width / 2 - mediumButton.frame_width / 2;
        mediumButton.y = 250;
        DrawModel2D(mediumButton);
        hardButton.x = Width / 2 - hardButton.frame_width / 2;
        hardButton.y = 350;
        DrawModel2D(hardButton);
        legDayButton.x = Width / 2 - legDayButton.frame_width / 2;
        legDayButton.y = 450;
        DrawModel2D(legDayButton);
        customButton.x = Width / 2 - customButton.frame_width / 2;
        customButton.y = 550;
        DrawModel2D(customButton);
        break;

    case STATE_CUSTOMSETUP:
    {
        background.x = 0;
        background.y = 0;
        DrawModel2D(background);
        static bool upHeld = false;
        static bool downHeld = false;
        static DWORD lastChangeTime = 0;
        static const DWORD repeatDelay = 300;
        static const DWORD repeatRate = 500;
        DWORD now = timeGetTime();
        auto kb = keyboard->GetState();
        static bool upPressed = false;
        static bool downPressed = false;
        static bool enterPressed = false;
        if (kb.Up)
        {
            if (!upHeld || now - lastChangeTime >= repeatRate)
            {
                if (customScreens >= 999)
                    customScreens = 3;
                else
                    customScreens++;
                lastChangeTime = now;
            }
            upHeld = true;
        }
        else
        {
            upHeld = false;
        }
        if (kb.Down)
        {
            if (!downHeld)
            {
                if (customScreens <= 3)
                    customScreens = 999;
                else
                    customScreens--;
                lastChangeTime = now;
            }
            else if (now - lastChangeTime >= repeatRate)
            {
                if (customScreens <= 3)
                    customScreens = 999;
                else
                    customScreens--;
                lastChangeTime = now;
            }
            downHeld = true;
        }
        else
        {
            downHeld = false;
        }

        if (kb.Enter && !enterPressed) {
            totalScreens = static_cast<int>(2.4f * customScreens - 4);
            ResetGame();
        }
        upPressed = kb.Up;
        downPressed = kb.Down;
        enterPressed = kb.Enter;
        wchar_t label1[] = L"Custom";
        DirectX::XMVECTOR label1Size = spriteFontDXTK->MeasureString(label1);
        float label1X = (800 - DirectX::XMVectorGetX(label1Size)) / 2.0f;
        spriteFontDXTK->DrawString(spriteBatch.get(), label1, DirectX::XMFLOAT2(label1X, 120), Colors::Black);
        wchar_t numberStr[16];
        swprintf_s(numberStr, L"\n%d", customScreens);
        DirectX::XMVECTOR numberSize = spriteFontDXTK->MeasureString(numberStr);
        float numberX = (800 - DirectX::XMVectorGetX(numberSize)) / 2.0f;
        spriteFontDXTK->DrawString(spriteBatch.get(), numberStr, DirectX::XMFLOAT2(numberX, 160), Colors::Yellow);
        wchar_t line2[] = L"\nUp/Down to change";
        DirectX::XMVECTOR size2 = spriteFontDXTK->MeasureString(line2);
        spriteFontDXTK->DrawString(spriteBatch.get(), line2,
            DirectX::XMFLOAT2((800 - DirectX::XMVectorGetX(size2)) / 2.0f, 210), Colors::White);
        wchar_t line3[] = L"\nEnter to start";
        DirectX::XMVECTOR size3 = spriteFontDXTK->MeasureString(line3);
        spriteFontDXTK->DrawString(spriteBatch.get(), line3,
            DirectX::XMFLOAT2((800 - DirectX::XMVectorGetX(size3)) / 2.0f, 250), Colors::White);
        wchar_t line4[] = L"\nEach screen is about";
        DirectX::XMVECTOR size4 = spriteFontDXTK->MeasureString(line4);
        spriteFontDXTK->DrawString(spriteBatch.get(), line4,
            DirectX::XMFLOAT2((800 - DirectX::XMVectorGetX(size4)) / 2.0f, 300), Colors::White);
        wchar_t line5[] = L"\n4/5ths of the number you input";
        DirectX::XMVECTOR size5 = spriteFontDXTK->MeasureString(line5);
        spriteFontDXTK->DrawString(spriteBatch.get(), line5,
            DirectX::XMFLOAT2((800 - DirectX::XMVectorGetX(size5)) / 2.0f, 340), Colors::White);
        wchar_t line6[] = L"\nIf you want 40 screens";
        DirectX::XMVECTOR size6 = spriteFontDXTK->MeasureString(line6);
        spriteFontDXTK->DrawString(spriteBatch.get(), line6,
            DirectX::XMFLOAT2((800 - DirectX::XMVectorGetX(size6)) / 2.0f, 380), Colors::White);
        wchar_t line7[] = L"\nthen enter 50";
        DirectX::XMVECTOR size7 = spriteFontDXTK->MeasureString(line7);
        spriteFontDXTK->DrawString(spriteBatch.get(), line7,
            DirectX::XMFLOAT2((800 - DirectX::XMVectorGetX(size7)) / 2.0f, 420), Colors::White);
        break;
    }

    case STATE_PLAY: {
        if (currentEscapePressed && !wasEscapePressed)
        {
            gameState = STATE_PAUSE;
            pauseStart = clock();
            spriteBatch->End();
            wasEscapePressed = true;
            return;
        }
        background.x = camera.left;
        background.y = camera.top;
        DrawModel2D(background, camera);
        timer = ((float)(clock() - startTime) / CLOCKS_PER_SEC) - pausedTime;
        bool currentSpacePressed = kb.Space;
        if (onGround)
        {
            if (currentSpacePressed)
            {
                isChargingJump = true;
                if (jumpPower < MAX_JUMP_POWER)
                    jumpPower += JUMP_CHARGE_SPEED;
            }
            else if (!currentSpacePressed && isChargingJump)
            {
                if (jumpPower < MIN_JUMP_POWER)
                    jumpPower = MIN_JUMP_POWER;

                velocityY = -jumpPower;
                isJumping = true;
                onGround = false;
                isChargingJump = false;
                jumpPower = 0;

                PlaySoundW(jumpSoundPath, NULL, SND_FILENAME | SND_ASYNC);
            }
        }
        velocityX = 0;
        if (kb.A) { velocityX = -PLAYER_SPEED; facingLeft = true; walkAnimToggle = !walkAnimToggle; }
        if (kb.D) { velocityX = PLAYER_SPEED; facingLeft = false; walkAnimToggle = !walkAnimToggle; }
        playerX += (int)velocityX;
        if (!onGround)
        {
            velocityY += GRAVITY;
            playerY += (int)velocityY;
        }
        onGround = false;
        for (auto& plat : platforms)
        {
            RECT playerRect = { playerX, playerY, playerX + playerIdle.frame_width, playerY + playerIdle.frame_height };
            RECT platRect = { plat.x, plat.y, plat.x + plat.frame_width, plat.y + plat.frame_height };
            if (RectsCollide(playerRect, platRect))
            {
                int playerBottom = playerY + playerIdle.frame_height;
                int playerTop = playerY;
                int playerRight = playerX + playerIdle.frame_width;
                int playerLeft = playerX;
                int platTop = plat.y;
                int platBottom = plat.y + plat.frame_height;
                int platLeft = plat.x;
                int platRight = plat.x + plat.frame_width;
                int overlapBottom = playerBottom - platTop;
                int overlapTop = platBottom - playerTop;
                int overlapLeft = playerRight - platLeft;
                int overlapRight = platRight - playerLeft;
                bool fromTop = velocityY >= 0 && playerBottom - velocityY <= platTop;
                bool fromBottom = velocityY < 0 && playerTop - velocityY >= platBottom;
                bool fromLeft = velocityX > 0 && playerRight - velocityX <= platLeft;
                bool fromRight = velocityX < 0 && playerLeft - velocityX >= platRight;
                if (fromTop && overlapBottom < overlapTop && overlapBottom < overlapLeft && overlapBottom < overlapRight)
                {
                    playerY = platTop - playerIdle.frame_height;
                    velocityY = 0;
                    onGround = true;
                    isJumping = false;
                    int platformIndex = &plat - &platforms[0];  
                    if (onGround) lastPlatformIndex = platformIndex;
                }
                else if (fromBottom && overlapTop < overlapBottom && overlapTop < overlapLeft && overlapTop < overlapRight)
                {
                    playerY = platBottom;
                    velocityY = 0;
                }
                else if (fromLeft && overlapLeft < overlapRight && overlapLeft < overlapTop && overlapLeft < overlapBottom)
                {
                    playerX = platLeft - playerIdle.frame_width;
                }
                else if (fromRight && overlapRight < overlapLeft && overlapRight < overlapTop && overlapRight < overlapBottom)
                {
                    playerX = platRight;
                }
            }
        }
        for (auto& mp : movingPlatforms)
        {
            mp.offset += mp.direction * MOVING_PLATFORM_SPEED;
            int nextX = mp.originalX + mp.offset;
            if (nextX < 0 || nextX > Width - platformTile.frame_width)
            {
                mp.offset -= mp.direction * MOVING_PLATFORM_SPEED;
                mp.direction *= -1;
            }
            else if (abs(mp.offset) >= MOVING_PLATFORM_RANGE)
            {
                mp.offset = mp.offset > 0 ? MOVING_PLATFORM_RANGE : -MOVING_PLATFORM_RANGE;
                mp.direction *= -1;
            }
            platforms[mp.index].x = mp.originalX + mp.offset;
            if (onGround && lastPlatformIndex == mp.index)
            {
                playerX += mp.direction * MOVING_PLATFORM_SPEED;
            }
        }
        if (playerY < camera.top - SCREEN_SWITCH_BUFFER && currentScreen < totalScreens - 1)
        {
            currentScreen++;
            camera.top -= Height;
            camera.bottom -= Height;
        }
        else if (playerY + playerIdle.frame_height > camera.bottom + SCREEN_SWITCH_BUFFER && currentScreen > 0)
        {
            currentScreen--;
            camera.top += Height;
            camera.bottom += Height;
        }
        for (auto& plat : platforms) DrawModel2D(plat, camera);
        for (auto& t : thrones) DrawModel2D(t, camera);
        if (playerX < 0) playerX = 0;
        if (playerX > Width - playerIdle.frame_width) playerX = Width - playerIdle.frame_width;
        float elapsed = timer;
        int minutes = (int)(elapsed / 60);
        int seconds = (int)elapsed % 60;
        int milliseconds = (int)((elapsed - (int)elapsed) * 1000);
        wchar_t timeStr[64];
        swprintf_s(timeStr, L"%02d:%02d:%03d", minutes, seconds, milliseconds);
        DirectX::XMVECTOR timeSize = spriteFontDXTK->MeasureString(timeStr);
        float timeWidth = DirectX::XMVectorGetX(timeSize);
        float posY = 630.0f;
        float timeX = (800 - timeWidth) / 2.0f;
        spriteFontDXTK->DrawString(spriteBatch.get(), timeStr, DirectX::XMFLOAT2(timeX, posY), Colors::White);
        wchar_t screenStr[16];
        swprintf_s(screenStr, L"%d", currentScreen + 1);
        DirectX::XMVECTOR screenSize = spriteFontDXTK->MeasureString(screenStr);
        float screenWidth = DirectX::XMVectorGetX(screenSize);
        float screenX = 800.0f - screenWidth - 10.0f;
        spriteFontDXTK->DrawString(spriteBatch.get(), screenStr, DirectX::XMFLOAT2(screenX, posY), Colors::White);
        for (auto& t : thrones)
        {
            RECT playerRect = { playerX, playerY, playerX + playerIdle.frame_width, playerY + playerIdle.frame_height };
            RECT throneRect = { t.x, t.y, t.x + t.frame_width, t.y + t.frame_height };
            if (RectsCollide(playerRect, throneRect))
            {
                float finalTime = timer;
                int min = (int)(finalTime / 60);
                int sec = (int)finalTime % 60;
                int ms = (int)((finalTime - (int)finalTime) * 1000);
                swprintf_s(finalTimeStr, L"FINISHING TIME: %02d:%02d:%03d", min, sec, ms);
                PlaySoundW(throneSoundPath, NULL, SND_FILENAME | SND_ASYNC);
                gameState = STATE_WIN;
                break;
            }
        }
        if (isJumping)
        {
            (facingLeft ? playerJumpLeft : playerJumpRight).x = playerX;
            (facingLeft ? playerJumpLeft : playerJumpRight).y = playerY;
            DrawModel2D((facingLeft ? playerJumpLeft : playerJumpRight), camera);
        }
        else if (kb.A || kb.D)
        {
            Model2D& sprite = facingLeft ? (walkAnimToggle ? playerWalkLeft1 : playerWalkLeft2) : (walkAnimToggle ? playerWalkRight1 : playerWalkRight2);
            sprite.x = playerX; sprite.y = playerY;
            DrawModel2D(sprite, camera);
        }
        else
        {
            playerIdle.x = playerX; playerIdle.y = playerY;
            DrawModel2D(playerIdle, camera);
        }
        if (isChargingJump)
        {
            int chargeStage = (int)((jumpPower / MAX_JUMP_POWER) * 10);
            if (chargeStage < 0) chargeStage = 0;
            if (chargeStage > 10) chargeStage = 10;
            jumpChargeBars[chargeStage].x = playerX + playerIdle.frame_width / 2 - jumpChargeBars[chargeStage].frame_width / 2;
            jumpChargeBars[chargeStage].y = playerY + playerIdle.frame_height + 5;
            DrawModel2D(jumpChargeBars[chargeStage], camera);
        }
        break;
    }

    case STATE_PAUSE:
    {
        background.x = 0;
        background.y = 0;
        DrawModel2D(background);
        returnButton.x = Width / 2 - returnButton.frame_width / 2;
        returnButton.y = 400;
        DrawModel2D(returnButton);
        if (currentEscapePressed && !wasEscapePressed)
        {
            gameState = STATE_PLAY;
            pausedTime += (float)(clock() - pauseStart) / CLOCKS_PER_SEC;
            spriteBatch->End();
            wasEscapePressed = true;
            return;
        }
        DirectX::XMVECTOR textSize = spriteFontDXTK->MeasureString(L"PAUSED");
        float textWidth = DirectX::XMVectorGetX(textSize);
        float posX = (800 - textWidth) / 2.0f;
        float posY = 300.0f;
        spriteFontDXTK->DrawString(spriteBatch.get(), L"PAUSED", DirectX::XMFLOAT2(posX, posY), Colors::White);
        break;
    }

    case STATE_WIN:
        background.x = 0;
        background.y = 0;
        DrawModel2D(background);
        nextButton.x = Width / 2 - nextButton.frame_width / 2;
        nextButton.y = 500;
        DrawModel2D(nextButton);
        {
            DirectX::XMVECTOR winSize = spriteFontDXTK->MeasureString(finalTimeStr);
            float winWidth = DirectX::XMVectorGetX(winSize);
            float winX = (800 - winWidth) / 2.0f;
            float winY = 300.0f;
            spriteFontDXTK->DrawString(spriteBatch.get(), finalTimeStr, DirectX::XMFLOAT2(winX, winY), Colors::White);
        }
        break;
    }
    spriteBatch->End();
    wasEscapePressed = currentEscapePressed;
    swapchain->Present(0, 0);
    auto ms = mouse->GetState();
    if (ms.leftButton && !mouseClicked)
    {
        mouseClicked = true;
        if (gameState == STATE_MENU)
        {
            if (ms.x >= Width / 2 - 175 && ms.x <= Width / 2 + 175)
            {
                if (ms.y >= 150 && ms.y <= 230) { totalScreens = 8; ResetGame(); }
                else if (ms.y >= 250 && ms.y <= 330) { totalScreens = 20; ResetGame(); }
                else if (ms.y >= 350 && ms.y <= 430) { totalScreens = 31; ResetGame(); }
                else if (ms.y >= 450 && ms.y <= 530) { totalScreens = 236; ResetGame(); }
                else if (ms.y >= 530 && ms.y <= 610) { gameState = STATE_CUSTOMSETUP; }
            }
        }
        else if (gameState == STATE_WIN)
        {
            if (ms.x >= Width / 2 - 175 && ms.x <= Width / 2 + 175 && ms.y >= 500 && ms.y <= 580)
            {
                gameState = STATE_MENU;
            }
        }
        else if (gameState == STATE_PAUSE)
        {
            if (ms.x >= returnButton.x && ms.x <= returnButton.x + returnButton.frame_width &&
                ms.y >= returnButton.y && ms.y <= returnButton.y + returnButton.frame_height)
            {
                gameState = STATE_MENU;
            }
        }

    }
    else if (!ms.leftButton)
    {
        mouseClicked = false;
    }
}

void Game_End()
{
    CleanD3D();
}

void ResetGame()
{
    gameState = STATE_PLAY;
    startTime = clock();
    currentScreen = 0;
    camera = { 0, 0, Width, Height };
    playerX = Width / 2 - playerIdle.frame_width / 2;
    playerY = (Height - 100) - playerIdle.frame_height - 1;
    velocityY = 0;
    isJumping = false;
    onGround = false;
    chargingJump = false;
    jumpPower = 0;
    GeneratePlatforms();
    if (playerY > 599 - playerIdle.frame_height) playerY = 599 - playerIdle.frame_height;
}

void GeneratePlatforms()
{
    platforms.clear();
    thrones.clear();
    movingPlatforms.clear();
    const int numPaths = 2;
    std::vector<std::vector<Model2D>> paths(numPaths);
    int stepsPerScreen = 8;
    int totalSteps = stepsPerScreen * totalScreens;
    int pathX[numPaths];
    int currentY[numPaths];
    int leftLimit = 400 - platformTile.frame_width;
    int rightLimit = 400;
    pathX[0] = rand() % (leftLimit + 1);
    pathX[1] = rightLimit + rand() % (Width - rightLimit - platformTile.frame_width);
    currentY[0] = currentY[1] = Height * totalScreens - 100;
    for (int step = 0; step < totalSteps; step++)
    {
        for (int i = 0; i < numPaths; ++i)
        {
            Model2D plat = platformTile;
            plat.x = pathX[i];
            plat.y = currentY[i];
            if (plat.y + plat.frame_height <= 600)
            {
                platforms.push_back(plat);
                if (rand() % 5 == 0)
                {
                    int platformIndex = (int)platforms.size() - 1;
                    movingPlatforms.push_back(MovingPlatform{
                        platformIndex,
                        plat.x,
                        1,
                        0
                        });
                }
                paths[i].push_back(plat);
            }
            int verticalStep = 100 + rand() % 40;
            currentY[i] -= verticalStep;
        }
        for (int i = 0; i < numPaths; ++i)
        {
            int hStep = (rand() % 2 == 0 ? -1 : 1) * (50 + rand() % 40);
            pathX[i] += hStep;
            if (i == 0)
            {
                if (pathX[i] < 0) pathX[i] = 0;
                if (pathX[i] > leftLimit) pathX[i] = leftLimit;
            }
            else
            {
                if (pathX[i] < rightLimit) pathX[i] = rightLimit;
                if (pathX[i] > Width - platformTile.frame_width)
                    pathX[i] = Width - platformTile.frame_width;
            }
        }
        int indices[numPaths];
        for (int i = 0; i < numPaths; ++i) indices[i] = i;
        std::sort(indices, indices + numPaths, [&](int a, int b) { return pathX[a] < pathX[b]; });
        for (int i = 1; i < numPaths; ++i)
        {
            int prev = indices[i - 1];
            int curr = indices[i];
            if (pathX[curr] < pathX[prev] + 100)
                pathX[curr] = pathX[prev] + 100;
            if (curr == 0)
            {
                if (pathX[curr] > leftLimit) pathX[curr] = leftLimit;
            }
            else
            {
                if (pathX[curr] > Width - platformTile.frame_width)
                    pathX[curr] = Width - platformTile.frame_width;
            }
        }
    }
    for (int i = 0; i < numPaths; ++i)
    {
        Model2D* highestPlat = nullptr;
        for (auto& plat : paths[i])
        {
            if (!highestPlat || plat.y < highestPlat->y)
                highestPlat = &plat;
        }
        if (highestPlat)
        {
            Model2D throneCopy = throne;
            throneCopy.x = highestPlat->x + (platformTile.frame_width / 2) - (throne.frame_width / 2);
            throneCopy.y = highestPlat->y - throne.frame_height;
            thrones.push_back(throneCopy);
        }
    }
    for (int x = 0; x < Width; x += platformTile.frame_width)
    {
        Model2D bottomPlat = platformTile;
        bottomPlat.x = x;
        bottomPlat.y = 600;
        platforms.push_back(bottomPlat);
    }
}