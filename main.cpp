#include <iostream>
#include <SDL.h>
#include <SDL_mixer.h>
#include <emscripten.h>
#include <vector>

struct Projectile {
    float x;
    float y;
    float speedX;
    float speedY;
    int size;
};

struct Enemy {
    float x;
    float y;
    float speedX;
    float shootTimer;
    std::vector<Projectile> projectiles;
    int lineIndex;
};

struct GameState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    void* player;
    float x;
    float y;
    float health;
    bool alive;
    bool keys[4] = {false};
    
    float damageTimer;
    float respawnTimer;
    
    const float SPEED = 5.0f;
    const float DAMAGE_INTERVAL = 1.0f;
    const float RESPAWN_TIME = 3.0f;

    std::vector<Enemy*> enemies;

    // Sound effects
    Mix_Chunk* deathSound;
    Mix_Chunk* damageSound;
};

bool checkCollision(float x1, float y1, int size1, float x2, float y2, int size2) {
    return (x1 < x2 + size2 &&
            x1 + size1 > x2 &&
            y1 < y2 + size2 &&
            y1 + size1 > y2);
}


void initPlayer(GameState* state) {
    state->x = 400;
    state->y = 300;
    state->health = 100.0f;
    state->alive = true;
    state->damageTimer = 0.0f;
    state->respawnTimer = 0.0f;
}

void updatePlayerStatus(GameState* state) {
    if (state->player != nullptr && state->health > 0) {
        state->alive = true;
    } else {
        state->alive = false;
    }
}

bool damagePlayer(GameState* state, float damage) {
    if (state->alive) {
        // Play damage sound when hit, before losing health
        if (state->damageSound) {
            Mix_PlayChannel(-1, state->damageSound, 0);
        }

        state->health -= damage;
        if (state->health <= 0) {
            state->alive = false;
            // Play death sound when health reaches zero
            if (state->deathSound) {
                Mix_PlayChannel(-1, state->deathSound, 0);
            }
            return true;
        }
    }
    return false;
}


bool healPlayer(GameState* state, float healing) {
    if (state->alive) {
        state->health = std::min(state->health + healing, 100.0f);
    }
}

bool DestroyPlayer(GameState* state)
{
    return state->health <= 0;
}

bool Respawner(GameState* state)
{
if(state->health <= 0)
{
    initPlayer(state);
    return true;
}
return false;
}

void handleEvents(GameState* state) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_w: state->keys[0] = true; break;
                    case SDLK_a: state->keys[1] = true; break;
                    case SDLK_s: state->keys[2] = true; break;
                    case SDLK_d: state->keys[3] = true; break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_w: state->keys[0] = false; break;
                    case SDLK_a: state->keys[1] = false; break;
                    case SDLK_s: state->keys[2] = false; break;
                    case SDLK_d: state->keys[3] = false; break;
                }
                break;
            case SDL_QUIT:
                exit(0);
                break;
        }
    }
}

