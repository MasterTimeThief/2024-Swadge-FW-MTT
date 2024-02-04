/**
 * @file ddr.c
 * @author MasterTimeThief (mastertimethief@gmail.com)
 * @brief An example DDR game
 * @date 2024-01-27
 *
 * TODO networking
 */

//==============================================================================
// Includes
//==============================================================================

#include "esp_random.h"
#include "ddr.h"



//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for DDR
swadgeMode_t ddrMode = {
    .modeName                 = ddrName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = ddrEnterMode,
    .fnExitMode               = ddrExitMode,
    .fnMainLoop               = ddrMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = ddrBackgroundDrawCallback,
    .fnEspNowRecvCb           = ddrEspNowRecvCb,
    .fnEspNowSendCb           = ddrEspNowSendCb,
    .fnAdvancedUSB            = NULL,
};

bool ddrDebug = true;


const char* track1[] = {
    "0100",
    "0000",
    "0100",
    "0000",
    "1000",
    "0000",
    "0001",
    "0010",
    
    "0100",
    "0000",
    "0001",
    "0010",
    "0100",
    "0000",
    "1000",
    "0000",
    
    "0100",
    "0000",
    "0100",
    "0000",
    "1000",
    "0000",
    "0001",
    "0010",
    
    "0100",
    "0000",
    "1000",
    "0001",
    "0010",
    "0000",
    "0001",
    "0000",
    
    "0100",
    "0000",
    "0100",
    "0000",
    "1000",
    "0000",
    "0001",
    "0010",
    
    "0100",
    "0000",
    "0001",
    "0010",
    "0100",
    "0000",
    "1000",
    "0000",
    
    "0100",
    "0000",
    "0100",
    "0000",
    "1000",
    "0000",
    "0001",
    "0010",
    
    "0100",
    "0000",
    "0100",
    "0000",
    "0001",
    "0000",
    "0010",
    "0000",
};




/// All state information for the DDR mode. This whole struct is calloc()'d and free()'d so that DDR is only
/// using memory while it is being played
ddr_t* ddr = NULL;

//==============================================================================
// Functions
//==============================================================================

static void ddrEnterMode(void)
{
	// Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    ddr = calloc(1, sizeof(ddr_t));

    // Load all assets

    // Load a font
    loadFont("ibm_vga8.font", &ddr->ibm, false);

	// Load graphics
    loadWsg("ddrArrow.wsg", 	&ddr->arrowWsg, false);
    loadWsg("ddrArrowMove.wsg", &ddr->arrowMoveWsg, false);

    // Load SFX
    loadSong("Follinesque.sng", &ddr->song1, false);
    loadSong("Follinesque.sng", &ddr->song2, false);
    loadSong("Follinesque.sng", &ddr->song3, false);

    ddr->song1.shouldLoop = false;
    ddr->song2.shouldLoop = false;
    ddr->song3.shouldLoop = false;

    ddr->usBeatCtr = 0;
    ddr->beats[0] = DDR_ARROW_HEIGHT + (ddr->arrowWsg.h / 2);
    ddr->beats[1] = ddr->beats[0] + 60;
    ddr->beats[2] = ddr->beats[1] + 60;
    ddr->beats[3] = ddr->beats[2] + 60;

	// Initialize the track
	ddr->notes = calloc(1, sizeof(list_t));


    // Initialize the menu
    ddr->menu                = initMenu(ddrName, ddrMenuCb);
    ddr->menuLogbookRenderer = initMenuLogbookRenderer(&ddr->ibm);

    // These are the possible control schemes
    const char* songList[] = {
        ddrSong1,
        ddrSong2,
        ddrSong3,
    };

    // Add each control scheme to the menu. Each control scheme has a submenu to select difficulty
    for (uint8_t i = 0; i < ARRAY_SIZE(songList); i++)
    {
        // Add a control scheme to the menu. This opens a submenu with difficulties
        ddr->menu = startSubMenu(ddr->menu, songList[i]);
        // Add difficulties to the submenu
        addSingleItemToMenu(ddr->menu, ddrDiffEasy);
        addSingleItemToMenu(ddr->menu, ddrDiffMedium);
        addSingleItemToMenu(ddr->menu, ddrDiffHard);
        // End the submenu
        ddr->menu = endSubMenu(ddr->menu);
    }

    // Set frame rate to 30 FPS
    setFrameRateUs(1000000 / 30);

    // Set the mode to menu mode
    ddr->screen = DDR_MENU;
}
 
