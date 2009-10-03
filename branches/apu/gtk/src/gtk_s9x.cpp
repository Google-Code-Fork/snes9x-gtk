#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "gtk_s9x.h"
#include "gtk_control.h"
#include "gtk_sound.h"
#include "gtk_display.h"

#ifdef NETPLAY_SUPPORT
#include "gtk_netplay.h"
#endif

#define IDLE_FUNC_PRIORITY (G_PRIORITY_DEFAULT_IDLE)

void S9xPostRomInit (void);
void S9xSyncSpeedFinish (void);
static gboolean S9xIdleFunc (gpointer data);
static gboolean S9xScreenSaverCheckFunc (gpointer data);

Snes9xWindow          *top_level;
Snes9xConfig          *gui_config;
static struct timeval next_frame_time = { 0, 0 };
static struct timeval now;
static int            needs_fullscreening = FALSE;
int                   syncing;
guint                 idle_func_id;

void
S9xTerm (int signal)
{
    S9xExit ();

    return;
}

int
main (int argc, char *argv[])
{
    struct sigaction sig_callback;

    g_thread_init (NULL);
    gdk_threads_init ();

    gtk_init (&argc, &argv);
    glade_init ();

    bindtextdomain (GETTEXT_PACKAGE, SNES9XLOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    ZeroMemory (&Settings, sizeof (Settings));

    /* Allow original config file for backend settings */
    S9xLoadConfigFiles (argv, argc);

    /* Perform our config here */
    gui_config = new Snes9xConfig ();

    S9xInitInputDevices ();

    gui_config->load_config_file ();

    char *rom_filename = S9xParseArgs (argv, argc);

    S9xReportControllers ();

    if (!Memory.Init () || !S9xInitAPU ())
        exit (3);

    top_level = new Snes9xWindow (gui_config);
    top_level->show ();

    S9xInitDisplay (argc, argv);

    Memory.PostRomInitFunc = S9xPostRomInit;

    S9xPortSoundInit ();

    gui_config->reconfigure ();
    top_level->update_accels ();

    Settings.Paused = TRUE;
    syncing = 0;
    idle_func_id = g_idle_add_full (IDLE_FUNC_PRIORITY,
                                    S9xIdleFunc,
                                    NULL,
                                    NULL);

    g_timeout_add (10000, S9xScreenSaverCheckFunc, NULL);

    S9xNoROMLoaded ();

    if (rom_filename)
    {
        top_level->try_open_rom (rom_filename);
    }

    memset (&sig_callback, 0, sizeof (struct sigaction));
    sig_callback.sa_handler = S9xTerm;

    sigaction (15 /* SIGTERM */, &sig_callback, NULL);
    sigaction (3  /* SIGQUIT */, &sig_callback, NULL);
    sigaction (2  /* SIGINT  */, &sig_callback, NULL);

    if (gui_config->fullscreen)
    {
        gui_config->fullscreen = 0;
        needs_fullscreening = 1;
    }

#ifdef USE_JOYSTICK
    gui_config->flush_joysticks ();
#endif

    gtk_main ();

    return 0;
}

int
S9xOpenROM (const char *rom_filename)
{
    uint32 flags;
    bool8  loaded;

    if (gui_config->rom_loaded)
    {
        S9xAutoSaveSRAM ();
    }

#ifdef NETPLAY_SUPPORT
    S9xNetplayDisconnect ();
#endif

    flags = CPU.Flags;

    loaded = FALSE;

    if (Settings.Multi)
        loaded = Memory.LoadMultiCart (Settings.CartAName, Settings.CartBName);
    else if (rom_filename)
        loaded = Memory.LoadROM (rom_filename);

    Settings.StopEmulation = !loaded;

    if (!loaded && rom_filename)
    {
        char dir [_MAX_DIR + 1];
        char drive [_MAX_DRIVE + 1];
        char name [_MAX_FNAME + 1];
        char ext [_MAX_EXT + 1];
        char fname [_MAX_PATH + 1];

        _splitpath (rom_filename, drive, dir, name, ext);
        _makepath (fname, drive, dir, name, ext);

        strcpy (fname, S9xGetDirectory (ROM_DIR));
        strcat (fname, SLASH_STR);
        strcat (fname, name);

        if (ext [0])
        {
            strcat (fname, ".");
            strcat (fname, ext);
        }

        _splitpath (fname, drive, dir, name, ext);
        _makepath (fname, drive, dir, name, ext);

        if ((Settings.StopEmulation = !Memory.LoadROM (fname)))
        {
            fprintf (stderr, _("Error opening: %s\n"), rom_filename);

            return -1;
        }

        loaded = TRUE;
    }

    if (loaded)
    {
        Memory.LoadSRAM (S9xGetFilename (".srm", SRAM_DIR));
        S9xLoadCheatFile (S9xGetFilename (".cht", CHEAT_DIR));
    }
    else
    {
        S9xReset ();
        Settings.Paused = 1;
    }

    CPU.Flags = flags;

    S9xROMLoaded ();

    return 0;
}

void
S9xROMLoaded (void)
{
    gui_config->rom_loaded = TRUE;
    top_level->configure_widgets ();

    top_level->last_width = top_level->last_height = SIZE_FLAG_DIRTY;

    if (gui_config->full_screen_on_open)
    {
        Settings.Paused = FALSE;
        top_level->enter_fullscreen_mode ();
    }

    S9xSoundStart ();

    return;
}

void
S9xNoROMLoaded (void)
{
    S9xSoundStop ();
    gui_config->rom_loaded = FALSE;
    top_level->configure_widgets ();

    return;
}

gboolean
S9xSleepFunc (gpointer data)
{
    idle_func_id = g_idle_add_full (IDLE_FUNC_PRIORITY,
                                    S9xIdleFunc,
                                    NULL,
                                    NULL);

    return FALSE;
}

gboolean
S9xPauseFunc (gpointer data)
{
    S9xProcessEvents (TRUE);

    if (!gui_config->rom_loaded)
        return TRUE;

#ifdef NETPLAY_SUPPORT
    if (!S9xNetplayPush ())
    {
        S9xNetplayPop ();
    }
#endif

    if (!Settings.Paused) /* Coming out of pause */
    {
#ifdef USE_JOYSTICK
        /* Clear joystick queues */
        gui_config->flush_joysticks ();
#endif

        S9xSetSoundMute (FALSE);
        S9xSoundStart ();

#ifdef NETPLAY_SUPPORT
        if (Settings.NetPlay && NetPlay.Connected)
        {
            S9xNPSendPause (FALSE);
        }
#endif

        /* Resume high-performance callback */
        idle_func_id = g_idle_add_full (IDLE_FUNC_PRIORITY,
                                        S9xIdleFunc,
                                        NULL,
                                        NULL);
        top_level->update_statusbar ();
        return FALSE;
    }

    return TRUE;
}

gboolean
S9xIdleFunc (gpointer data)
{
    if (needs_fullscreening)
    {
        top_level->enter_fullscreen_mode();
        needs_fullscreening = FALSE;
    }

    if (Settings.Paused)
    {
        S9xSetSoundMute (TRUE);
        S9xSoundStop ();

#ifdef USE_JOYSTICK
        gui_config->flush_joysticks ();
#endif

#ifdef NETPLAY_SUPPORT
        if (Settings.NetPlay && NetPlay.Connected)
        {
            S9xNPSendPause (TRUE);
        }
#endif

        /* Move to a timer-based function to use less CPU */
        g_timeout_add (100, S9xPauseFunc, NULL);
        top_level->update_statusbar ();
        return FALSE;
    }

    if (syncing)
        S9xSyncSpeedFinish ();

    S9xProcessEvents (TRUE);

#ifdef NETPLAY_SUPPORT
    if (!S9xNetplayPush ())
    {
#endif
    S9xMainLoop ();
    S9xMixSound ();

#ifdef NETPLAY_SUPPORT
        S9xNetplayPop ();
    }
#endif

    return TRUE;
}

gboolean
S9xScreenSaverCheckFunc (gpointer data)
{

    if (!Settings.Paused &&
        (gui_config->screensaver_needs_reset ||
         gui_config->prevent_screensaver))
        top_level->reset_screensaver ();

    return TRUE;
}

/* Snes9x core hooks */
void
S9xMessage (int type, int number, const char *message)
{
    /*
    fprintf (stderr, "%s\n", message);
     */
    return;
}

/* Varies from ParseArgs because this one is for the OS port to handle */
void
S9xParseArg (char **argv, int &i, int argc)
{
    if (!strcasecmp (argv[i], "-filter"))
    {
        if ((++i) < argc)
        {
            if (!strcasecmp (argv[i], "none"))
            {
                gui_config->scale_method = FILTER_NONE;
            }
            else if (!strcasecmp (argv[i], "supereagle"))
            {
                gui_config->scale_method = FILTER_SUPEREAGLE;
            }
            else if (!strcasecmp (argv[i], "2xsai"))
            {
                gui_config->scale_method = FILTER_2XSAI;
            }
            else if (!strcasecmp (argv[i], "super2xsai"))
            {
                gui_config->scale_method = FILTER_SUPER2XSAI;
            }
            else if (!strcasecmp (argv[i], "hq2x"))
            {
                gui_config->scale_method = FILTER_HQ2X;
            }
            else if (!strcasecmp (argv[i], "hq3x"))
            {
                gui_config->scale_method = FILTER_HQ3X;
            }
            else if (!strcasecmp (argv[i], "hq4x"))
            {
                gui_config->scale_method = FILTER_HQ4X;
            }
            else if (!strcasecmp (argv[i], "epx"))
            {
                gui_config->scale_method = FILTER_EPX;
            }
            else if (!strcasecmp (argv[i], "ntsc"))
            {
                gui_config->scale_method = FILTER_NTSC;
            }
            else
            {
                gui_config->scale_method = FILTER_NONE;
            }
        }
    }
    else if (!strcasecmp (argv[i], "-mutesound"))
    {
        gui_config->mute_sound = TRUE;
    }

    return;
}

/* Finishes syncing by using more accurate system sleep functions*/
void
S9xSyncSpeedFinish (void)
{
    if (!syncing)
        return;

    if (Settings.SoundSync)
    {
        while (!S9xSyncSound ())
        {
            usleep (10);
        }
        return;
    }

    gettimeofday (&now, NULL);

    while (timercmp (&next_frame_time, &now, >))
    {
        unsigned time_left = (next_frame_time.tv_sec  - now.tv_sec) * 1000000 +
                              next_frame_time.tv_usec - now.tv_usec;

        if (time_left > 500000)
        {
            next_frame_time = now;
            break;
        }

        usleep (time_left);

        gettimeofday (&now, NULL);
    }

    next_frame_time.tv_usec += Settings.FrameTime;

    if (next_frame_time.tv_usec >= 1000000)
    {
        next_frame_time.tv_sec += next_frame_time.tv_usec / 1000000;
        next_frame_time.tv_usec %= 1000000;
    }

    syncing = 0;

    return;
}

/* SyncSpeed Handles delays between frames, similar to unix.cpp version,
 * cleaned up for clarity, adjusted for GUI event loop */
void
S9xSyncSpeed (void)
{
    unsigned int limit;
    int          lag;

#ifdef NETPLAY_SUPPORT
    if (S9xNetplaySyncSpeed ())
        return;
#endif

    if (Settings.HighSpeedSeek > 0)
    {
        Settings.HighSpeedSeek--;
        IPPU.RenderThisFrame = FALSE;
        IPPU.SkippedFrames = 0;

        gettimeofday (&now, NULL);
        next_frame_time = now;

        syncing = 0;

        return;
    }

    else if (Settings.TurboMode)
    {
        if ((++IPPU.FrameSkip >= Settings.TurboSkipFrames || Settings.OldTurbo)
            && !Settings.HighSpeedSeek)
        {
            IPPU.FrameSkip = 0;
            IPPU.SkippedFrames = 0;
            IPPU.RenderThisFrame = TRUE;
        }
        else
        {
            IPPU.SkippedFrames++;
            IPPU.RenderThisFrame = FALSE;
        }

        return;
    }

    gettimeofday (&now, NULL);

    if (next_frame_time.tv_sec == 0)
    {
        next_frame_time = now;
        ++next_frame_time.tv_usec;
    }

    if (Settings.SkipFrames == AUTO_FRAMERATE && !Settings.SoundSync)
    {
        lag = (now.tv_sec  - next_frame_time.tv_sec) * 1000000 +
               now.tv_usec - next_frame_time.tv_usec;

        /* We compensate for the frame time by a frame in case it's just a CPU
         * discrepancy. We can recover lost time in the next frame anyway. */
        if (lag > (int) (Settings.FrameTime))
        {
            if (lag > (int) Settings.FrameTime * 10)
            {
                /* Running way too slowly */
                next_frame_time = now;
                IPPU.RenderThisFrame = 1;
                IPPU.SkippedFrames = 0;
            }

            IPPU.RenderThisFrame = 0;
            IPPU.SkippedFrames++;
        }

        else
        {
            IPPU.RenderThisFrame = 1;
            IPPU.SkippedFrames = 0;
        }
    }
    else
    {
        limit = Settings.SoundSync ? 1 : Settings.SkipFrames + 1;

        IPPU.SkippedFrames++;
        IPPU.RenderThisFrame = 0;

        if (IPPU.SkippedFrames >= limit)
        {
            IPPU.RenderThisFrame = 1;
            IPPU.SkippedFrames = 0;
        }
    }

    syncing = 1;

    return;
}

/* Final exit point, issues exit (0) */
void
S9xExit (void)
{
    gui_config->save_config_file ();

    top_level->leave_fullscreen_mode ();

    S9xPortSoundDeinit ();

    Settings.StopEmulation = TRUE;

    if (gui_config->rom_loaded)
    {
        S9xAutoSaveSRAM ();
    }

    S9xDeinitAPU ();

    S9xDeinitInputDevices ();
    S9xDeinitDisplay ();

    gtk_main_quit ();

    delete top_level;
    delete gui_config;

    exit (0);

    return;
}

void
S9xPostRomInit (void)
{
    //First plug in both, they'll change later as needed
    S9xSetController (0, CTL_JOYPAD,     0, 0, 0, 0);
    S9xSetController (1, CTL_JOYPAD,     1, 0, 0, 0);
    top_level->set_menu_item_selected ("joypad1");
    top_level->set_menu_item_selected ("joypad2");

    if (!strncmp ((const char *) Memory.NSRTHeader + 24, "NSRT", 4))
    {
        switch (Memory.NSRTHeader[29])
        {
            case 0: //Everything goes
                break;

            case 0x10: //Mouse in Port 0
                S9xSetController (0, CTL_MOUSE,      0, 0, 0, 0);
                top_level->set_menu_item_selected ("mouse1");
                break;

            case 0x01: //Mouse in Port 1
                S9xSetController (1, CTL_MOUSE,      1, 0, 0, 0);
                top_level->set_menu_item_selected ("mouse2");
                break;

            case 0x03: //Super Scope in Port 1
                S9xSetController (1, CTL_SUPERSCOPE, 0, 0, 0, 0);
                top_level->set_menu_item_selected ("superscope1");
                break;

            case 0x06: //Multitap in Port 1
                S9xSetController (1, CTL_MP5,        1, 2, 3, 4);
                top_level->set_menu_item_selected ("multitap1");
                break;

            case 0x66: //Multitap in Ports 0 and 1
                S9xSetController (0, CTL_MP5,        0, 1, 2, 3);
                S9xSetController (1, CTL_MP5,        4, 5, 6, 7);
                top_level->set_menu_item_selected ("multitap1");
                top_level->set_menu_item_selected ("multitap2");
                break;

            case 0x08: //Multitap in Port 1, Mouse in new Port 1
                S9xSetController (1, CTL_MOUSE,      1, 0, 0, 0);
                //There should be a toggle here for putting in Multitap instead
                top_level->set_menu_item_selected ("mouse2");
                break;

            case 0x04: //Pad or Super Scope in Port 1
                S9xSetController (1, CTL_SUPERSCOPE, 0, 0, 0, 0);
                top_level->set_menu_item_selected ("superscope2");
                //There should be a toggle here for putting in a pad instead
                break;

            case 0x05: //Justifier - Must ask user...
                S9xSetController (1, CTL_JUSTIFIER,  1, 0, 0, 0);
                //There should be a toggle here for how many justifiers
                break;

            case 0x20: //Pad or Mouse in Port 0
                S9xSetController (0, CTL_MOUSE,      0, 0, 0, 0);
                //There should be a toggle here for putting in a pad instead
                break;

            case 0x22: //Pad or Mouse in Port 0 & 1
                S9xSetController (0, CTL_MOUSE,      0, 0, 0, 0);
                S9xSetController (1, CTL_MOUSE,      1, 0, 0, 0);
                //There should be a toggles here for putting in pads instead
                break;

            case 0x24: //Pad or Mouse in Port 0, Pad or Super Scope in Port 1
                //There should be a toggles here for what to put in, I'm leaving it at gamepad for now
                break;

            case 0x27: //Pad or Mouse in Port 0, Pad or Mouse or Super Scope in Port 1
                //There should be a toggles here for what to put in, I'm leaving it at gamepad for now
                break;

                //Not Supported yet
            case 0x99: break; //Lasabirdie
            case 0x0A: break; //Barcode Battler
        }
    }

    return;
}

const char *
S9xStringInput(const char *message)
{
    return NULL;
}

void
S9xExtraUsage (void)
{
    printf ("GTK port options:\n"
            "-filter [option]               Use a filter to scale the image.\n"
            "                               [option] is one of: none supereagle 2xsai\n"
            "                               super2xsai hq2x hq3x hq4x epx ntsc\n"
            "\n"
            "-mutesound                     Disables sound output.\n");
    return;
}
