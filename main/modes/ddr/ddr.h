#ifndef _DDR_MODE_H_
#define _DDR_MODE_H_
 
#include "swadge2024.h"
 
//==============================================================================
// Defines
//==============================================================================

/// Physics math is done with fixed point numbers where the bottom four bits are the fractional part. It's like Q28.4
/*
#define DECIMAL_BITS 4

#define BALL_RADIUS   (5 << DECIMAL_BITS)
#define PADDLE_WIDTH  (8 << DECIMAL_BITS)
#define PADDLE_HEIGHT (40 << DECIMAL_BITS)
#define FIELD_HEIGHT  (TFT_HEIGHT << DECIMAL_BITS)
#define FIELD_WIDTH   (TFT_WIDTH << DECIMAL_BITS)

#define SPEED_LIMIT (30 << DECIMAL_BITS)
*/

#define DDR_ARROW_HEIGHT 20
#define DDR_TRACK_SPEED 4

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief Enum of screens that may be shown in ddr mode
 */
typedef enum
{
    DDR_MENU,
    DDR_GAME,
} ddrScreen_t;

/**
 * @brief Enum of songs that may be played in ddr mode
 */
typedef enum
{
    DDR_SONG_1,
    DDR_SONG_2,
    DDR_SONG_3,
} ddrSong_t;

typedef enum
{
    DDR_UP,
    DDR_DOWN,
    DDR_LEFT,
    DDR_RIGHT,
} ddrArrow_t;

/**
 * @brief Enum of track difficulties
 */
typedef enum
{
    DDR_EASY,
    DDR_MEDIUM,
    DDR_HARD,
} ddrDifficulty_t;



//==============================================================================
// Structs
//==============================================================================

typedef struct 
{
	uint8_t x;
	uint8_t y;
} note_t;

typedef struct
{
	menu_t* menu;                               ///< The menu structure
    menuLogbookRenderer_t* menuLogbookRenderer; ///< Renderer for the menu
    font_t ibm;                                 ///< The font used in the menu and game
    p2pInfo p2p;                                ///< Peer to peer connectivity info, currently unused
    
	ddrScreen_t screen;		///< The screen being displayed
	ddrSong_t currSong;		///< The level being played
    uint8_t bpm;			///< BPM for the current song
	list_t* notes;
    int32_t usPerBeat;
    int32_t usBeatCtr;
	bool songIsCounting;

	ddrDifficulty_t difficulty;					///< The difficulty of the track

    uint8_t score;            ///< The score for the game

	int16_t beats[4];			///< y position of beats on track

    int32_t restartTimerUs; ///< A timer that counts down before the game begins
    uint16_t btnState;      ///< The button state used for paddle control
    bool isPaused;          ///< true if the game is paused, false if it is running
	
	wsg_t arrowWsg; 		///< A graphic for the arrow at the top
    wsg_t arrowMoveWsg;   	///< A graphic for the arrow that moves

    song_t song1;  ///< Background music
    song_t song2;  ///< Background music
    song_t song3;  ///< Background music
    //song_t hit1; ///< Sound effect for one paddle's hit
    //song_t hit2; ///< Sound effect for the other paddle's hit

    //led_t ledL;           ///< The left LED color
    //led_t ledR;           ///< The right LED color
    //int32_t ledFadeTimer; ///< The timer to fade LEDs
} ddr_t;

//==============================================================================
// Function Prototypes
//==============================================================================

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
static const char ddrName[]  = "DDR";
 
static void ddrEnterMode(void);
static void ddrExitMode(void);
static void ddrMainLoop(int64_t elapsedUs);
static void ddrGameLoop(int64_t elapsedUs);
static void ddrMenuCb(const char* label, bool selected, uint32_t settingVal);
static void ddrAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void ddrBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void ddrDrawField(void);
static void ddrEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void ddrEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static int16_t ddrAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet);

static void ddrResetGame(bool isInit);
static void ddrMoveTrack(void);
static void setBPM(uint8_t bpm);

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */

static const char ddrSong1[] = "Follinesque";
static const char ddrSong2[] = "Song 2";
static const char ddrSong3[] = "Song 3";

static const char ddrDiffEasy[]   = "Easy";
static const char ddrDiffMedium[] = "Medium";
static const char ddrDiffHard[]   = "Impossible";

static const char ddrPaused[] = "Paused";

extern swadgeMode_t ddrMode;
 
#endif