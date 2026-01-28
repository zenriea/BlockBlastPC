#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <cmath>

// Game constants
int SCREEN_WIDTH = 600;
int SCREEN_HEIGHT = 800;
const int GRID_SIZE = 8;
const int CELL_SIZE = 50;
int GRID_OFFSET_X = 100;
int GRID_OFFSET_Y = 180;
const int TITLE_BAR_HEIGHT = 40;
bool isFullscreen = false;
int windowedWidth = 600;
int windowedHeight = 800;

// Modern Renkler
const Color BACKGROUND = { 20, 20, 35, 255 };
const Color TITLE_BAR_COLOR = { 15, 15, 28, 255 };
const Color GRID_BG = { 35, 35, 55, 255 };
const Color GRID_LINE = { 50, 50, 75, 255 };
const Color TEXT_COLOR = WHITE;
const Color GOLD_COLOR = { 255, 215, 0, 255 };
const Color ACCENT_COLOR = { 100, 100, 255, 255 };
const Color BUTTON_COLOR = { 60, 60, 100, 255 };
const Color BUTTON_HOVER = { 80, 80, 130, 255 };
const Color PANEL_COLOR = { 30, 30, 50, 230 };

// Blok renkleri - Neon parlak renkler
const Color BLOCK_COLORS[] = {
    { 255, 82, 82, 255 },    // Neon Kırmızı
    { 0, 255, 200, 255 },    // Neon Turkuaz
    { 255, 255, 100, 255 },  // Neon Sarı
    { 100, 180, 255, 255 },  // Neon Mavi
    { 255, 130, 255, 255 },  // Neon Pembe
    { 180, 130, 255, 255 },  // Neon Mor
    { 100, 255, 130, 255 },  // Neon Yeşil
    { 255, 180, 100, 255 },  // Neon Turuncu
};
const int NUM_COLORS = 8;

// Blok şekilleri
struct Shape {
    std::vector<std::vector<int>> cells;
};

const std::vector<Shape> SHAPES = {
    {{{1}}},
    {{{1, 1}}},
    {{{1}, {1}}},
    {{{1, 1, 1}}},
    {{{1}, {1}, {1}}},
    {{{1, 0}, {1, 0}, {1, 1}}},
    {{{0, 1}, {0, 1}, {1, 1}}},
    {{{1, 1, 1}, {0, 1, 0}}},
    {{{1, 1}, {1, 1}}},
    {{{1, 1, 1}, {1, 1, 1}, {1, 1, 1}}},
    {{{1, 1, 0}, {0, 1, 1}}},
    {{{0, 1, 1}, {1, 1, 0}}},
    {{{0, 1, 0}, {1, 1, 1}, {0, 1, 0}}},
    {{{1, 1, 1, 1}}},
    {{{1}, {1}, {1}, {1}}},
    {{{1, 0, 0}, {1, 0, 0}, {1, 1, 1}}},
    {{{1, 1, 1, 1, 1}}},
};

// Parçacık yapısı
struct Particle {
    float x, y;
    float vx, vy;
    Color color;
    float life;
    float size;
    float rotation;
    float rotSpeed;
};

// Glow parçacığı
struct GlowParticle {
    float x, y;
    float size;
    Color color;
    float life;
    float maxLife;
};

// Blok yapısı
struct Block {
    Shape shape;
    Color color;
    float x, y;
    float originalX, originalY;
    bool dragging;
    bool active;
    float scale;
    float targetScale;
    
    int getWidth() const { return shape.cells.empty() ? 0 : shape.cells[0].size(); }
    int getHeight() const { return shape.cells.size(); }
};

// Oyun durumları
enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_SETTINGS,
    STATE_GAME_OVER
};

// Settings structure
struct Settings {
    float musicVolume = 0.7f;
    float sfxVolume = 0.8f;
    bool particlesEnabled = true;
    bool screenShakeEnabled = true;
};

// Sound types
enum SoundType {
    SND_PLACE,      // Block placed
    SND_CLEAR,      // Line cleared
    SND_COMBO,      // Combo sound
    SND_PICKUP,     // Block picked up
    SND_DROP,       // Block dropped (invalid)
    SND_GAMEOVER,   // Game over
    SND_CLICK,      // Button click
    SND_COUNT
};

// Game class
class Game {
private:
    int grid[GRID_SIZE][GRID_SIZE];
    Color gridColors[GRID_SIZE][GRID_SIZE];
    Block availableBlocks[3];
    std::vector<Particle> particles;
    std::vector<GlowParticle> glowParticles;
    int score;
    int highScore;
    int combo;
    GameState gameState;
    Block* draggingBlock;
    float shakeTime;
    float shakeIntensity;
    Settings settings;
    
    // Sound system
    Sound sounds[SND_COUNT];
    Music bgMusic;
    bool soundsLoaded;
    bool musicLoaded;
    
    // UI animations
    float titlePulse;
    float menuAnimTime;
    float scorePopup;
    int lastScoreGain;
    float comboScale;
    
    // Buton durumları
    bool isDraggingTitleBar;
    Vector2 dragOffset;
    int hoveredButton;
    
    // Slider durumları
    int draggingSlider; // 0: none, 1: music, 2: sfx
    
public:
    Game() {
        srand(time(NULL));
        loadHighScore();
        loadSettings();
        initSounds();
        gameState = STATE_MENU;
        titlePulse = 0;
        menuAnimTime = 0;
        scorePopup = 0;
        lastScoreGain = 0;
        comboScale = 1.0f;
        isDraggingTitleBar = false;
        hoveredButton = -1;
        draggingSlider = 0;
        resetGame();
    }
    
    ~Game() {
        // Unload sounds
        for (int i = 0; i < SND_COUNT; i++) {
            if (soundsLoaded) UnloadSound(sounds[i]);
        }
        if (musicLoaded) UnloadMusicStream(bgMusic);
        CloseAudioDevice();
    }
    