static void ddrExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(ddr->menu);
    deinitMenuLogbookRenderer(ddr->menuLogbookRenderer);
    // Free the font
    freeFont(&ddr->ibm);
    // Free graphics
    freeWsg(&ddr->arrowWsg);
    freeWsg(&ddr->arrowMoveWsg);
    // Free the songs
    freeSong(&ddr->song1);
    freeSong(&ddr->song2);
    freeSong(&ddr->song3);
    // Free the json track
    //freeJson(ddr->track);
    // Free everything else
    free(ddr);
}
 
static void ddrMainLoop(int64_t elapsedUs)
{
    // Pick what runs and draws depending on the screen being displayed
    switch (ddr->screen)
    {
        case DDR_MENU:
        {
            // Process button events
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                // Pass button events to the menu
                ddr->menu = menuButton(ddr->menu, evt);
            }

            // Draw the menu
            drawMenuLogbook(ddr->menu, ddr->menuLogbookRenderer, elapsedUs);
            break;
        }
        case DDR_GAME:
        {
            // Run the main game loop. This will also process button events
            ddrGameLoop(elapsedUs);
            break;
        }
    }
}

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void ddrGameLoop(int64_t elapsedUs)
{
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        ddr->btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down && (PB_START == evt.button))
        {
            // Toggle pause
            ddr->isPaused = !ddr->isPaused;
        }
    }

    
    // If the game is paused
    if (ddr->isPaused)
    {
        // Just draw and return
        ddrDrawField();
        return;
    }
    
    
    // While the restart timer is active
    while (ddr->restartTimerUs > 0)
    {
        // Decrement the timer and draw the field, but don't run game logic
        ddr->restartTimerUs -= elapsedUs;
        ddrDrawField();
        return;
    }

    // Do update each loop
    ddrMoveTrack();
    /*
    pongFadeLeds(elapsedUs);
    pongControlPlayerPaddle();
    pongControlCpuPaddle();
    pongUpdatePhysics(elapsedUs);
    */

    // Draw the field
    
    ddrDrawField();
}


static void ddrMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    // Only care about selected items, not scrolled-to items.
    // The same callback is called from the menu and submenu with no indication of which menu it was called from
    // Note that the label arg will be one of the strings used in startSubMenu() or addSingleItemToMenu()
    
	if (selected)
    {
        // Set the song to play (first-level menu)
        if (label == ddrSong1)
        {
            ddr->currSong = DDR_SONG_1;
            setBPM(120);
        }
        // Set the song to play (first-level menu)
        else if (label == ddrSong2)
        {
            ddr->currSong = DDR_SONG_2;
            setBPM(100);
        }
        // Set the song to play (first-level menu)
        else if (label == ddrSong3)
        {
            ddr->currSong = DDR_SONG_3;
            setBPM(100);
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == ddrDiffEasy)
        {
            ddr->difficulty = DDR_EASY;
            ddrResetGame(true);
            ddr->screen = DDR_GAME;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == ddrDiffMedium)
        {
            ddr->difficulty = DDR_MEDIUM;
            ddrResetGame(true);
            ddr->screen = DDR_GAME;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == ddrDiffHard)
        {
            ddr->difficulty = DDR_HARD;
            ddrResetGame(true);
            ddr->screen = DDR_GAME;
        }
    }
	
}

 
static void ddrAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    // Fill this in
}
 
static void ddrBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // 280 x 240 resolution
    accelIntegrate();

    // Use TURBO drawing mode to draw individual pixels fast
    SETUP_FOR_TURBO();

    // Draw a grid
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            if ((0 == xp % 40) || (0 == yp % 40))
            {
                TURBO_SET_PIXEL(xp, yp, c110);
            }
            else
            {
                TURBO_SET_PIXEL(xp, yp, c001);
            }
        }
    }

    // Draw the arrows
    drawWsg(&ddr->arrowWsg, 120, 20, false, false, 270);
    drawWsg(&ddr->arrowWsg, 160, 20, false, false, 180);
    drawWsg(&ddr->arrowWsg, 200, 20, false, false, 0);
    drawWsg(&ddr->arrowWsg, 240, 20, false, false, 90);
}

