#include "raylib.h"
#include "Math.h"
#include <cmath> // Required for sqrtf

// Custom Vector2Distance function
float Vector2Distance(Vector2 v1, Vector2 v2)
{
    return sqrtf((v2.x - v1.x) * (v2.x - v1.x) + (v2.y - v1.y) * (v2.y - v1.y));
}


#include <cassert>
#include <array>
#include <vector>
#include <algorithm>

const float SCREEN_SIZE = 800;

const int TILE_COUNT = 20;
const float TILE_SIZE = SCREEN_SIZE / TILE_COUNT;

enum TileType : int
{
    GRASS,      // Marks unoccupied space, can be overwritten 
    DIRT,       // Marks the path, cannot be overwritten
    WAYPOINT,   // Marks where the path turns, cannot be overwritten
    TURRET,    // New Turret Tile
    COUNT
};

struct Cell
{
    int row;
    int col;
};

constexpr std::array<Cell, 4> DIRECTIONS{ Cell{ -1, 0 }, Cell{ 1, 0 }, Cell{ 0, -1 }, Cell{ 0, 1 } };

inline bool InBounds(Cell cell, int rows = TILE_COUNT, int cols = TILE_COUNT)
{
    return cell.col >= 0 && cell.col < cols && cell.row >= 0 && cell.row < rows;
}

void DrawTile(int row, int col, Color color)
{
    DrawRectangle(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, color);
}

void DrawTile(int row, int col, int type)
{
    Color color = type > 0 ? BEIGE : GREEN;
    DrawTile(row, col, color);
}

Vector2 TileCenter(int row, int col)
{
    float x = col * TILE_SIZE + TILE_SIZE * 0.5f;
    float y = row * TILE_SIZE + TILE_SIZE * 0.5f;
    return { x, y };
}

Vector2 TileCorner(int row, int col)
{
    float x = col * TILE_SIZE;
    float y = row * TILE_SIZE;
    return { x, y };
}

// Returns a collection of adjacent cells that match the search value.
std::vector<Cell> FloodFill(Cell start, int tiles[TILE_COUNT][TILE_COUNT], TileType searchValue)
{
    // "open" = "places we want to search", "closed" = "places we've already searched".
    std::vector<Cell> result;
    std::vector<Cell> open;
    bool closed[TILE_COUNT][TILE_COUNT];
    for (int row = 0; row < TILE_COUNT; row++)
    {
        for (int col = 0; col < TILE_COUNT; col++)
        {
            // We don't want to search zero-tiles, so add them to closed!
            closed[row][col] = tiles[row][col] == 0;
        }
    }

    // Add the starting cell to the exploration queue & search till there's nothing left!
    open.push_back(start);
    while (!open.empty())
    {
        // Remove from queue and prevent revisiting
        Cell cell = open.back();
        open.pop_back();
        closed[cell.row][cell.col] = true;

        // Add to result if explored cell has the desired value
        if (tiles[cell.row][cell.col] == searchValue)
            result.push_back(cell);

        // Search neighbours
        for (Cell dir : DIRECTIONS)
        {
            Cell adj = { cell.row + dir.row, cell.col + dir.col };
            if (InBounds(adj) && !closed[adj.row][adj.col] && tiles[adj.row][adj.col] > 0)
                open.push_back(adj);
        }
    }

    return result;
}

struct Enemy
{
    Vector2 position;  // Current position of the enemy
    float speed;       // Movement speed of the enemy
    int currentWaypoint; // Index of the current waypoint the enemy is heading toward
    float health;      // Enemy health
    float radius;      // For rendering and collision detection
    bool active;       // Whether the enemy is still alive
};

struct Turret
{
    Vector2 position;     // Position of the turret
    float range;          // How far the turret can shoot
    float fireRate;       // Time between shots
    float reloadTime;     // Tracks time since last shot
    float bulletSpeed;    // Speed of bullets fired
    float damage;         // How much damage the turret does per shot
    bool active;          // Whether the turret is active
};


