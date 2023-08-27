//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include "mode_ray.h"
#include "ray_map_loader.h"
#include "ray_renderer.h"
#include "ray_object.h"
#include "ray_tex_manager.h"

//==============================================================================
// Function Prototypes
//==============================================================================

void rayEnterMode(void);
void rayExitMode(void);
void rayMainLoop(int64_t elapsedUs);
void rayBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static bool isPassableCell(rayMapCell_t* cell);
void setPlayerAngle(q24_8 angle);

//==============================================================================
// Const Variables
//==============================================================================

const char rayName[] = "Magtroid Pocket";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t rayMode = {
    .modeName                 = rayName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = rayEnterMode,
    .fnExitMode               = rayExitMode,
    .fnMainLoop               = rayMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = rayBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

ray_t* ray;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void rayEnterMode(void)
{
    // Allocate memory
    ray = calloc(1, sizeof(ray_t));

    // Set invalid IDs for all objects
    for (uint16_t objIdx = 0; objIdx < MAX_RAY_OBJS; objIdx++)
    {
        ray->objs[objIdx].id = -1;
    }

    // Load the map and object data
    loadRayMap("demo.rmh", ray, false);

    // Set initial position and direction, centered on the tile
    ray->posX = TO_FX(ray->posX) + (1 << (FRAC_BITS - 1));
    ray->posY = TO_FX(ray->posY) + (1 << (FRAC_BITS - 1));
    setPlayerAngle(TO_FX(0));
    ray->posZ = TO_FX(0);

    // Initialize texture manager
    initLoadedTextures(ray);

    // Create an array for all LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    // Set the LED output
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void rayExitMode(void)
{
    freeRayMap(&ray->map);
    freeAllTex(ray);
    free(ray);
}

/**
 * @brief This function is called from the main loop. It's pretty quick, but the timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
void rayMainLoop(int64_t elapsedUs)
{
    // Check all queued button events
    uint16_t prevBtnState = ray->btnState;
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        ray->btnState = evt.state;
    }

    // Move objects, check logic, etc.
    moveRayObjects(ray, elapsedUs);
    // Draw the walls. The background is already drawn in rayBackgroundDrawCallback()
    castWalls(ray);
    // Draw sprites
    rayObj_t* centeredSprite = castSprites(ray);

    // Run a timer for head bob
    ray->bobTimer += elapsedUs;
    while (ray->bobTimer > 2500)
    {
        ray->bobTimer -= 2500;

        // Only bob when walking or finishing a bob cycle
        if ((ray->btnState & (PB_UP | PB_DOWN)) || (0 != ray->bobCount && 180 != ray->bobCount))
        {
            // Step through the bob cycle, which is a sin function
            ray->bobCount++;
            if (360 == ray->bobCount)
            {
                ray->bobCount = 0;
            }
            // Bob the camera. Note that fixed point numbers are << 8, and trig functions are << 10
            ray->posZ = getSin1024(ray->bobCount) * 4;
        }
        else
        {
            // Reset the count to always restart on an upward bob
            ray->bobCount = 0;
        }
    }

    // Run a timer to open and close doors
    ray->doorTimer += elapsedUs;
    while (ray->doorTimer >= 5000)
    {
        ray->doorTimer -= 5000;

        for (int16_t y = 0; y < ray->map.h; y++)
        {
            for (int16_t x = 0; x < ray->map.w; x++)
            {
                if (ray->map.tiles[x][y].doorOpen > 0 && ray->map.tiles[x][y].doorOpen < TO_FX(1))
                {
                    ray->map.tiles[x][y].doorOpen++;
                }
            }
        }
    }

    // Find move distances
    // TODO scale with elapsed time
    // TODO normalize when moving diagonally
    q24_8 deltaX = 0;
    q24_8 deltaY = 0;

    // B button strafes, which may lock on an enemy
    if ((ray->btnState & PB_B) && !(prevBtnState & PB_B))
    {
        // Set strafe to true
        ray->isStrafing = true;
    }
    else if (!(ray->btnState & PB_B) && (prevBtnState & PB_B))
    {
        // Set strafe to false
        ray->isStrafing = false;
    }

    // Strafing is either locked or unlocked
    if (ray->isStrafing)
    {
        if (centeredSprite)
        {
            // Adjust position to always center on the locked target object
            int32_t newAngle = cordicAtan2(centeredSprite->posX - ray->posX, ray->posY - centeredSprite->posY);
            setPlayerAngle(TO_FX(newAngle));
            // TODO this style of strafe (adjust angle, move tangentially) makes the player slowly spiral outward
        }

        if (ray->btnState & PB_RIGHT)
        {
            // Strafe right
            deltaX -= (ray->dirY / 6);
            deltaY += (ray->dirX / 6);
        }
        else if (ray->btnState & PB_LEFT)
        {
            // Strafe left
            deltaX += (ray->dirY / 6);
            deltaY -= (ray->dirX / 6);
        }
    }
    else
    {
        // Rotate right, in place
        if (ray->btnState & PB_RIGHT)
        {
            q24_8 newAngle = ADD_FX(ray->dirAngle, TO_FX(5));
            if (newAngle >= TO_FX(360))
            {
                newAngle -= TO_FX(360);
            }
            setPlayerAngle(newAngle);
        }

        // Rotate left, in place
        if (ray->btnState & PB_LEFT)
        {
            q24_8 newAngle = SUB_FX(ray->dirAngle, TO_FX(5));
            if (newAngle < TO_FX(0))
            {
                newAngle += TO_FX(360);
            }
            setPlayerAngle(newAngle);
        }
    }

    // If the up button is held
    if (ray->btnState & PB_UP)
    {
        // Move forward
        deltaX += (ray->dirX / 6);
        deltaY += (ray->dirY / 6);
    }
    // Else if the down button is held
    else if (ray->btnState & PB_DOWN)
    {
        // Move backwards
        deltaX -= (ray->dirX / 6);
        deltaY -= (ray->dirY / 6);
    }

    // Boundary checks are longer than the move dist to not get right up on the wall
    q24_8 boundaryCheckX = deltaX * 2;
    q24_8 boundaryCheckY = deltaY * 2;

    // Move forwards if no wall in front of you
    if (isPassableCell(&ray->map.tiles[FROM_FX(ray->posX + boundaryCheckX)][FROM_FX(ray->posY)]))
    {
        ray->posX += deltaX;
    }

    if (isPassableCell(&ray->map.tiles[FROM_FX(ray->posX)][FROM_FX(ray->posY + boundaryCheckY)]))
    {
        ray->posY += deltaY;
    }

    if ((ray->btnState & PB_A) && !(prevBtnState & PB_A))
    {
        // TODO shoot different things
        for (uint16_t newIdx = 0; newIdx < MAX_RAY_OBJS; newIdx++)
        {
            if (-1 == ray->objs[newIdx].id)
            {
                wsg_t* texture           = getTexByType(ray, OBJ_BULLET_NORMAL);
                ray->objs[newIdx].sprite = texture;
                ray->objs[newIdx].dist   = 0;
                ray->objs[newIdx].posX   = ray->posX + ray->dirX / 2;
                ray->objs[newIdx].posY   = ray->posY + ray->dirY / 2;
                ray->objs[newIdx].velX   = ray->dirX;
                ray->objs[newIdx].velY   = ray->dirY;
                ray->objs[newIdx].radius = DIV_FX(TO_FX(texture->w), TO_FX(64));
                ray->objs[newIdx].type   = OBJ_BULLET_NORMAL;
                ray->objs[newIdx].id     = 0;
                break;
            }
        }
    }
}

/**
 * @brief Helper function to set the angle the player is facing and associated camera variables
 *
 * @param angle The angle the player is facing. Must be in the range [0, 359]. 0 is north
 */
void setPlayerAngle(q24_8 angle)
{
    // The angle the player is facing
    ray->dirAngle = angle;

    // Compute cartesian direction from angular direction
    // trig functions are already << 10, so / 4 to get to << 8
    ray->dirY = -getCos1024(FROM_FX(ray->dirAngle)) / 4;
    ray->dirX = getSin1024(FROM_FX(ray->dirAngle)) / 4;

    // the 2d rayCaster version of camera plane, orthogonal to the direction vector and scaled to 2/3
    ray->planeX = MUL_FX(-((1 << FRAC_BITS) * 2) / 3, ray->dirY);
    ray->planeY = MUL_FX(((1 << FRAC_BITS) * 2) / 3, ray->dirX);
}

/**
 * @brief This function is called when the display driver wishes to update a section of the display.
 *
 * @param disp The display to draw to
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
void rayBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Draw a portion of the background
    castFloorCeiling(ray, y, y + h);
}

/**
 * @brief Check if a cell is currently passable
 *
 * @param cell The cell type to check
 * @return true if the cell can be passed through, false if it cannot
 */
static bool isPassableCell(rayMapCell_t* cell)
{
    if (CELL_IS_TYPE(cell->type, BG | WALL))
    {
        // Never pass through walls
        return false;
    }
    else if (CELL_IS_TYPE(cell->type, BG | DOOR))
    {
        // Only pass through open doors
        return (TO_FX(1) == cell->doorOpen);
    }
    else
    {
        // Always pass through everything else
        return true;
    }
}