/**
 * @brief Draw the DDR field to the TFT
 */
static void ddrDrawField(void)
{
    if(ddrDebug) printf("Entered ddrDrawField \n");
    /*
    // Create an array for all LEDs
    led_t leds[CONFIG_NUM_LEDS];
    // Copy the LED colors for left and right to the whole array
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS / 2; i++)
    {
        leds[i]                         = pong->ledL;
        leds[i + (CONFIG_NUM_LEDS / 2)] = pong->ledR;
    }
    // Set the LED output
    setLeds(leds, CONFIG_NUM_LEDS);
    */

    // No need to clear the display before drawing because it's redrawn by pongBackgroundDrawCallback() each time

    
    if(ddrDebug) printf("Draw lines \n");
    //Draw Track
    drawLineFast(120, ddr->beats[0], 280, ddr->beats[0], c050);
    drawLineFast(120, ddr->beats[1], 280, ddr->beats[1], c555);
    drawLineFast(120, ddr->beats[2], 280, ddr->beats[2], c555);
    drawLineFast(120, ddr->beats[3], 280, ddr->beats[3], c555);

	//Draw ONLY notes that should appear
	node_t* currentNode = ddr->notes->first;
	while (currentNode != NULL)
	{
		note_t* note = (note_t*)currentNode->val;
        if(ddrDebug) printf("Note: x:%"PRIu8" y:%"PRIu8" \n", note->x, note->y );
		// Check if note is at top of screen and needs to be removed
		if (note->y <= 0)
		{
			// Remove top entry, and continue
            currentNode = currentNode->next;
			shift(ddr->notes);
			continue;
		}

		// Check if current note is below screen, then stop the loo
		if (note->y > 240)
		{
			break;
		}
		
		//Determine rotation
		int16_t rot = 0;
		if 		(note->x == 120) rot = 270;
		else if (note->x == 160) rot = 180;
		else if (note->x == 200) rot = 0;
		else if (note->x == 240) rot = 90;


		// Draw note onscreen
		//note->y -= DDR_TRACK_SPEED;
		drawWsg(&ddr->arrowMoveWsg, note->x, note->y, false, false, rot);
		currentNode = currentNode->next;
	}

    /*
#ifdef DRAW_SHAPES
    // This will draw the game with geometric shapes, not sprites

    // Bitshift the ball's location and radius from math coordinates to screen coordinates, then draw it
    drawCircleFilled((pong->ball.x) >> DECIMAL_BITS, (pong->ball.y) >> DECIMAL_BITS,
                     (pong->ball.radius) >> DECIMAL_BITS, c555);

    // Bitshift the left paddle's location and radius from math coordinates to screen coordinates, then draw it
    drawRect((pong->paddleL.x >> DECIMAL_BITS), (pong->paddleL.y >> DECIMAL_BITS),
             ((pong->paddleL.x + pong->paddleL.width) >> DECIMAL_BITS),
             ((pong->paddleL.y + pong->paddleL.height) >> DECIMAL_BITS), c333);
    // Bitshift the right paddle's location and radius from math coordinates to screen coordinates, then draw it

    drawRect((pong->paddleR.x >> DECIMAL_BITS), (pong->paddleR.y >> DECIMAL_BITS),
             ((pong->paddleR.x + pong->paddleR.width) >> DECIMAL_BITS),
             ((pong->paddleR.y + pong->paddleR.height) >> DECIMAL_BITS), c333);
#else
    // This will draw the game with sprites, not geometric shapes

    // Draw the ball
    drawWsgSimple(&pong->ballWsg, (pong->ball.x - pong->ball.radius) >> DECIMAL_BITS,
                  (pong->ball.y - pong->ball.radius) >> DECIMAL_BITS);
    // Draw one paddle
    drawWsg(&pong->paddleWsg, pong->paddleL.x >> DECIMAL_BITS, pong->paddleL.y >> DECIMAL_BITS, false, false, 0);
    // Draw the other paddle, flipped
    drawWsg(&pong->paddleWsg, pong->paddleR.x >> DECIMAL_BITS, pong->paddleR.y >> DECIMAL_BITS, true, false, 0);

#endif
*/
    // Set up variables to draw text
    char scoreStr[16] = {0};
    int16_t tWidth;

    // Render a number to a string
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRIu8, ddr->score);
    // Measure the width of the score string
    tWidth = textWidth(&ddr->ibm, scoreStr);
    // Draw the score string to the display, centered at (TFT_WIDTH / 4)
    drawText(&ddr->ibm, c555, scoreStr, (TFT_WIDTH / 4) - (tWidth / 2), 0);



    // If the restart timer is active, draw it
    if (ddr->isPaused)
    {
        // Measure the width of the time string
        tWidth = textWidth(&ddr->ibm, ddrPaused);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&ddr->ibm, c555, ddrPaused, ((TFT_WIDTH - tWidth) / 2), 0);
    }
    else if (ddr->restartTimerUs > 0)
    {
        // Render the time to a string
        snprintf(scoreStr, sizeof(scoreStr) - 1, "%01" PRId32, (ddr->restartTimerUs / ddr->usPerBeat)+1);
        // Measure the width of the time string
        tWidth = textWidth(&ddr->ibm, scoreStr);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&ddr->ibm, c555, scoreStr, ((TFT_WIDTH - tWidth) / 2), 0);
    }
    
}