void mainLoop(void* arg) {
    GameState* state = (GameState*)arg;
    float deltaTime = 1.0f / 60.0f;
    const int SCREEN_WIDTH = 800;
    const int SCREEN_HEIGHT = 600;
    const int ENTITY_SIZE = 50;
    const int PROJECTILE_SIZE = 10;

    handleEvents(state);

    int fixedLines[2] = {100, 400};

    if (state->alive) {
        state->damageTimer += deltaTime;

        // Enemy logic
        for (auto* enemy : state->enemies) {
            enemy->shootTimer += deltaTime;

            // Enemy shooting
            if (enemy->shootTimer >= 2.0f) {
                Projectile proj;
                proj.x = enemy->x + ENTITY_SIZE / 2;
                proj.y = enemy->y + ENTITY_SIZE / 2;
                proj.speedX = (state->x - proj.x) * 0.03f;
                proj.speedY = (state->y - proj.y) * 0.03f;
                proj.size = PROJECTILE_SIZE;
                enemy->projectiles.push_back(proj);
                enemy->shootTimer = 0.0f;
            }

            // Move projectiles and check collisions
            for (auto it = enemy->projectiles.begin(); it != enemy->projectiles.end();) {
                it->x += it->speedX;
                it->y += it->speedY;

                // Check player collision
                if (checkCollision(state->x, state->y, ENTITY_SIZE, it->x, it->y, it->size)) {
                    damagePlayer(state, 10.0f);
                    it = enemy->projectiles.erase(it);
                    continue;
                }

                // Remove projectiles off screen
                if (it->x < 0 || it->x > SCREEN_WIDTH || it->y < 0 || it->y > SCREEN_HEIGHT) {
                    it = enemy->projectiles.erase(it);
                } else {
                    ++it;
                }
            }

            // Enemy movement
            enemy->x += enemy->speedX;
            enemy->y = fixedLines[enemy->lineIndex];

            if (enemy->x <= 0 || enemy->x >= SCREEN_WIDTH - ENTITY_SIZE) {
                enemy->speedX *= -1;
                enemy->lineIndex = 1 - enemy->lineIndex;
            }
        }

        // Player movement
        if (state->keys[0] && state->y > 0) state->y -= state->SPEED;           // W key (up)
        if (state->keys[1] && state->x > 0) state->x -= state->SPEED;           // A key (left)
        if (state->keys[2] && state->y < SCREEN_HEIGHT - ENTITY_SIZE) state->y += state->SPEED;  // S key (down)
        if (state->keys[3] && state->x < SCREEN_WIDTH - ENTITY_SIZE) state->x += state->SPEED;  // D key (right)
    } else {
        // Respawn timer logic
        state->respawnTimer += deltaTime;
        if (state->respawnTimer >= state->RESPAWN_TIME) {
            initPlayer(state);
            state->respawnTimer = 0.0f;
        }
    }

    // Rendering
    SDL_SetRenderDrawColor(state->renderer, 0, 0, 255, 255);
    SDL_RenderClear(state->renderer);

    if (state->alive) {
        // Draw player
        SDL_Rect playerRect = {static_cast<int>(state->x), static_cast<int>(state->y), 50, 50};
        SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(state->renderer, &playerRect);

        // Draw health bar
        SDL_Rect healthBarBg = {static_cast<int>(state->x), static_cast<int>(state->y) - 10, 50, 5};
        SDL_SetRenderDrawColor(state->renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(state->renderer, &healthBarBg);

        SDL_Rect healthBar = {
            static_cast<int>(state->x), 
            static_cast<int>(state->y) - 10, 
            static_cast<int>(50 * (state->health / 100.0f)), 
            5
        };
        SDL_SetRenderDrawColor(state->renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(state->renderer, &healthBar);
    }

    // Draw enemies and their projectiles
    for (const auto* enemy : state->enemies) {
        SDL_Rect enemyRect = {
            static_cast<int>(enemy->x), 
            static_cast<int>(enemy->y), 
            50, 
            50
        };
        SDL_SetRenderDrawColor(state->renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(state->renderer, &enemyRect);

        // Draw projectiles
        for (const auto& proj : enemy->projectiles) {
            SDL_Rect projRect = {
                static_cast<int>(proj.x), 
                static_cast<int>(proj.y), 
                proj.size, 
                proj.size
            };
            SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(state->renderer, &projRect);
        }
    }

    SDL_RenderPresent(state->renderer);
}

int main() {
    // Initialize SDL with video and audio
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        SDL_Quit();
        return 1;
    }

    GameState* state = new GameState();

    // Load sound effects (IMPORTANT: Replace with your actual sound file paths)
    state->deathSound = Mix_LoadWAV("anime-cat-girl-6731.wav");
    state->damageSound = Mix_LoadWAV("anime-cat-girl-105182.wav");

    if (!state->deathSound) {
        printf("Failed to load death sound! SDL_mixer Error: %s\n", Mix_GetError());
    }
    if (!state->damageSound) {
        printf("Failed to load damage sound! SDL_mixer Error: %s\n", Mix_GetError());
    }

    // Create 5 enemies
    for (int i = 0; i < 5; ++i) {
        Enemy* enemy = new Enemy();
        enemy->x = 100 + i * 120;  // Spread enemies horizontally
        enemy->y = (i % 2 == 0) ? 100 : 400;  // Alternate lines
        enemy->speedX = 2 * (i % 2 == 0 ? 1 : -1);  // Alternate directions
        enemy->shootTimer = 0.0f;
        enemy->lineIndex = (i % 2 == 0) ? 0 : 1;
        state->enemies.push_back(enemy);
    }

    state->window = SDL_CreateWindow(
        "WASD Game with Multiple Enemies",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        800,
        600,
        SDL_WINDOW_SHOWN
    );

    state->renderer = SDL_CreateRenderer(
        state->window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    initPlayer(state);

    emscripten_set_main_loop_arg(mainLoop, state, 0, 1);

    // Cleanup
    if (state->deathSound) Mix_FreeChunk(state->deathSound);
    if (state->damageSound) Mix_FreeChunk(state->damageSound);
    
    Mix_CloseAudio();
    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->window);
    SDL_Quit();

    // Free enemies
    for (auto* enemy : state->enemies) {
        delete enemy;
    }
    delete state;

    return 0;
}