struct Bullet
{
    Vector2 position{};
    Vector2 direction{};
    float time = 0.0f;
    bool enabled = true;
};

int main()
{
    // TODO - Modify this grid to contain turret tiles. Instantiate turret objects accordingly
    int tiles[TILE_COUNT][TILE_COUNT]
    {
        //col:0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19    row:
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 }, // 0
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 1
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 2
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 3
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 4
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 5
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 6
            { 0, 3, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0 }, // 7
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0 }, // 9
            { 0, 0, 0, 1, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 10
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 11
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 12
            { 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 3, 0 }, // 13
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 14
            { 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0 }, // 15
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 16
            { 0, 0, 0, 0, 0, 0, 0, 3, 0, 2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0 }, // 17
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 18
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0 }  // 19
    };

    std::vector<Cell> waypoints = FloodFill({ 0, 12 }, tiles, WAYPOINT);
    size_t curr = 0;
    size_t next = curr + 1;
    bool atEnd = false;

    Vector2 enemyPosition = TileCenter(waypoints[curr].row, waypoints[curr].col);
    const float enemySpeed = 250.0f;
    const float enemyRadius = 20.0f;

    const float bulletTime = 1.0f;
    const float bulletSpeed = 500.0f;
    const float bulletRadius = 15.0f;

    std::vector<Bullet> bullets;
    float shootCurrent = 0.0f;
    float shootTotal = 0.25f;

    std::vector<Enemy> enemies; // Collection to store all enemies
    float spawnTimer = 0.0f;    
    float spawnInterval = 1.0f; 
    int enemyCount = 0;         
    int maxEnemies = 10;        

    std::vector<Turret> turrets; // Collection to store all turrets
    for (int row = 0; row < TILE_COUNT; row++)
    {
        for (int col = 0; col < TILE_COUNT; col++)
        {
            if (tiles[row][col] == TURRET) // If it's a turret tile
            {
                // Create a turret and  its position and attributes
                Turret turret;
                turret.position = TileCenter(row, col); // Position at tile center
                turret.range = 250.0f;    
                turret.fireRate = 0.8f;   
                turret.reloadTime = 0.0f; 
                turret.bulletSpeed = 500.0f; 
                turret.damage = 15.0f;    
                turret.active = true;     

                turrets.push_back(turret);
            }
        }
    }



    InitWindow(SCREEN_SIZE, SCREEN_SIZE, "Tower Defense");
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {

        // TODO - Make enemies follow the path
        float dt = GetFrameTime();
        spawnTimer += dt;
        if (spawnTimer >= spawnInterval && enemyCount < maxEnemies)
        {
            spawnTimer = 0.0f; // Reset timer

            // Initialize a new enemy
            Enemy enemy;
            enemy.position = TileCenter(waypoints[0].row, waypoints[0].col); // Start at the first waypoint
            enemy.speed = enemySpeed;  // Use a predefined speed, or set one for each enemy
            enemy.currentWaypoint = 0; // Starting at the first waypoint
            enemy.health = 150.0f;     // Example health
            enemy.radius = enemyRadius;
            enemy.active = true;

            enemies.push_back(enemy);  // Add the enemy to the list
            enemyCount++;              // Increment the number of spawned enemies
        }

        // Update each enemy's position and movement along the path
        for (Enemy& enemy : enemies)
        {
            if (enemy.active)
            {
                Vector2 from = TileCenter(waypoints[enemy.currentWaypoint].row, waypoints[enemy.currentWaypoint].col);
                Vector2 to = TileCenter(waypoints[enemy.currentWaypoint + 1].row, waypoints[enemy.currentWaypoint + 1].col);
                Vector2 direction = Normalize(to - from);

                enemy.position = enemy.position + direction * enemy.speed * dt;

                // Check if the enemy has reached the next waypoint
                if (CheckCollisionPointCircle(enemy.position, to, enemy.radius))
                {
                    enemy.currentWaypoint++;
                    if (enemy.currentWaypoint >= waypoints.size() - 1)
                    {
                        enemy.active = false; // Enemy has reached the end of the path
                    }
                }
            }
        }
        for (Turret& turret : turrets)
        {
            turret.reloadTime += dt; // Increment reload time

            Enemy* nearestEnemy = nullptr;
            float nearestDistance = FLT_MAX;

            // Find the nearest active enemy within range
            for (Enemy& enemy : enemies)
            {
                if (enemy.active)
                {
                    float distance = Vector2Distance(turret.position, enemy.position);
                    if (distance < nearestDistance && distance <= turret.range)
                    {
                        nearestEnemy = &enemy;
                        nearestDistance = distance;
                    }
                }
            }

            // If there's an enemy in range and the turret is ready to fire
            if (nearestEnemy && turret.reloadTime >= turret.fireRate)
            {
                turret.reloadTime = 0.0f; // Reset reload timer

                // Fire a bullet at the nearest enemy
                Bullet bullet;
                bullet.position = turret.position; // Bullet starts from the turret
                bullet.direction = Normalize(nearestEnemy->position - turret.position); // Aim at the enemy
                bullet.enabled = true; // Bullet is now active
                bullets.push_back(bullet); // Add bullet to the list

                // You can also apply damage directly to the enemy (optional):
                nearestEnemy->health -= turret.damage;
                if (nearestEnemy->health <= 0)
                {
                    nearestEnemy->active = false; // Mark enemy as inactive if dead
                }
            }
        }

        // Path following
        if (!atEnd)
        {
            Vector2 from = TileCenter(waypoints[curr].row, waypoints[curr].col);
            Vector2 to = TileCenter(waypoints[next].row, waypoints[next].col);
            Vector2 direction = Normalize(to - from);
            enemyPosition = enemyPosition + direction * enemySpeed * dt;
            if (CheckCollisionPointCircle(enemyPosition, to, enemyRadius))
            {
                curr++;
                next++;
                atEnd = next == waypoints.size();
                enemyPosition = TileCenter(waypoints[curr].row, waypoints[curr].col);
            }
        }

        

        // TODO - Loop through all bullets & enemies, handle collision & movement accordingly
        for (Bullet& bullet : bullets)
        {
            bullet.position = bullet.position + bullet.direction * bulletSpeed * dt;
            bullet.time += dt;

            bool expired = bullet.time >= bulletTime;
            bool collision = CheckCollisionCircles(enemyPosition, enemyRadius, bullet.position, bulletRadius);
            bullet.enabled = !expired && !collision;
        }

        // Bullet removal
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [&bullets](Bullet bullet) {
                return !bullet.enabled;
            }), bullets.end());

        BeginDrawing();
        ClearBackground(BLACK);
        for (int row = 0; row < TILE_COUNT; row++)
        {
            for (int col = 0; col < TILE_COUNT; col++)
            {
                DrawTile(row, col, tiles[row][col]);
            }
        }
        for (const Enemy& enemy : enemies)
        {
            if (enemy.active)
            {
                DrawCircleV(enemy.position, enemy.radius, RED);
            }
        }
        for (const Turret& turret : turrets)
        {
            DrawCircleV(turret.position, TILE_SIZE * 0.3f, YELLOW); // Draw turret as a yellow circle
        }
        // Render bullets
        for (const Bullet& bullet : bullets)
            DrawCircleV(bullet.position, bulletRadius, BLUE);
        DrawText(TextFormat("Total bullets: %i", bullets.size()), 10, 10, 20, BLUE);
        DrawText(TextFormat("Enemies spawned: %i", enemies.size()), 10, 40, 20, BLUE);  // Add this line
        EndDrawing();
    }
    CloseWindow();
    return 0;
}