static void ddrMoveTrack(void)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        ddr->beats[i] -= DDR_TRACK_SPEED;
        if(ddr->beats[i] < 0)
        {
            ddr->beats[i] += 240;
        }
    }

	//Move all notes
	node_t* currentNode = ddr->notes->first;
	while (currentNode != NULL)
	{
		// move up note
		note_t* note = (note_t*)currentNode->val;
		note->y -= DDR_TRACK_SPEED;
		currentNode = currentNode->next;
	}

}

static void ddrSetupTrack(const char *song)
{
    //const char *json = loadJson(song, true);
    //ddr->track = json['track'];
	uint8_t pos = DDR_ARROW_HEIGHT;

	//Just do track 1 for now, make different tracks later
    for (uint8_t i = 0; i < ARRAY_SIZE(track1); i++)
    {
        //check each beat for which notes to add
		for (uint8_t j = 0; j < 4; j++)
		{
			if(track1[i][j] == '1')
			{
				note_t* n = malloc(sizeof(note_t));
				n->x = 120+(j*40);
				n->y = pos;
				push(ddr->notes, n);
			}

		}

		pos += 30;
    }

    
}
 
static void ddrEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // Fill this in
}
 
static void ddrEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // Fill this in
}
 
static int16_t ddrAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet)
{
    // Fill this in
    return 0;
}






/**
 * @brief Reset the DDR game variables
 *
 * @param isInit True if this is the first time this is being called, false if it is reset after a player scores
 */
static void ddrResetGame(bool isInit)
{
    
	// Set different variables based on initialization
    if (isInit)
    {
        //Load track + Start playing music
        switch(ddr->currSong)
        {
            case DDR_SONG_1:
            {
                ddrSetupTrack("song1.json");
                bzrPlayBgm(&ddr->song1, BZR_STEREO);
            }
            case DDR_SONG_2:
            {
                ddrSetupTrack("song2.json");
                bzrPlayBgm(&ddr->song2, BZR_STEREO);
            }
            case DDR_SONG_3:
            {
                ddrSetupTrack("song3.json");
                bzrPlayBgm(&ddr->song3, BZR_STEREO);
            }
        }
    }
    else
    {
        //idk
    }

    // Set the restart timer
    //ddr->restartTimerUs = 2000000;
    ddr->restartTimerUs = ddr->usPerBeat * 4;


    /*
    // Set the ball variables
    pong->ball.x      = (FIELD_WIDTH) / 2;
    pong->ball.y      = (FIELD_HEIGHT) / 2;
    pong->ball.radius = BALL_RADIUS;

    // Give the ball initial velocity to the top right
    pong->ballVel.x = (4 << DECIMAL_BITS);
    pong->ballVel.y = -(4 << DECIMAL_BITS);

    // Rotate up to 90 degrees, for some randomness
    pong->ballVel = rotateVec2d(pong->ballVel, esp_random() % 90);
	*/
}

static void setBPM(uint8_t bpm)
{
    // Figure out how many microseconds are in one beat
    ddr->bpm = bpm;
    ddr->usPerBeat = (60 * 1000000) / ddr->bpm;

    //Set restart timer for 4 beats
    //ddr->restartTimerUs = ddr->usPerBeat * 4;
}