    void initSounds() {
        InitAudioDevice();
        soundsLoaded = false;
        musicLoaded = false;
        
        // Load sound effects
        if (FileExists("sounds/place.ogg")) {
            sounds[SND_PLACE] = LoadSound("sounds/place.ogg");
            sounds[SND_PICKUP] = LoadSound("sounds/place.ogg"); // Use same sound for pickup
            sounds[SND_DROP] = LoadSound("sounds/place.ogg"); // Use same sound for drop
            sounds[SND_CLICK] = LoadSound("sounds/place.ogg"); // Use same sound for clicks
            soundsLoaded = true;
        }
        if (FileExists("sounds/clear.ogg")) {
            sounds[SND_CLEAR] = LoadSound("sounds/clear.ogg");
            sounds[SND_COMBO] = LoadSound("sounds/clear.ogg"); // Use same sound for combo
        }
        if (FileExists("sounds/gameover.ogg")) {
            sounds[SND_GAMEOVER] = LoadSound("sounds/gameover.ogg");
        }
        
        // Try to load background music
        if (FileExists("sounds/music.mp3")) {
            bgMusic = LoadMusicStream("sounds/music.mp3");
            musicLoaded = true;
            SetMusicVolume(bgMusic, settings.musicVolume);
            PlayMusicStream(bgMusic);
        } else if (FileExists("sounds/music.ogg")) {
            bgMusic = LoadMusicStream("sounds/music.ogg");
            musicLoaded = true;
            SetMusicVolume(bgMusic, settings.musicVolume);
            PlayMusicStream(bgMusic);
        }
    }
    
    void playSound(SoundType snd) {
        if (soundsLoaded && settings.sfxVolume > 0) {
            SetSoundVolume(sounds[snd], settings.sfxVolume);
            PlaySound(sounds[snd]);
        }
    }
    
    void updateMusic() {
        if (musicLoaded) {
            UpdateMusicStream(bgMusic);
            SetMusicVolume(bgMusic, settings.musicVolume);
        }
    }
    
    void loadHighScore() {
        std::ifstream file("highscore.dat");
        if (file.is_open()) {
            file >> highScore;
            file.close();
        } else {
            highScore = 0;
        }
    }
    
    void saveHighScore() {
        std::ofstream file("highscore.dat");
        if (file.is_open()) {
            file << highScore;
            file.close();
        }
    }
    
    void loadSettings() {
        std::ifstream file("settings.dat");
        if (file.is_open()) {
            file >> settings.musicVolume >> settings.sfxVolume;
            file >> settings.particlesEnabled >> settings.screenShakeEnabled;
            file.close();
        }
    }
    
    void saveSettings() {
        std::ofstream file("settings.dat");
        if (file.is_open()) {
            file << settings.musicVolume << " " << settings.sfxVolume << " ";
            file << settings.particlesEnabled << " " << settings.screenShakeEnabled;
            file.close();
        }
    }
    
    void resetGame() {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                grid[y][x] = 0;
            }
        }
        score = 0;
        combo = 0;
        draggingBlock = nullptr;
        shakeTime = 0;
        shakeIntensity = 0;
        particles.clear();
        glowParticles.clear();
        generateNewBlocks();
    }
    
    void generateNewBlocks() {
        // Calculate centered positions for 3 blocks below the grid
        int gridTotalWidth = GRID_SIZE * CELL_SIZE;
        int blockAreaY = GRID_OFFSET_Y + GRID_SIZE * CELL_SIZE + 40;
        int blockSpacing = gridTotalWidth / 3;
        
        float positions[3][2] = {
            {(float)(GRID_OFFSET_X + blockSpacing * 0 + blockSpacing / 2 - 40), (float)blockAreaY},
            {(float)(GRID_OFFSET_X + blockSpacing * 1 + blockSpacing / 2 - 40), (float)blockAreaY},
            {(float)(GRID_OFFSET_X + blockSpacing * 2 + blockSpacing / 2 - 40), (float)blockAreaY}
        };
        
        for (int i = 0; i < 3; i++) {
            // Smart block selection - ensure at least one block can be placed
            int shapeIdx;
            int attempts = 0;
            bool canPlace = false;
            
            do {
                shapeIdx = rand() % SHAPES.size();
                // Check if this shape can be placed anywhere
                Block testBlock;
                testBlock.shape = SHAPES[shapeIdx];
                for (int gy = 0; gy < GRID_SIZE && !canPlace; gy++) {
                    for (int gx = 0; gx < GRID_SIZE && !canPlace; gx++) {
                        if (canPlaceBlock(testBlock, gx, gy)) canPlace = true;
                    }
                }
                attempts++;
            } while (!canPlace && attempts < 20);
            
            // If no placeable block found after 20 attempts, use a small block
            if (!canPlace) {
                shapeIdx = 0; // Single cell block - always fits somewhere or game is over
            }
            
            int colorIdx = rand() % NUM_COLORS;
            
            availableBlocks[i].shape = SHAPES[shapeIdx];
            availableBlocks[i].color = BLOCK_COLORS[colorIdx];
            availableBlocks[i].x = positions[i][0];
            availableBlocks[i].y = positions[i][1];
            availableBlocks[i].originalX = positions[i][0];
            availableBlocks[i].originalY = positions[i][1];
            availableBlocks[i].dragging = false;
            availableBlocks[i].active = true;
            availableBlocks[i].scale = 0.0f;
            availableBlocks[i].targetScale = 0.5f;
        }
    }
    
    bool canPlaceBlock(const Block& block, int gridX, int gridY) {
        for (int row = 0; row < block.getHeight(); row++) {
            for (int col = 0; col < block.getWidth(); col++) {
                if (block.shape.cells[row][col]) {
                    int gx = gridX + col;
                    int gy = gridY + row;
                    if (gx < 0 || gx >= GRID_SIZE || gy < 0 || gy >= GRID_SIZE) {
                        return false;
                    }
                    if (grid[gy][gx] != 0) {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    
    void placeBlock(Block& block, int gridX, int gridY) {
        int cellCount = 0;
        for (int row = 0; row < block.getHeight(); row++) {
            for (int col = 0; col < block.getWidth(); col++) {
                if (block.shape.cells[row][col]) {
                    grid[gridY + row][gridX + col] = 1;
                    gridColors[gridY + row][gridX + col] = block.color;
                    cellCount++;
                    
                    // Yerleştirme parçacıkları
                    if (settings.particlesEnabled) {
                        createPlaceParticles(gridX + col, gridY + row, block.color);
                    }
                }
            }
        }
        int gained = cellCount * 10;
        score += gained;
        lastScoreGain = gained;
        scorePopup = 1.0f;
        block.active = false;
    }
    
    void createPlaceParticles(int gridX, int gridY, Color color) {
        float px = GRID_OFFSET_X + gridX * CELL_SIZE + CELL_SIZE / 2;
        float py = GRID_OFFSET_Y + gridY * CELL_SIZE + CELL_SIZE / 2;
        
        for (int i = 0; i < 4; i++) {
            Particle p;
            p.x = px;
            p.y = py;
            p.vx = (rand() % 60 - 30) / 10.0f;
            p.vy = (rand() % 60 - 30) / 10.0f;
            p.color = color;
            p.life = 0.5f;
            p.size = 3 + rand() % 3;
            p.rotation = rand() % 360;
            p.rotSpeed = (rand() % 100 - 50) / 5.0f;
            particles.push_back(p);
        }
    }
    
    void createParticles(int gridX, int gridY, Color color) {
        float px = GRID_OFFSET_X + gridX * CELL_SIZE + CELL_SIZE / 2;
        float py = GRID_OFFSET_Y + gridY * CELL_SIZE + CELL_SIZE / 2;
        
        // Patlama parçacıkları
        for (int i = 0; i < 12; i++) {
            Particle p;
            p.x = px;
            p.y = py;
            float angle = (i / 12.0f) * 2 * PI + (rand() % 100) / 100.0f;
            float speed = 3 + rand() % 5;
            p.vx = cos(angle) * speed;
            p.vy = sin(angle) * speed;
            p.color = color;
            p.life = 1.0f;
            p.size = 4 + rand() % 4;
            p.rotation = rand() % 360;
            p.rotSpeed = (rand() % 100 - 50) / 3.0f;
            particles.push_back(p);
        }
        
        // Glow efekti
        GlowParticle g;
        g.x = px;
        g.y = py;
        g.size = CELL_SIZE * 2;
        g.color = color;
        g.life = 0.5f;
        g.maxLife = 0.5f;
        glowParticles.push_back(g);
    }
    
    void checkAndClearLines() {
        std::vector<int> rowsToClear;
        std::vector<int> colsToClear;
        
        for (int y = 0; y < GRID_SIZE; y++) {
            bool full = true;
            for (int x = 0; x < GRID_SIZE; x++) {
                if (grid[y][x] == 0) { full = false; break; }
            }
            if (full) rowsToClear.push_back(y);
        }
        
        for (int x = 0; x < GRID_SIZE; x++) {
            bool full = true;
            for (int y = 0; y < GRID_SIZE; y++) {
                if (grid[y][x] == 0) { full = false; break; }
            }
            if (full) colsToClear.push_back(x);
        }
        
        int linesCleared = rowsToClear.size() + colsToClear.size();
        
        if (linesCleared > 0) {
            // Play sound based on combo
            if (combo >= 1) {
                playSound(SND_COMBO);
            } else {
                playSound(SND_CLEAR);
            }
            
            if (settings.screenShakeEnabled) {
                shakeTime = 0.4f;
                shakeIntensity = 8.0f * linesCleared;
            }
            
            for (int y : rowsToClear) {
                for (int x = 0; x < GRID_SIZE; x++) {
                    if (settings.particlesEnabled) createParticles(x, y, gridColors[y][x]);
                    grid[y][x] = 0;
                }
            }
            
            for (int x : colsToClear) {
                for (int y = 0; y < GRID_SIZE; y++) {
                    if (grid[y][x] != 0) {
                        if (settings.particlesEnabled) createParticles(x, y, gridColors[y][x]);
                        grid[y][x] = 0;
                    }
                }
            }
            
            combo++;
            comboScale = 1.5f;
            int bonus = linesCleared * 100 * combo;
            score += bonus;
            lastScoreGain = bonus;
            scorePopup = 1.0f;
            
            if (score > highScore) {
                highScore = score;
                saveHighScore();
            }
        } else {
            combo = 0;
        }
    }
    
    bool canPlaceAnyBlock() {
        for (int i = 0; i < 3; i++) {
            if (!availableBlocks[i].active) continue;
            for (int y = 0; y < GRID_SIZE; y++) {
                for (int x = 0; x < GRID_SIZE; x++) {
                    if (canPlaceBlock(availableBlocks[i], x, y)) return true;
                }
            }
        }
        return false;
    }
    
    void updateParticles(float dt) {
        for (auto it = particles.begin(); it != particles.end();) {
            it->x += it->vx;
            it->y += it->vy;
            it->vy += 150 * dt;
            it->life -= dt * 1.5f;
            it->rotation += it->rotSpeed * dt;
            it->size *= 0.97f;
            
            if (it->life <= 0) it = particles.erase(it);
            else ++it;
        }
        
        for (auto it = glowParticles.begin(); it != glowParticles.end();) {
            it->life -= dt;
            it->size *= 1.05f;
            if (it->life <= 0) it = glowParticles.erase(it);
            else ++it;
        }
    }
    
    void update() {
        float dt = GetFrameTime();
        titlePulse += dt * 2;
        menuAnimTime += dt;
        
        // Update background music
        updateMusic();
        
        if (scorePopup > 0) scorePopup -= dt * 2;
        if (comboScale > 1.0f) comboScale -= dt * 2;
        if (comboScale < 1.0f) comboScale = 1.0f;
        
        // Blok animasyonları
        for (int i = 0; i < 3; i++) {
            if (availableBlocks[i].active) {
                float diff = availableBlocks[i].targetScale - availableBlocks[i].scale;
                availableBlocks[i].scale += diff * dt * 10;
            }
        }
        
        if (shakeTime > 0) shakeTime -= dt;
        updateParticles(dt);
        
        Vector2 mouse = GetMousePosition();
        
        // Custom title bar sürükleme
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (mouse.y < TITLE_BAR_HEIGHT && mouse.x < SCREEN_WIDTH - 100) {
                isDraggingTitleBar = true;
                dragOffset = mouse;
            }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            isDraggingTitleBar = false;
            draggingSlider = 0;
        }
        if (isDraggingTitleBar) {
            Vector2 windowPos = GetWindowPosition();
            SetWindowPosition(windowPos.x + mouse.x - dragOffset.x, 
                            windowPos.y + mouse.y - dragOffset.y);
        }
        
        // Kapat butonu
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Rectangle closeBtn = { SCREEN_WIDTH - 45, 5, 35, 30 };
            if (CheckCollisionPointRec(mouse, closeBtn)) {
                CloseWindow();
                exit(0);
            }
            Rectangle minBtn = { SCREEN_WIDTH - 85, 5, 35, 30 };
            if (CheckCollisionPointRec(mouse, minBtn)) {
                MinimizeWindow();
            }
        }
        
        switch (gameState) {
            case STATE_MENU: updateMenu(); break;
            case STATE_PLAYING: updatePlaying(); break;
            case STATE_SETTINGS: updateSettings(); break;
            case STATE_GAME_OVER: updateGameOver(); break;
        }
    }
    
    void updateMenu() {
        Vector2 mouse = GetMousePosition();
        hoveredButton = -1;
        
        Rectangle playBtn = { SCREEN_WIDTH/2 - 150, 400, 300, 60 };
        Rectangle settingsBtn = { SCREEN_WIDTH/2 - 150, 480, 300, 60 };
        Rectangle quitBtn = { SCREEN_WIDTH/2 - 150, 560, 300, 60 };
        
        if (CheckCollisionPointRec(mouse, playBtn)) hoveredButton = 0;
        else if (CheckCollisionPointRec(mouse, settingsBtn)) hoveredButton = 1;
        else if (CheckCollisionPointRec(mouse, quitBtn)) hoveredButton = 2;
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredButton >= 0) {
            if (hoveredButton == 0) { resetGame(); gameState = STATE_PLAYING; }
            else if (hoveredButton == 1) gameState = STATE_SETTINGS;
            else if (hoveredButton == 2) { CloseWindow(); exit(0); }
        }
    }
    
    void updateSettings() {
        Vector2 mouse = GetMousePosition();
        hoveredButton = -1;
        
        // Sliders and buttons - must match drawSettings positions
        Rectangle musicSlider = { SCREEN_WIDTH/2 - 100, 230, 200, 20 };
        Rectangle sfxSlider = { SCREEN_WIDTH/2 - 100, 300, 200, 20 };
        Rectangle particlesBtn = { SCREEN_WIDTH/2 - 150, 350, 300, 45 };
        Rectangle shakeBtn = { SCREEN_WIDTH/2 - 150, 405, 300, 45 };
        Rectangle fullscreenBtn = { SCREEN_WIDTH/2 - 150, 460, 300, 45 };
        Rectangle backBtn = { SCREEN_WIDTH/2 - 150, 530, 300, 55 };
        
        if (CheckCollisionPointRec(mouse, backBtn)) hoveredButton = 4;
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mouse, musicSlider)) draggingSlider = 1;
            else if (CheckCollisionPointRec(mouse, sfxSlider)) draggingSlider = 2;
            else if (CheckCollisionPointRec(mouse, particlesBtn)) { settings.particlesEnabled = !settings.particlesEnabled; }
            else if (CheckCollisionPointRec(mouse, shakeBtn)) { settings.screenShakeEnabled = !settings.screenShakeEnabled; }
            else if (CheckCollisionPointRec(mouse, fullscreenBtn)) { toggleFullscreen(); }
            else if (hoveredButton == 4) { saveSettings(); gameState = STATE_MENU; }
        }
        
        if (draggingSlider == 1) {
            float val = (mouse.x - musicSlider.x) / musicSlider.width;
            settings.musicVolume = fmax(0.0f, fmin(1.0f, val));
        } else if (draggingSlider == 2) {
            float val = (mouse.x - sfxSlider.x) / sfxSlider.width;
            settings.sfxVolume = fmax(0.0f, fmin(1.0f, val));
        }
    }
    
    void toggleFullscreen() {
        isFullscreen = !isFullscreen;
        if (isFullscreen) {
            // Store current window size
            windowedWidth = SCREEN_WIDTH;
            windowedHeight = SCREEN_HEIGHT;
            
            // Get monitor dimensions
            int monitor = GetCurrentMonitor();
            int monW = GetMonitorWidth(monitor);
            int monH = GetMonitorHeight(monitor);
            
            // Set to fullscreen size
            SCREEN_WIDTH = monW;
            SCREEN_HEIGHT = monH;
            SetWindowPosition(0, 0);
            SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        } else {
            // Restore windowed mode
            SCREEN_WIDTH = 600;
            SCREEN_HEIGHT = 800;
            SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
            
            // Center window
            int monitor = GetCurrentMonitor();
            int monW = GetMonitorWidth(monitor);
            int monH = GetMonitorHeight(monitor);
            SetWindowPosition((monW - SCREEN_WIDTH) / 2, (monH - SCREEN_HEIGHT) / 2);
        }
        
        // Recalculate grid offset to center it
        GRID_OFFSET_X = (SCREEN_WIDTH - GRID_SIZE * CELL_SIZE) / 2;
        GRID_OFFSET_Y = (SCREEN_HEIGHT - GRID_SIZE * CELL_SIZE) / 2 - 50;
        
        // Update block positions
        updateBlockPositions();
    }
    
    void updateBlockPositions() {
        int gridTotalWidth = GRID_SIZE * CELL_SIZE;
        int blockAreaY = GRID_OFFSET_Y + GRID_SIZE * CELL_SIZE + 40;
        int blockSpacing = gridTotalWidth / 3;
        
        float positions[3][2] = {
            {(float)(GRID_OFFSET_X + blockSpacing * 0 + blockSpacing / 2 - 40), (float)blockAreaY},
            {(float)(GRID_OFFSET_X + blockSpacing * 1 + blockSpacing / 2 - 40), (float)blockAreaY},
            {(float)(GRID_OFFSET_X + blockSpacing * 2 + blockSpacing / 2 - 40), (float)blockAreaY}
        };
        
        for (int i = 0; i < 3; i++) {
            availableBlocks[i].x = positions[i][0];
            availableBlocks[i].y = positions[i][1];
            availableBlocks[i].originalX = positions[i][0];
            availableBlocks[i].originalY = positions[i][1];
        }
    }
    
    void updatePlaying() {
        Vector2 mouse = GetMousePosition();
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (int i = 0; i < 3; i++) {
                if (!availableBlocks[i].active) continue;
                
                Block& block = availableBlocks[i];
                float bw = block.getWidth() * CELL_SIZE * block.scale;
                float bh = block.getHeight() * CELL_SIZE * block.scale;
                Rectangle rect = { block.x, block.y, bw, bh };
                
                if (CheckCollisionPointRec(mouse, rect)) {
                    block.dragging = true;
                    block.targetScale = 1.0f;
                    draggingBlock = &block;
                    break;
                }
            }
        }
        
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && draggingBlock) {
            int gridX = round((draggingBlock->x - GRID_OFFSET_X) / CELL_SIZE);
            int gridY = round((draggingBlock->y - GRID_OFFSET_Y) / CELL_SIZE);
            
            if (canPlaceBlock(*draggingBlock, gridX, gridY)) {
                placeBlock(*draggingBlock, gridX, gridY);
                playSound(SND_PLACE);
                checkAndClearLines();
                
                bool allUsed = true;
                for (int i = 0; i < 3; i++) {
                    if (availableBlocks[i].active) { allUsed = false; break; }
                }
                if (allUsed) generateNewBlocks();
                
                if (!canPlaceAnyBlock()) {
                    playSound(SND_GAMEOVER);
                    gameState = STATE_GAME_OVER;
                }
            } else {
                draggingBlock->x = draggingBlock->originalX;
                draggingBlock->y = draggingBlock->originalY;
                draggingBlock->targetScale = 0.5f;
            }
            
            draggingBlock->dragging = false;
            draggingBlock = nullptr;
        }
        
        if (draggingBlock && draggingBlock->dragging) {
            draggingBlock->x = mouse.x - draggingBlock->getWidth() * CELL_SIZE / 2;
            draggingBlock->y = mouse.y - draggingBlock->getHeight() * CELL_SIZE / 2;
        }
    }
    
    void updateGameOver() {
        Vector2 mouse = GetMousePosition();
        hoveredButton = -1;
        
        Rectangle restartBtn = { SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 + 50, 260, 60 };
        Rectangle menuBtn = { SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 + 130, 260, 60 };
        
        if (CheckCollisionPointRec(mouse, restartBtn)) hoveredButton = 0;
        else if (CheckCollisionPointRec(mouse, menuBtn)) hoveredButton = 1;
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (hoveredButton == 0) { resetGame(); gameState = STATE_PLAYING; }
            else if (hoveredButton == 1) { gameState = STATE_MENU; }
        }
    }
    
    void drawRoundedRect(Rectangle rec, float roundness, Color color) {
        DrawRectangleRounded(rec, roundness, 8, color);
    }
    
    void drawModernButton(Rectangle rec, const char* text, bool hovered, Color baseColor) {
        Color c = hovered ? BUTTON_HOVER : baseColor;
        
        // Gölge
        DrawRectangleRounded({ rec.x + 3, rec.y + 3, rec.width, rec.height }, 0.3f, 8, { 0, 0, 0, 100 });
        
        // Buton
        DrawRectangleRounded(rec, 0.3f, 8, c);
        
        // Üst parlama
        DrawRectangleRounded({ rec.x, rec.y, rec.width, rec.height * 0.4f }, 0.3f, 8, 
                            { 255, 255, 255, hovered ? (unsigned char)30 : (unsigned char)15 });
        
        // Kenar
        DrawRectangleRoundedLines(rec, 0.3f, 8, { 255, 255, 255, 50 });
        
        // Metin
        int textWidth = MeasureText(text, 24);
        DrawText(text, rec.x + (rec.width - textWidth) / 2, rec.y + (rec.height - 24) / 2, 24, TEXT_COLOR);
    }
    
    void drawTitleBar() {
        // Title bar arka planı
        DrawRectangle(0, 0, SCREEN_WIDTH, TITLE_BAR_HEIGHT, TITLE_BAR_COLOR);
        
        // Logo/Icon
        float iconPulse = sin(titlePulse) * 0.1f + 1.0f;
        DrawRectangleRounded({ 10, 8, 24, 24 }, 0.3f, 6, 
                            { (unsigned char)(100 * iconPulse), (unsigned char)(100 * iconPulse), (unsigned char)(255 * iconPulse), 255 });
        DrawRectangleRounded({ 14, 12, 8, 8 }, 0.2f, 4, GOLD_COLOR);
        DrawRectangleRounded({ 22, 20, 8, 8 }, 0.2f, 4, { 255, 100, 100, 255 });
        
        // Başlık
        DrawText("BLOCK BLAST", 45, 12, 20, TEXT_COLOR);
        
        // Minimize butonu
        Vector2 mouse = GetMousePosition();
        Rectangle minBtn = { SCREEN_WIDTH - 85, 5, 35, 30 };
        Color minColor = CheckCollisionPointRec(mouse, minBtn) ? Color{80, 80, 100, 255} : Color{50, 50, 70, 255};
        DrawRectangleRounded(minBtn, 0.3f, 6, minColor);
        DrawRectangle(SCREEN_WIDTH - 75, 18, 15, 3, TEXT_COLOR);
        
        // Kapat butonu
        Rectangle closeBtn = { SCREEN_WIDTH - 45, 5, 35, 30 };
        Color closeColor = CheckCollisionPointRec(mouse, closeBtn) ? Color{200, 60, 60, 255} : Color{50, 50, 70, 255};
        DrawRectangleRounded(closeBtn, 0.3f, 6, closeColor);
        DrawText("X", SCREEN_WIDTH - 33, 10, 20, TEXT_COLOR);
    }
    
    void drawGrid() {
        float shakeX = 0, shakeY = 0;
        if (shakeTime > 0) {
            shakeX = (rand() % 100 - 50) / 50.0f * shakeIntensity;
            shakeY = (rand() % 100 - 50) / 50.0f * shakeIntensity;
        }
        
        // Izgara gölgesi
        DrawRectangleRounded(
            { GRID_OFFSET_X - 5 + shakeX + 5, GRID_OFFSET_Y - 5 + shakeY + 5, 
              GRID_SIZE * CELL_SIZE + 10, GRID_SIZE * CELL_SIZE + 10 },
            0.02f, 10, { 0, 0, 0, 80 }
        );
        
        // Izgara arka planı
        DrawRectangleRounded(
            { GRID_OFFSET_X - 8 + shakeX, GRID_OFFSET_Y - 8 + shakeY, 
              GRID_SIZE * CELL_SIZE + 16, GRID_SIZE * CELL_SIZE + 16 },
            0.02f, 10, GRID_BG
        );
        
        // Hücreler
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                float px = GRID_OFFSET_X + x * CELL_SIZE + shakeX;
                float py = GRID_OFFSET_Y + y * CELL_SIZE + shakeY;
                
                if (grid[y][x] != 0) {
                    Color c = gridColors[y][x];
                    
                    // Glow efekti
                    DrawRectangleRounded({ px - 2, py - 2, CELL_SIZE + 2, CELL_SIZE + 2 }, 0.2f, 6, 
                                        { c.r, c.g, c.b, 50 });
                    
                    // Ana blok
                    DrawRectangleRounded({ px + 2, py + 2, CELL_SIZE - 4, CELL_SIZE - 4 }, 0.25f, 6, c);
                    
                    // Üst parlama
                    Color highlight = { 
                        (unsigned char)fmin(255, c.r + 80),
                        (unsigned char)fmin(255, c.g + 80),
                        (unsigned char)fmin(255, c.b + 80),
                        180 
                    };
                    DrawRectangleRounded({ px + 5, py + 5, CELL_SIZE - 10, 12 }, 0.5f, 4, highlight);
                    
                    // Alt gölge
                    DrawRectangleRounded({ px + 5, py + CELL_SIZE - 15, CELL_SIZE - 10, 8 }, 0.5f, 4, 
                                        { 0, 0, 0, 40 });
                } else {
                    DrawRectangleRounded({ px + 2, py + 2, CELL_SIZE - 4, CELL_SIZE - 4 }, 0.2f, 4, GRID_LINE);
                }
            }
        }
    }
    
    void drawBlock(const Block& block) {
        float scale = block.scale;
        
        // Draw pickup outline around the entire block bounding box
        if (!block.dragging) {
            float bw = block.getWidth() * CELL_SIZE * scale;
            float bh = block.getHeight() * CELL_SIZE * scale;
            
            // Pulsing effect for the outline
            float pulse = (sin(GetTime() * 4) + 1) * 0.5f;
            unsigned char outlineAlpha = (unsigned char)(80 + pulse * 60);
            
            // Outer glow
            Color glowColor = { block.color.r, block.color.g, block.color.b, (unsigned char)(outlineAlpha * 0.5f) };
            DrawRectangleRounded({ block.x - 4, block.y - 4, bw + 8, bh + 8 }, 0.15f, 6, glowColor);
            
            // Outline border
            Color outlineColor = { 
                (unsigned char)fmin(255, block.color.r + 50),
                (unsigned char)fmin(255, block.color.g + 50),
                (unsigned char)fmin(255, block.color.b + 50),
                outlineAlpha
            };
            DrawRectangleRoundedLines({ block.x - 2, block.y - 2, bw + 4, bh + 4 }, 0.15f, 6, outlineColor);
            
            // Corner indicators to make pickup area more visible
            float cornerSize = 8;
            Color cornerColor = { 255, 255, 255, outlineAlpha };
            // Top-left
            DrawLineEx({ block.x - 2, block.y - 2 }, { block.x - 2 + cornerSize, block.y - 2 }, 2, cornerColor);
            DrawLineEx({ block.x - 2, block.y - 2 }, { block.x - 2, block.y - 2 + cornerSize }, 2, cornerColor);
            // Top-right
            DrawLineEx({ block.x + bw + 2 - cornerSize, block.y - 2 }, { block.x + bw + 2, block.y - 2 }, 2, cornerColor);
            DrawLineEx({ block.x + bw + 2, block.y - 2 }, { block.x + bw + 2, block.y - 2 + cornerSize }, 2, cornerColor);
            // Bottom-left
            DrawLineEx({ block.x - 2, block.y + bh + 2 - cornerSize }, { block.x - 2, block.y + bh + 2 }, 2, cornerColor);
            DrawLineEx({ block.x - 2, block.y + bh + 2 }, { block.x - 2 + cornerSize, block.y + bh + 2 }, 2, cornerColor);
            // Bottom-right
            DrawLineEx({ block.x + bw + 2 - cornerSize, block.y + bh + 2 }, { block.x + bw + 2, block.y + bh + 2 }, 2, cornerColor);
            DrawLineEx({ block.x + bw + 2, block.y + bh + 2 - cornerSize }, { block.x + bw + 2, block.y + bh + 2 }, 2, cornerColor);
        }
        
        for (int row = 0; row < block.getHeight(); row++) {
            for (int col = 0; col < block.getWidth(); col++) {
                if (block.shape.cells[row][col]) {
                    float size = CELL_SIZE * scale - 4;
                    float px = block.x + col * CELL_SIZE * scale;
                    float py = block.y + row * CELL_SIZE * scale;
                    
                    // Glow when dragging
                    if (block.dragging) {
                        DrawRectangleRounded({ px - 3, py - 3, size + 6, size + 6 }, 0.2f, 6, 
                                            { block.color.r, block.color.g, block.color.b, 80 });
                    }
                    
                    DrawRectangleRounded({ px, py, size, size }, 0.25f, 6, block.color);
                    
                    // Parlama
                    Color highlight = { 
                        (unsigned char)fmin(255, block.color.r + 70),
                        (unsigned char)fmin(255, block.color.g + 70),
                        (unsigned char)fmin(255, block.color.b + 70),
                        180 
                    };
                    DrawRectangleRounded({ px + 3, py + 3, size - 6, size * 0.3f }, 0.5f, 4, highlight);
                }
            }
        }
    }
    
    void drawGhost(const Block& block) {
        int gridX = round((block.x - GRID_OFFSET_X) / CELL_SIZE);
        int gridY = round((block.y - GRID_OFFSET_Y) / CELL_SIZE);
        
        // Only show ghost if block is within or near the grid area
        int blockEndX = gridX + block.getWidth() - 1;
        int blockEndY = gridY + block.getHeight() - 1;
        bool nearGrid = (gridX >= -1 && gridX <= GRID_SIZE && 
                         gridY >= -1 && gridY <= GRID_SIZE &&
                         blockEndX >= 0 && blockEndY >= 0);
        
        if (!nearGrid) return;
        
        bool valid = canPlaceBlock(block, gridX, gridY);
        
        float pulse = (sin(GetTime() * 8) + 1) * 0.5f;
        unsigned char alpha = (unsigned char)(60 + pulse * 40);
        Color ghostColor = valid ? Color{ 100, 255, 100, alpha } : Color{ 255, 100, 100, alpha };
        
        for (int row = 0; row < block.getHeight(); row++) {
            for (int col = 0; col < block.getWidth(); col++) {
                if (block.shape.cells[row][col]) {
                    int cellGridX = gridX + col;
                    int cellGridY = gridY + row;
                    // Only draw ghost cells that are within the grid
                    if (cellGridX >= 0 && cellGridX < GRID_SIZE && cellGridY >= 0 && cellGridY < GRID_SIZE) {
                        float px = GRID_OFFSET_X + cellGridX * CELL_SIZE;
                        float py = GRID_OFFSET_Y + cellGridY * CELL_SIZE;
                        DrawRectangleRounded({ px + 3, py + 3, CELL_SIZE - 6, CELL_SIZE - 6 }, 0.25f, 6, ghostColor);
                    }
                }
            }
        }
    }
    
    void drawParticles() {
        // Glow parçacıkları
        for (const auto& g : glowParticles) {
            float alpha = (g.life / g.maxLife) * 150;
            Color c = { g.color.r, g.color.g, g.color.b, (unsigned char)alpha };
            DrawCircleGradient(g.x, g.y, g.size, c, { g.color.r, g.color.g, g.color.b, 0 });
        }
        
        // Normal parçacıklar
        for (const auto& p : particles) {
            Color c = p.color;
            c.a = (unsigned char)(255 * p.life);
            
            Vector2 center = { p.x, p.y };
            DrawPoly(center, 4, p.size, p.rotation, c);
        }
    }
    
    void drawUI() {
        // Score box - positioned at top left, above grid
        DrawRectangleRounded({ 30, TITLE_BAR_HEIGHT + 20, 170, 70 }, 0.2f, 8, PANEL_COLOR);
        DrawRectangleRoundedLines({ 30, TITLE_BAR_HEIGHT + 20, 170, 70 }, 0.2f, 8, { 100, 100, 150, 100 });
        DrawText("SCORE", 50, TITLE_BAR_HEIGHT + 30, 18, GRAY);
        
        // Skor animasyonu
        float scoreScale = 1.0f + scorePopup * 0.3f;
        int fontSize = (int)(32 * scoreScale);
        DrawText(TextFormat("%d", score), 50, TITLE_BAR_HEIGHT + 52, fontSize, TEXT_COLOR);
        
        // Puan popup
        if (scorePopup > 0 && lastScoreGain > 0) {
            unsigned char alpha = (unsigned char)(scorePopup * 255);
            DrawText(TextFormat("+%d", lastScoreGain), 130, TITLE_BAR_HEIGHT + 25 - (1 - scorePopup) * 20, 20, 
                    { 100, 255, 100, alpha });
        }
        
        // High score box - positioned at top right, above grid
        DrawRectangleRounded({ SCREEN_WIDTH - 200, TITLE_BAR_HEIGHT + 20, 170, 70 }, 0.2f, 8, PANEL_COLOR);
        DrawRectangleRoundedLines({ SCREEN_WIDTH - 200, TITLE_BAR_HEIGHT + 20, 170, 70 }, 0.2f, 8, { 100, 100, 150, 100 });
        DrawText("HIGH SCORE", SCREEN_WIDTH - 180, TITLE_BAR_HEIGHT + 30, 18, GRAY);
        DrawText(TextFormat("%d", highScore), SCREEN_WIDTH - 180, TITLE_BAR_HEIGHT + 55, 28, GOLD_COLOR);
        
        // Combo göstergesi
        if (combo > 1) {
            const char* comboText = TextFormat("COMBO x%d!", combo);
            int comboWidth = MeasureText(comboText, 36);
            float scale = comboScale;
            int fontSize = (int)(36 * scale);
            int scaledWidth = MeasureText(comboText, fontSize);
            
            // Glow efekti
            DrawText(comboText, (SCREEN_WIDTH - scaledWidth) / 2 + 2, TITLE_BAR_HEIGHT + 35 + 2, fontSize, 
                    { 255, 200, 0, 100 });
            DrawText(comboText, (SCREEN_WIDTH - scaledWidth) / 2, TITLE_BAR_HEIGHT + 35, fontSize, GOLD_COLOR);
        }
    }
    
    void drawMenu() {
        // Animasyonlu arka plan deseni
        for (int i = 0; i < 20; i++) {
            float x = fmod(i * 80 + menuAnimTime * 30, SCREEN_WIDTH + 100) - 50;
            float y = 200 + sin(menuAnimTime + i) * 50;
            float size = 20 + sin(menuAnimTime * 2 + i) * 10;
            DrawRectangleRounded({ x, y, size, size }, 0.3f, 6, { 60, 60, 100, 30 });
        }
        
        // Başlık
        const char* title = "BLOCK BLAST";
        int titleWidth = MeasureText(title, 70);
        float pulse = sin(titlePulse) * 5;
        
        // Başlık gölgesi
        DrawText(title, (SCREEN_WIDTH - titleWidth) / 2 + 4, 150 + pulse + 4, 70, { 0, 0, 0, 100 });
        
        // Gradient başlık efekti
        for (int i = 0; i < 3; i++) {
            Color c = { (unsigned char)(100 + i * 50), (unsigned char)(100 + i * 50), (unsigned char)(255 - i * 30), 255 };
            DrawText(title, (SCREEN_WIDTH - titleWidth) / 2, 150 + pulse - i, 70, c);
        }
        
        // Alt başlık
        const char* subtitle = "Puzzle Game";
        int subWidth = MeasureText(subtitle, 24);
        DrawText(subtitle, (SCREEN_WIDTH - subWidth) / 2, 230, 24, GRAY);
        
        // Buttons
        drawModernButton({ SCREEN_WIDTH/2 - 150, 400, 300, 60 }, "PLAY", hoveredButton == 0, BUTTON_COLOR);
        drawModernButton({ SCREEN_WIDTH/2 - 150, 480, 300, 60 }, "SETTINGS", hoveredButton == 1, BUTTON_COLOR);
        drawModernButton({ SCREEN_WIDTH/2 - 150, 560, 300, 60 }, "EXIT", hoveredButton == 2, { 150, 60, 60, 255 });
        
        // High score
        DrawText(TextFormat("High Score: %d", highScore), SCREEN_WIDTH/2 - 80, 700, 20, GOLD_COLOR);
    }
    
    void drawSettings() {
        // Panel
        DrawRectangleRounded({ SCREEN_WIDTH/2 - 250, 120, 500, 530 }, 0.05f, 10, PANEL_COLOR);
        DrawRectangleRoundedLines({ SCREEN_WIDTH/2 - 250, 120, 500, 530 }, 0.05f, 10, { 100, 100, 150, 100 });
        
        // Title
        const char* title = "SETTINGS";
        int titleWidth = MeasureText(title, 40);
        DrawText(title, (SCREEN_WIDTH - titleWidth) / 2, 140, 40, TEXT_COLOR);
        
        // Music slider
        DrawText("Music Volume", SCREEN_WIDTH/2 - 100, 200, 20, GRAY);
        Rectangle musicSlider = { SCREEN_WIDTH/2 - 100, 230, 200, 20 };
        DrawRectangleRounded(musicSlider, 0.5f, 8, GRID_LINE);
        DrawRectangleRounded({ musicSlider.x, musicSlider.y, musicSlider.width * settings.musicVolume, 20 }, 0.5f, 8, ACCENT_COLOR);
        DrawCircle(musicSlider.x + musicSlider.width * settings.musicVolume, musicSlider.y + 10, 12, TEXT_COLOR);
        DrawText(TextFormat("%d%%", (int)(settings.musicVolume * 100)), SCREEN_WIDTH/2 + 120, 227, 20, TEXT_COLOR);
        
        // SFX slider
        DrawText("SFX Volume", SCREEN_WIDTH/2 - 100, 270, 20, GRAY);
        Rectangle sfxSlider = { SCREEN_WIDTH/2 - 100, 300, 200, 20 };
        DrawRectangleRounded(sfxSlider, 0.5f, 8, GRID_LINE);
        DrawRectangleRounded({ sfxSlider.x, sfxSlider.y, sfxSlider.width * settings.sfxVolume, 20 }, 0.5f, 8, ACCENT_COLOR);
        DrawCircle(sfxSlider.x + sfxSlider.width * settings.sfxVolume, sfxSlider.y + 10, 12, TEXT_COLOR);
        DrawText(TextFormat("%d%%", (int)(settings.sfxVolume * 100)), SCREEN_WIDTH/2 + 120, 297, 20, TEXT_COLOR);
        
        // Toggle buttons
        Rectangle particlesBtn = { SCREEN_WIDTH/2 - 150, 350, 300, 45 };
        Color pColor = settings.particlesEnabled ? Color{60, 150, 60, 255} : BUTTON_COLOR;
        DrawRectangleRounded(particlesBtn, 0.3f, 8, pColor);
        DrawText(settings.particlesEnabled ? "Particles: ON" : "Particles: OFF", 
                particlesBtn.x + 70, particlesBtn.y + 12, 20, TEXT_COLOR);
        
        Rectangle shakeBtn = { SCREEN_WIDTH/2 - 150, 405, 300, 45 };
        Color sColor = settings.screenShakeEnabled ? Color{60, 150, 60, 255} : BUTTON_COLOR;
        DrawRectangleRounded(shakeBtn, 0.3f, 8, sColor);
        DrawText(settings.screenShakeEnabled ? "Screen Shake: ON" : "Screen Shake: OFF", 
                shakeBtn.x + 55, shakeBtn.y + 12, 20, TEXT_COLOR);
        
        // Fullscreen button
        Rectangle fullscreenBtn = { SCREEN_WIDTH/2 - 150, 460, 300, 45 };
        Color fColor = isFullscreen ? Color{60, 150, 60, 255} : BUTTON_COLOR;
        DrawRectangleRounded(fullscreenBtn, 0.3f, 8, fColor);
        DrawText(isFullscreen ? "Fullscreen: ON" : "Fullscreen: OFF", 
                fullscreenBtn.x + 65, fullscreenBtn.y + 12, 20, TEXT_COLOR);
        
        // Back button
        drawModernButton({ SCREEN_WIDTH/2 - 150, 530, 300, 55 }, "BACK", hoveredButton == 4, BUTTON_COLOR);
    }
    
    void drawGameOver() {
        // Karartma
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, { 0, 0, 0, 200 });
        
        // Panel
        DrawRectangleRounded({ SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 150, 400, 380 }, 0.05f, 10, PANEL_COLOR);
        DrawRectangleRoundedLines({ SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 150, 400, 380 }, 0.05f, 10, { 150, 100, 100, 150 });
        
        // Game Over text
        const char* goText = "GAME OVER!";
        int goWidth = MeasureText(goText, 50);
        DrawText(goText, (SCREEN_WIDTH - goWidth) / 2, SCREEN_HEIGHT/2 - 120, 50, { 255, 100, 100, 255 });
        
        // Score
        DrawText("Your Score", SCREEN_WIDTH/2 - 55, SCREEN_HEIGHT/2 - 50, 24, GRAY);
        const char* scoreText = TextFormat("%d", score);
        int scoreWidth = MeasureText(scoreText, 48);
        DrawText(scoreText, (SCREEN_WIDTH - scoreWidth) / 2, SCREEN_HEIGHT/2 - 20, 48, TEXT_COLOR);
        
        // High score
        if (score >= highScore) {
            DrawText("NEW RECORD!", SCREEN_WIDTH/2 - 65, SCREEN_HEIGHT/2 + 35, 24, GOLD_COLOR);
        }
        
        // Buttons
        drawModernButton({ SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 + 70, 260, 60 }, "PLAY AGAIN", hoveredButton == 0, { 60, 150, 60, 255 });
        drawModernButton({ SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 + 150, 260, 60 }, "MAIN MENU", hoveredButton == 1, BUTTON_COLOR);
    }
    
    void draw() {
        BeginDrawing();
        ClearBackground(BACKGROUND);
        
        drawTitleBar();
        
        switch (gameState) {
            case STATE_MENU:
                drawMenu();
                break;
            case STATE_PLAYING:
                drawUI();
                drawGrid();
                if (draggingBlock) drawGhost(*draggingBlock);
                drawParticles();
                for (int i = 0; i < 3; i++) {
                    if (availableBlocks[i].active) drawBlock(availableBlocks[i]);
                }
                break;
            case STATE_SETTINGS:
                drawSettings();
                break;
            case STATE_GAME_OVER:
                drawUI();
                drawGrid();
                drawParticles();
                for (int i = 0; i < 3; i++) {
                    if (availableBlocks[i].active) drawBlock(availableBlocks[i]);
                }
                drawGameOver();
                break;
        }
        
        EndDrawing();
    }
    
    void run() {
        while (!WindowShouldClose()) {
            update();
            draw();
        }
    }
};

int main() {
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Block Blast");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    
    // Set window icon for taskbar
    Image icon = LoadImage("icon.png");
    if (icon.data != NULL) {
        SetWindowIcon(icon);
        UnloadImage(icon);
    }
    
    // Center window
    int monitorWidth = GetMonitorWidth(0);
    int monitorHeight = GetMonitorHeight(0);
    SetWindowPosition((monitorWidth - SCREEN_WIDTH) / 2, (monitorHeight - SCREEN_HEIGHT) / 2);
    
    Game game;
    game.run();
    
    CloseWindow();
    return 0;
}
