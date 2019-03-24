#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <SDL.h>

#include "port.h"
#include "r3000a.h"
#include "plugins.h"
#include "cdrom.h"
#include "cdriso.h"
#include "cdrom_hacks.h"

/* PATH_MAX inclusion */
#ifdef __MINGW32__
  #include <limits.h>
#endif

#ifdef SPU_PCSXREARMED
  #include "spu/spu_pcsxrearmed/spu_config.h"		// To set spu-specific configuration
#endif

// New gpulib from Notaz's PCSX Rearmed handles duties common to GPU plugins
#ifdef USE_GPULIB
  #include "gpu/gpulib/gpu.h"
#endif

#ifdef GPU_UNAI
  #include "gpu/gpu_unai/gpu.h"
#endif

#define timer_delay(a)	wait_ticks(a*1000)

enum
{
  KEY_UP=0x1,	KEY_LEFT=0x4,		KEY_DOWN=0x10,	KEY_RIGHT=0x40,
  KEY_START=1<<8,	KEY_SELECT=1<<9,	KEY_L=1<<10,	KEY_R=1<<11,
  KEY_A=1<<12,	KEY_B=1<<13,		KEY_X=1<<14,	KEY_Y=1<<15,
};
#define _KEY_BACK   (KEY_SELECT|KEY_B)
extern char sstatesdir[PATH_MAX];
static int saveslot = -1;
static uint16_t* sshot_img; // Ptr to active image in savestate menu
static int sshot_img_num;   // Which slot above image is loaded for

static u32 ret = 0;

static inline void key_reset()
{
  ret = 0;
}

static unsigned int key_read(void)
{
  SDL_Event event;

  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
          case SDLK_UP:
            ret |= KEY_UP;
            break;
          case SDLK_DOWN:
            ret |= KEY_DOWN;
            break;
          case SDLK_LEFT:
            ret |= KEY_LEFT;
            break;
          case SDLK_RIGHT:
            ret |= KEY_RIGHT;
            break;

          case SDLK_LCTRL:
            ret |= KEY_A;
            break;
          case SDLK_LALT:
            ret |= KEY_B;
            break;
          case SDLK_SPACE:
            ret |= KEY_X;
            break;
          case SDLK_LSHIFT:
            ret |= KEY_Y;
            break;

          case SDLK_TAB:
            ret |= KEY_L;
            break;
          case SDLK_BACKSPACE:
            ret |= KEY_R;
            break;

          case SDLK_RETURN:
            ret |= KEY_START;
            break;
          case SDLK_ESCAPE:
            ret |= KEY_SELECT;
            break;
          case SDLK_m:
            ret |= KEY_SELECT | KEY_Y;
            break;

          default:
            break;
        }
        break;
      case SDL_KEYUP:
        switch(event.key.keysym.sym)
        {
          case SDLK_UP:
            ret &= ~KEY_UP;
            break;
          case SDLK_DOWN:
            ret &= ~KEY_DOWN;
            break;
          case SDLK_LEFT:
            ret &= ~KEY_LEFT;
            break;
          case SDLK_RIGHT:
            ret &= ~KEY_RIGHT;
            break;

          case SDLK_LCTRL:
            ret &= ~KEY_A;
            break;
          case SDLK_LALT:
            ret &= ~KEY_B;
            break;
          case SDLK_SPACE:
            ret &= ~KEY_X;
            break;
          case SDLK_LSHIFT:
            ret &= ~KEY_Y;
            break;

          case SDLK_TAB:
            ret &= ~KEY_L;
            break;
          case SDLK_BACKSPACE:
            ret &= ~KEY_R;
            break;

          case SDLK_RETURN:
            ret &= ~KEY_START;
            break;
          case SDLK_ESCAPE:
            ret &= ~KEY_SELECT;
            break;
          case SDLK_m:
            ret &= ~(KEY_SELECT | KEY_Y);
            break;

          default:
            break;
        }
        break;
      default:
        break;
    }
  }


  return ret;
}

struct dir_item
{
  char	*name;
  s32	type; // 0=dir, 1=file, 2=zip archive
};

int compare_names(struct dir_item *a, struct dir_item *b)
{
  uint8_t aIsParent = strcmp(a->name, "..") == 0;
  uint8_t bIsParent = strcmp(b->name, "..") == 0;

  if (aIsParent && bIsParent)
    return 0;
  else if (aIsParent)
    return -1;
  else if (bIsParent)
    return 1;

  if ((a->type != b->type) && (a->type == 0 || b->type == 0))
  {
    return a->type == 0 ? -1 : 1;
  }

  return strcasecmp(a->name, b->name);
}

void sort_dir(struct dir_item *list, int num_items)
{
  qsort((void *)list,num_items,sizeof(struct dir_item),(int (*)(const void*, const void*))compare_names);
}

static char gamepath[PATH_MAX] = "./";
static struct dir_item filereq_dir_items[1024] = { { 0, 0 }, };

#define MENU_X		8
#define MENU_Y		8
#define MENU_LS		(MENU_Y + 10)
#define MENU_HEIGHT	22

static inline void ChDir(char *dir)
{
  int unused = chdir(dir);
  (void)unused;
}

static inline char *GetCwd(void)
{
  char *unused = getcwd(gamepath, PATH_MAX);
  (void)unused;
#ifdef __WIN32__
  for (int i = 0; i < PATH_MAX; i++)
  {
    if (gamepath[i] == 0)
      break;
    if (gamepath[i] == '\\')
      gamepath[i] = '/';
  }
#endif

  return gamepath;
}

#define FREE_LIST() \
  do { \
    for (int i = 0; i < num_items; i++) \
      if (filereq_dir_items[i].name) { \
        free(filereq_dir_items[i].name); \
        filereq_dir_items[i].name = NULL; \
      } \
    num_items = 0; \
  } while (0)

static const char *wildcards[] =
{
  //senquack - we do not (yet) support these 3 PocketISO compressed formats
  // TODO: adapt PCSX Rearmed's cdrcimg.c plugin to get these
  //"z", "bz", "znx",

  "bin", "img", "mdf", "iso", "cue",
  "pbp", "cbn", NULL
};

static s32 check_ext(const char *name)
{
  const char *p = strrchr(name, '.');

  if (!p)
    return 0;

  for (int i = 0; wildcards[i] != NULL; i++)
  {
    if (strcasecmp(wildcards[i], p + 1) == 0)
      return 1;
  }

  return 0;
}

static s32 get_entry_type(char *cwd, char *d_name)
{
  s32 type;
  struct stat item;
  char *path = (char *)malloc(strlen(cwd) + strlen(d_name) + 2);

  sprintf(path, "%s/%s", cwd, d_name);
  if (!stat(path, &item))
  {
    if (S_ISDIR(item.st_mode))
    {
      type = 0;
    }
    else
    {
      type = 1;
    }
  }
  else
  {
    type = 1;
  }

  free(path);
  return type;
}

char *FileReq(char *dir, const char *ext, char *result)
{
  static char *cwd = NULL;
  static s32 cursor_pos = 1;
  static s32 first_visible;
  static s32 num_items = 0;
  DIR *dirstream;
  struct dirent *direntry;
  static s32 row;
  char tmp_string[41];
  u32 keys;

  if (dir)
    ChDir(dir);

  cwd = GetCwd();

  for (;;)
  {
    keys = key_read();

    video_clear();

    if (keys & _KEY_BACK)
    {
      FREE_LIST();
      key_reset();
      return NULL;
    }

    if (num_items == 0)
    {
      dirstream = opendir(cwd);
      if (dirstream == NULL)
      {
        port_printf(0, 20, "error opening directory");
        return NULL;
      }
      // read directory entries
      while ((direntry = readdir(dirstream)))
      {
        s32 type = get_entry_type(cwd, direntry->d_name);

        // this is a very ugly way of only accepting a certain extension
        if ((type == 0 && strcmp(direntry->d_name, ".")) ||
            check_ext(direntry->d_name) ||
            (ext && (strlen(direntry->d_name) > 4 &&0 == strncasecmp(direntry->d_name + (strlen(direntry->d_name) - strlen(ext)), ext, strlen(ext)))))
        {
          // Hide ".." if at Unix root dir. Don't display Unix hidden files (.file).
          if ((!strcmp(direntry->d_name, "..") && strcmp(cwd, "/")) || direntry->d_name[0] != '.')
          {
            filereq_dir_items[num_items].name = (char *)malloc(strlen(direntry->d_name) + 1);
            strcpy(filereq_dir_items[num_items].name, direntry->d_name);
            filereq_dir_items[num_items].type = type;
            num_items++;
            if (num_items > 1024) break;
          }
        }
      }
      closedir(dirstream);

      sort_dir(filereq_dir_items, num_items);
      cursor_pos = 0;
      first_visible = 0;
    }

    // display current directory
    int len = strlen(cwd);

    if (len > 40)
    {
      strcpy(tmp_string, "..");
      strcat(tmp_string, cwd + len - 38);
      port_printf(0, MENU_Y, tmp_string);
    }
    else
      port_printf(0, MENU_Y, cwd);

    if (keys & KEY_DOWN)   //down
    {
      if (++cursor_pos >= num_items)
      {
        cursor_pos = 0;
        first_visible = 0;
      }
      if ((cursor_pos - first_visible) >= MENU_HEIGHT) first_visible++;
    }
    else if (keys & KEY_UP)     // up
    {
      if (--cursor_pos < 0)
      {
        cursor_pos = num_items - 1;
        first_visible = cursor_pos - MENU_HEIGHT + 1;
        if (first_visible < 0) first_visible = 0;
      }
      if (cursor_pos < first_visible) first_visible--;
    }
    else if (keys & KEY_LEFT)     //left
    {
      if (cursor_pos >= 10) cursor_pos -= 10;
      else cursor_pos = 0;
      if (cursor_pos < first_visible) first_visible = cursor_pos;
    }
    else if (keys & KEY_RIGHT)     //right
    {
      if (cursor_pos < (num_items - 11)) cursor_pos += 10;
      else cursor_pos = num_items - 1;
      if ((cursor_pos - first_visible) >= MENU_HEIGHT)
        first_visible = cursor_pos - (MENU_HEIGHT - 1);
    }
    else if (keys & KEY_A)     // button 1
    {
      // directory selected
      if (filereq_dir_items[cursor_pos].type == 0)
      {
        strcat(cwd, "/");
        strcat(cwd, filereq_dir_items[cursor_pos].name);

        ChDir(cwd);
        cwd = GetCwd();

        FREE_LIST();
        key_reset();
      }
      else
      {
        sprintf(result, "%s/%s", cwd, filereq_dir_items[cursor_pos].name);
        if (dir)
          strcpy(dir, cwd);

        video_clear();
        port_printf(16 * 8, 120, "LOADING");
        video_flip();

        FREE_LIST();
        key_reset();
        return result;
      }
    }
    else if (keys & KEY_B)
    {
      cursor_pos = 0;
      first_visible = 0;
      key_reset();
    }

    // display directory contents
    row = 0;
    while (row < num_items && row < MENU_HEIGHT)
    {
      if (row == (cursor_pos - first_visible))
      {
        // draw cursor
        port_printf(MENU_X + 16, MENU_LS + (10 * row), "-->");
      }

      if (filereq_dir_items[row + first_visible].type == 0)
        port_printf(MENU_X, MENU_LS + (10 * row), "DIR");
      int len = strlen(filereq_dir_items[row + first_visible].name);
      if (len > 32)
      {
        snprintf(tmp_string, 16, "%s", filereq_dir_items[row + first_visible].name);
        strcat(tmp_string, "..");
        strcat(tmp_string, &filereq_dir_items[row + first_visible].name[len - 15]);
      }
      else
        snprintf(tmp_string, 33, "%s", filereq_dir_items[row + first_visible].name);
      port_printf(MENU_X + (8 * 5), MENU_LS + (10 * row), tmp_string);
      row++;
    }
    while (row < MENU_HEIGHT)
      row++;

    video_flip();
    timer_delay(75);

    if (keys & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_L | KEY_R |
                KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))
      timer_delay(50);
  }

  return NULL;
}

typedef struct
{
  char *name;
  int (*on_press_a)();
  int (*on_press)(u32 keys);
  char *(*showval)();
  void (*hint)();
} MENUITEM;

typedef struct
{
  int num;
  int cur;
  int x, y;
  MENUITEM *m; // array of items
} MENU;

/* Forward declaration */
static int gui_RunMenu(MENU *menu);
static int gui_LoadIso();
static int gui_Settings();
static int gui_GPUSettings();
static int gui_SPUSettings();
static int gui_Quit();

static int gui_Credits()
{
  for (;;)
  {
    u32 keys = key_read();

    video_clear();

    // check keys
    if (keys)
    {
      key_reset();
      return 0;
    }

    // diplay menu
    port_printf(15 * 8 + 4, 10, "CREDITS:");
    port_printf( 2 * 8, 30, "pcsx team, pcsx-df team, pcsx-r team");

    port_printf( 6 * 8, 50, "Franxis and Chui - PCSX4ALL");
    port_printf( 4 * 8, 60, "Unai - fast PCSX4ALL GPU plugin");

    port_printf( 5 * 8, 80, "Ulrich Hecht - psx4all-dingoo");

    port_printf(10 * 8, 90, "notaz - PCSX-ReArmed");

    port_printf( 0 * 8, 110, "Dmitry Smagin - porting and optimizing");
    port_printf( 0 * 8, 120, "                of mips recompiler,");
    port_printf( 0 * 8, 130, "                gui coding");

    port_printf( 0 * 8, 150, "senquack - fixing polygons in gpu_unai,");
    port_printf( 0 * 8, 160, "           porting spu and other stuff");
    port_printf( 0 * 8, 170, "           from pcsx_rearmed and pcsx-r,");
    port_printf( 0 * 8, 180, "           many fixes and improvements");

    port_printf( 0 * 8, 195, "JohnnyonFlame   - gpu_unai dithering");
    port_printf( 0 * 8, 205, "                  and other fixes");

    port_printf( 0 * 8, 220, "zear         - gui fixing and testing");

    video_flip();
    timer_delay(75);
  }

  return 0;
}

static MENUITEM gui_MainMenuItems[] =
{
  {(char *)"Load game", &gui_LoadIso, NULL, NULL, NULL},
  {(char *)"Core settings", &gui_Settings, NULL, NULL, NULL},
  {(char *)"GPU settings", &gui_GPUSettings, NULL, NULL, NULL},
  {(char *)"SPU settings", &gui_SPUSettings, NULL, NULL, NULL},
  {(char *)"Credits", &gui_Credits, NULL, NULL, NULL},
  {(char *)"Quit", &gui_Quit, NULL, NULL, NULL},
  {0}
};

#define MENU_SIZE ((sizeof(gui_MainMenuItems) / sizeof(MENUITEM)) - 1)
static MENU gui_MainMenu = { MENU_SIZE, 0, 112, 140, (MENUITEM *)&gui_MainMenuItems };

static int gui_state_save(int slot)
{
  if (sshot_img)
  {
    free(sshot_img);
    sshot_img = NULL;
  }

  // Remember which saveslot was accessed last
  saveslot = slot;

  video_clear();
  port_printf(160-(6*8/2), 120-(8/2), "SAVING");
  video_flip();

  if (state_save(slot) < 0)
  {
    // Error saving

    for (;;)
    {
      u32 keys = key_read();
      video_clear();
      // check keys
      if (keys)
      {
        key_reset();
        return 0;
      }

      port_printf(160-(11*8/2), 120-12, "SAVE FAILED");
      port_printf(160-(18*8/2), 120+12, "Out of disk space?");
      video_flip();
      timer_delay(75);
    }
  }

  // Return -1 to gui_StateSave() caller menu, so it knows
  //  to tell main menu to return back to main menu.
  return -1;
}

#define GUI_STATE_SAVE(slot) \
  static int gui_state_save##slot() \
  { \
    return gui_state_save(slot); \
  }

GUI_STATE_SAVE(0)
GUI_STATE_SAVE(1)
GUI_STATE_SAVE(2)
GUI_STATE_SAVE(3)
GUI_STATE_SAVE(4)
GUI_STATE_SAVE(5)
GUI_STATE_SAVE(6)
GUI_STATE_SAVE(7)
GUI_STATE_SAVE(8)
GUI_STATE_SAVE(9)

static void gui_state_save_hint(int slot)
{
  int x, y, len;
  char filename[128];
  char fullpath[256+PATH_MAX];
  snprintf(filename, sizeof(filename), "%s.%d.sav", CdromId, slot);
  snprintf(fullpath, sizeof(fullpath), "%s/%s", sstatesdir, filename);

  // If file doesn't exist, there's no info to display
  if (!FileExists(fullpath))
    return;

  int checkstate = CheckState(fullpath, NULL, 1, NULL);

  // If file doesn't exist or has header/version mismatch, abort.
  // If CheckState() indicates file is ok except lacking screenshot,
  //  i.e. an old version number , we'll display a info msg for that.
  if (checkstate < 0 && checkstate != CHECKSTATE_ERR_NO_SSHOT)
    return;

  // Allocate/get 160x120 rgb565 screenshot image data
  if (checkstate == CHECKSTATE_SUCCESS)
  {
    if (!sshot_img || sshot_img_num != slot)
    {
      free(sshot_img);
      sshot_img = (uint16_t*)malloc(160*120*2);
      if (sshot_img)
      {
        // Fetch the image data
        CheckState(fullpath, NULL, 1, sshot_img);
        sshot_img_num = slot;
      }
      else
      {
        printf("Warning: malloc failed for sshot image in %s\n", __func__);
      }
    }
  }
  else
  {
    // Savestate is too old to have embedded sshot, or read error
    free(sshot_img);
    sshot_img = NULL;
  }

  // Display screenshot image
  if (sshot_img)
  {
    x = 160-8;
    y = 70;
    int dst_stride = 320;
    uint16_t *dst = (uint16_t*)SCREEN + y * dst_stride + x;
    for (int j=0; j < 120; ++j)
    {
      memcpy((void*)dst, (void*)(sshot_img + j*160), 160*2);
      dst += dst_stride;
    }
  }
  else
  {
    port_printf(320-135, 125 - 10, "No screenshot");
    port_printf(320-135, 125,      "  available  ");
  }

  // Display date of last modification
  char date_str[128];
  date_str[0] = '\0';
  if (FileDate(fullpath, date_str, NULL) >= 0)
  {
    len = strlen(date_str);
    x = 320 - 8 - len * 8;
    y = 70 - 22;
    port_printf(x, y, date_str);
  }

  // Display name of save file
  len = strlen(filename);
  if (len > 25)
    filename[25] = '\0';
  x = 320 - 8 - len * 8;
  y = 70 - 11;
  port_printf(x, y, filename);
}

#define GUI_STATE_SAVE_HINT(slot) \
  static void gui_state_save_hint##slot() \
  { \
    gui_state_save_hint(slot); \
  }

GUI_STATE_SAVE_HINT(0)
GUI_STATE_SAVE_HINT(1)
GUI_STATE_SAVE_HINT(2)
GUI_STATE_SAVE_HINT(3)
GUI_STATE_SAVE_HINT(4)
GUI_STATE_SAVE_HINT(5)
GUI_STATE_SAVE_HINT(6)
GUI_STATE_SAVE_HINT(7)
GUI_STATE_SAVE_HINT(8)
GUI_STATE_SAVE_HINT(9)

static int gui_state_save_back()
{
  if (sshot_img)
  {
    free(sshot_img);
    sshot_img = NULL;
  }

  return 1;
}

// In-game savestate save sub-menu, called from GameMenu() menu
static int gui_StateSave()
{
  const char* str_slot[10];
  const char* str_slot_unused[10] =
  {
    "Empty slot 0", "Empty slot 1", "Empty slot 2", "Empty slot 3", "Empty slot 4",
    "Empty slot 5", "Empty slot 6", "Empty slot 7", "Empty slot 8", "Empty slot 9",
  };
  const char* str_slot_used[10] =
  {
    "Used  slot 0", "Used  slot 1", "Used  slot 2", "Used  slot 3", "Used  slot 4",
    "Used  slot 5", "Used  slot 6", "Used  slot 7", "Used  slot 8", "Used  slot 9",
  };

  // Restore last accessed position
  int initial_pos = saveslot;
  if (initial_pos > 9)
    initial_pos = 9;

  for (int i=0; i < 10; ++i)
  {
    char savename[128+PATH_MAX];
    snprintf(savename, sizeof(savename), "%s/%s.%d.sav", sstatesdir, CdromId, i);
    if (FileExists(savename))
    {
      str_slot[i] = str_slot_used[i];
    }
    else
    {
      str_slot[i] = str_slot_unused[i];
      // Initial position points to a file that doesn't exist?
      if (initial_pos == i)
        initial_pos = -1;
    }
  }

  MENUITEM gui_StateSaveItems[] =
  {
    {(char *)str_slot[0], &gui_state_save0, NULL, NULL, &gui_state_save_hint0},
    {(char *)str_slot[1], &gui_state_save1, NULL, NULL, &gui_state_save_hint1},
    {(char *)str_slot[2], &gui_state_save2, NULL, NULL, &gui_state_save_hint2},
    {(char *)str_slot[3], &gui_state_save3, NULL, NULL, &gui_state_save_hint3},
    {(char *)str_slot[4], &gui_state_save4, NULL, NULL, &gui_state_save_hint4},
    {(char *)str_slot[5], &gui_state_save5, NULL, NULL, &gui_state_save_hint5},
    {(char *)str_slot[6], &gui_state_save6, NULL, NULL, &gui_state_save_hint6},
    {(char *)str_slot[7], &gui_state_save7, NULL, NULL, &gui_state_save_hint7},
    {(char *)str_slot[8], &gui_state_save8, NULL, NULL, &gui_state_save_hint8},
    {(char *)str_slot[9], &gui_state_save9, NULL, NULL, &gui_state_save_hint9},
    {NULL, NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL, NULL},
    {(char *)"Back to main menu    ", &gui_state_save_back, NULL, NULL, NULL},
    {0}
  };

  const int menu_size = (sizeof(gui_StateSaveItems) / sizeof(MENUITEM)) - 1;

  // If no files were present, select last menu entry as initial position
  if (initial_pos < 0)
    initial_pos = menu_size-1;

  MENU gui_StateSaveMenu = { menu_size, initial_pos, 30, 80, (MENUITEM *)&gui_StateSaveItems };

  int ret = gui_RunMenu(&gui_StateSaveMenu);

  // Free memory for screenshot image
  free(sshot_img);
  sshot_img = NULL;

  if (ret >= 0)
  {
    // User wants to go back to main menu
    return 0;
  }
  else
  {
    // State was saved by gui_state_save() and returned -1 to
    //  indicate main menu should return back to emu.
    return 1;
  }
}

static int gui_state_load(int slot)
{
  if (sshot_img)
  {
    free(sshot_img);
    sshot_img = NULL;
  }

  // Remember which saveslot was accessed last
  saveslot = slot;
  if (state_load(slot) < 0)
  {
    /* Load failure */
    return 0;
  }

  // Return -1 to gui_StateLoad() caller menu, so it knows
  //  to tell main menu to return back to main menu.
  return -1;
}

#define GUI_STATE_LOAD(slot) \
  static int gui_state_load##slot() \
  { \
    return gui_state_load(slot); \
  }

GUI_STATE_LOAD(0)
GUI_STATE_LOAD(1)
GUI_STATE_LOAD(2)
GUI_STATE_LOAD(3)
GUI_STATE_LOAD(4)
GUI_STATE_LOAD(5)
GUI_STATE_LOAD(6)
GUI_STATE_LOAD(7)
GUI_STATE_LOAD(8)
GUI_STATE_LOAD(9)

static void gui_state_load_hint(int slot)
{
  int x, y, len;
  char filename[128];
  char fullpath[256+PATH_MAX];
  snprintf(filename, sizeof(filename), "%s.%d.sav", CdromId, slot);
  snprintf(fullpath, sizeof(fullpath), "%s/%s", sstatesdir, filename);

  // If file doesn't exist, there's no info to display
  if (!FileExists(fullpath))
    return;

  int checkstate = CheckState(fullpath, NULL, 1, NULL);

  // If file doesn't exist or has header/version mismatch, abort.
  // If CheckState() indicates file is ok except lacking screenshot,
  //  i.e. an old version number , we'll display a info msg for that.
  if (checkstate < 0 && checkstate != CHECKSTATE_ERR_NO_SSHOT)
    return;

  // Allocate/get 160x120 rgb565 screenshot image data
  if (checkstate == CHECKSTATE_SUCCESS)
  {
    if (!sshot_img || sshot_img_num != slot)
    {
      free(sshot_img);
      sshot_img = (uint16_t*)malloc(160*120*2);
      if (sshot_img)
      {
        // Fetch the image data
        CheckState(fullpath, NULL, 1, sshot_img);
        sshot_img_num = slot;
      }
      else
      {
        printf("Warning: malloc failed for sshot image in %s\n", __func__);
      }
    }
  }
  else
  {
    // Savestate is too old to have embedded sshot, or read error
    free(sshot_img);
    sshot_img = NULL;
  }

  // Display screenshot image
  if (sshot_img)
  {
    x = 160-8;
    y = 70;
    const int dst_stride = 320;
    uint16_t *dst = (uint16_t*)SCREEN + y * dst_stride + x;
    for (int j=0; j < 120; ++j)
    {
      memcpy((void*)dst, (void*)(sshot_img + j*160), 160*2);
      dst += dst_stride;
    }
  }
  else
  {
    port_printf(320-135, 125 - 10, "No screenshot");
    port_printf(320-135, 125,      "  available  ");
  }

  // Display date of last modification
  char date_str[128];
  date_str[0] = '\0';
  if (FileDate(fullpath, date_str, NULL) >= 0)
  {
    len = strlen(date_str);
    x = 320 - 8 - len * 8;
    y = 70 - 22;
    port_printf(x, y, date_str);
  }

  // Display name of save file
  len = strlen(filename);
  if (len > 25)
    filename[25] = '\0';
  x = 320 - 8 - len * 8;
  y = 70 - 11;
  port_printf(x, y, filename);
}

#define GUI_STATE_LOAD_HINT(slot) \
  static void gui_state_load_hint##slot() \
  { \
    gui_state_load_hint(slot); \
  }

GUI_STATE_LOAD_HINT(0)
GUI_STATE_LOAD_HINT(1)
GUI_STATE_LOAD_HINT(2)
GUI_STATE_LOAD_HINT(3)
GUI_STATE_LOAD_HINT(4)
GUI_STATE_LOAD_HINT(5)
GUI_STATE_LOAD_HINT(6)
GUI_STATE_LOAD_HINT(7)
GUI_STATE_LOAD_HINT(8)
GUI_STATE_LOAD_HINT(9)

static int gui_state_load_back()
{
  if (sshot_img)
  {
    free(sshot_img);
    sshot_img = NULL;
  }

  return 1;
}

// In-game savestate load sub-menu, called from GameMenu() menu
static int gui_StateLoad()
{
  const char* str_slot[10] =
  {
    "Load slot 0", "Load slot 1", "Load slot 2", "Load slot 3", "Load slot 4",
    "Load slot 5", "Load slot 6", "Load slot 7", "Load slot 8", "Load slot 9",
  };

  // Restore last accessed position
  int initial_pos = saveslot;
  if (initial_pos > 9)
    initial_pos = 9;

  int newest_file_pos = -1;

  time_t newest_mtime = 0;
  for (int i=0; i < 10; ++i)
  {
    char savename[128+PATH_MAX];
    snprintf(savename, sizeof(savename), "%s/%s.%d.sav", sstatesdir, CdromId, i);
    if (FileExists(savename))
    {
      time_t mtime;
      if (FileDate(savename, NULL, &mtime) >= 0)
      {
        if (mtime > newest_mtime)
        {
          newest_file_pos = i;
          newest_mtime = mtime;
        }
      }
    }
    else
    {
      str_slot[i] = NULL;
      // Initial position points to a file that doesn't exist?
      if (initial_pos == i)
        initial_pos = -1;
    }
  }

  // If a save hasn't been loaded before for the current game, choose
  //  the file with the newest modification date as initial position
  if (initial_pos < 0 && newest_file_pos >= 0)
    initial_pos = newest_file_pos;

  MENUITEM gui_StateLoadItems[] =
  {
    {(char *)str_slot[0], &gui_state_load0, NULL, NULL, &gui_state_load_hint0},
    {(char *)str_slot[1], &gui_state_load1, NULL, NULL, &gui_state_load_hint1},
    {(char *)str_slot[2], &gui_state_load2, NULL, NULL, &gui_state_load_hint2},
    {(char *)str_slot[3], &gui_state_load3, NULL, NULL, &gui_state_load_hint3},
    {(char *)str_slot[4], &gui_state_load4, NULL, NULL, &gui_state_load_hint4},
    {(char *)str_slot[5], &gui_state_load5, NULL, NULL, &gui_state_load_hint5},
    {(char *)str_slot[6], &gui_state_load6, NULL, NULL, &gui_state_load_hint6},
    {(char *)str_slot[7], &gui_state_load7, NULL, NULL, &gui_state_load_hint7},
    {(char *)str_slot[8], &gui_state_load8, NULL, NULL, &gui_state_load_hint8},
    {(char *)str_slot[9], &gui_state_load9, NULL, NULL, &gui_state_load_hint9},
    {NULL, NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL, NULL},
    {(char *)"Back to main menu    ", &gui_state_load_back, NULL, NULL, NULL},
    {0}
  };

  const int menu_size = (sizeof(gui_StateLoadItems) / sizeof(MENUITEM)) - 1;

  // If no files were present, select last menu entry as initial position
  if (initial_pos < 0)
    initial_pos = menu_size-1;

  MENU gui_StateLoadMenu = { menu_size, initial_pos, 30, 80, (MENUITEM *)&gui_StateLoadItems };

  int ret = gui_RunMenu(&gui_StateLoadMenu);

  // Free memory for screenshot image
  free(sshot_img);
  sshot_img = NULL;

  if (ret >= 0)
  {
    // User wants to go back to main menu
    return 0;
  }
  else
  {
    // State was loaded by gui_state_load() and returned -1 to
    //  indicate main menu should return back to emu.
    return 1;
  }
}

//To choose which of a multi-CD image should be used. Can be called
// from front-end 'Swap CD' menu item, in which case parameter
// 'swapping_cd' is 1. Or, can be called via callback function
// gui_select_multicd_to_boot_from() inside cdriso.cpp, in which
// case swapping_cd parameter is 0.
static int gui_select_multicd(uint8_t swapping_cd)
{
  if (cdrIsoMultidiskCount <= 1)
    return 0;

  // Only max of 8 ISO images inside an Eboot multi-disk .pbp are supported
  //  by cdriso.cpp PBP code, but enforce it here to be sure:
  int num_rows = (cdrIsoMultidiskCount > 8) ? 8 : cdrIsoMultidiskCount;

  int cursor_pos = cdrIsoMultidiskSelect;
  if ((cursor_pos >= num_rows) || (cursor_pos < 0))
    cursor_pos = 0;

  for (;;)
  {
    video_clear();
    u32 keys = key_read();

    if ((swapping_cd) && (keys & _KEY_BACK))
    {
      key_reset();
      return 0;
    }

    if (!swapping_cd)
      port_printf(MENU_X, MENU_Y, "Multi-CD image detected:");

    char tmp_string[41];
    for (int row=0; row < num_rows; ++row)
    {
      if (row == cursor_pos)
      {
        // draw cursor
        port_printf(MENU_X + 16, MENU_LS + 10 + (10 * row), "-->");
      }

      sprintf(tmp_string, "CD %d", (row+1));

      if (swapping_cd && (row == cdrIsoMultidiskSelect))
      {
        // print indication of which CD is already inserted
        strcat(tmp_string, " (inserted)");
      }

      port_printf(MENU_X + (8 * 5), MENU_LS + 10 + (10 * row), tmp_string);
    }

    if (keys & KEY_DOWN)   //down
    {
      if (++cursor_pos >= num_rows)
        cursor_pos = 0;
    }
    else if (keys & KEY_UP)     // up
    {
      if (--cursor_pos < 0)
        cursor_pos = num_rows - 1;
    }
    else if (keys & KEY_LEFT)     //left
    {
      cursor_pos = 0;
    }
    else if (keys & KEY_RIGHT)     //right
    {
      cursor_pos = num_rows - 1;
    }
    else if (keys & KEY_A)     // button 1
    {
      key_reset();
      cdrIsoMultidiskSelect = cursor_pos;
      video_clear();
      video_flip();
      // Forget last used save slot
      saveslot = -1;
      return 1;
    }

    video_flip();
    timer_delay(75);

    if (keys & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_L | KEY_R |
                KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))
      timer_delay(50);
  }

}

//Called via callback when handlepbp() in cdriso.cpp detects a multi-CD
// .pbp image is being loaded, so user can choose CD to boot from.
// This is necessary because we do not know if a given CD image is
// a multi-CD image until after cdrom plugin has gone through many
// steps and verifications.
static CALLBACK void gui_select_multicd_to_boot_from(void)
{
  if (cdrIsoMultidiskSelect >= cdrIsoMultidiskCount)
    cdrIsoMultidiskSelect = 0;

  //Pass 0 to indicate a CD is not being swapped through front-end menu
  gui_select_multicd(0);

  //Before return to emu, clear/flip once more in case we're triple-buffered
  video_clear();
  video_flip();
}

static int gui_swap_cd(void)
{
  //Is a multi-cd image loaded? (EBOOT .pbp files support this)
  uint8_t using_multicd = cdrIsoMultidiskCount > 1;

  if (using_multicd)
  {
    // Pass 1 to gui_select_multicd() so it knows CD is being swapped
    if (!gui_select_multicd(1))
    {
      // User cancelled, return to menu
      return 0;
    }
    else
    {
      printf("CD swap selected image %d of %d in multi-CD\n", cdrIsoMultidiskSelect+1, cdrIsoMultidiskCount);
    }
  }
  else
  {
    static char isoname[PATH_MAX];
    const char *name = FileReq(Config.LastDir, NULL, isoname);
    if (name == NULL)
      return 0;

    SetIsoFile(name);
    printf("CD swap selected file: %s\n", name);
  }

  CdromId[0] = '\0';
  CdromLabel[0] = '\0';

  //Unregister multi-CD callback so handlepbp() or other cdriso
  // plugins don't ask for CD to boot from
  cdrIsoMultidiskCallback = NULL;

  if (ReloadCdromPlugin() < 0)
  {
    printf("Failed to re-initialize cdr\n");
    return 0;
  }

  if (CDR_open() < 0)
  {
    printf("Failed to open cdr\n");
    return 0;
  }

  SetCdOpenCaseTime(time(NULL) + 2);
  LidInterrupt();
  return 1;
}

static MENUITEM gui_GameMenuItems[] =
{
  {(char *)"Swap CD", &gui_swap_cd, NULL, NULL, NULL},
  {(char *)"Load state", &gui_StateLoad, NULL, NULL, NULL},
  {(char *)"Save state", &gui_StateSave, NULL, NULL, NULL},
  {(char *)"GPU settings", &gui_GPUSettings, NULL, NULL, NULL},
  {(char *)"SPU settings", &gui_SPUSettings, NULL, NULL, NULL},
  {(char *)"Core settings", &gui_Settings, NULL, NULL, NULL},
  {(char *)"Quit", &gui_Quit, NULL, NULL, NULL},
  {0}
};

#define GMENU_SIZE ((sizeof(gui_GameMenuItems) / sizeof(MENUITEM)) - 1)
static MENU gui_GameMenu = { GMENU_SIZE, 0, 112, 120, (MENUITEM *)&gui_GameMenuItems };

#ifdef PSXREC
static int emu_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.Cpu == 1) Config.Cpu = 0;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.Cpu == 0) Config.Cpu = 1;
  }

  return 0;
}

static char *emu_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", Config.Cpu ? "int" : "rec");
  return buf;
}

extern u32 cycle_multiplier; // in mips/recompiler.cpp

static int cycle_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (cycle_multiplier < 0x300) cycle_multiplier += 0x04;
  }
  else if (keys & KEY_LEFT)
  {
    if (cycle_multiplier > 0x050) cycle_multiplier -= 0x04;
  }

  return 0;
}

static char *cycle_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%d.%02d", cycle_multiplier >> 8, (cycle_multiplier & 0xff) * 100 / 256);
  return buf;
}
#endif

static int bios_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.HLE == 0) Config.HLE = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.HLE == 1) Config.HLE = 0;
  }

  return 0;
}

static char *bios_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", Config.HLE ? "on" : "off");
  return buf;
}

static int bios_set()
{
  static char biosname[PATH_MAX];
  const char *name = FileReq(Config.BiosDir, NULL, biosname);

  if (name)
  {
    const char *p = strrchr(name, '/');
    strcpy(Config.Bios, p + 1);
  }

  return 0;
}

static int RCntFix_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.RCntFix < 1) Config.RCntFix = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.RCntFix > 0) Config.RCntFix = 0;
  }

  return 0;
}

static char *RCntFix_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", Config.RCntFix ? "on" : "off");
  return buf;
}

static void RCntFix_hint()
{
  port_printf(2 * 8 - 4, 10 * 8, "Parasite Eve 2, Vandal Hearts 1/2 Fix");
}

static int VSyncWA_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.VSyncWA < 1) Config.VSyncWA = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.VSyncWA > 0) Config.VSyncWA = 0;
  }

  return 0;
}

static void VSyncWA_hint()
{
  port_printf(6 * 8, 10 * 8, "InuYasha Sengoku Battle Fix");
}

static char *VSyncWA_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", Config.VSyncWA ? "on" : "off");
  return buf;
}

static int Analog1_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.AnalogArrow < 1) Config.AnalogArrow = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.AnalogArrow > 0) Config.AnalogArrow = 0;
  }

  return 0;
}

static void Analog1_hint()
{
  port_printf(6 * 8, 10 * 8, "Analog Stick -> Arrow Keys");
}

static char *Analog1_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", Config.AnalogArrow ? "on" : "off");
  return buf;
}

static int settings_back()
{
  return 1;
}

static int settings_defaults()
{
  /* Restores settings to default values. */
  Config.Mdec = 0;
  Config.PsxAuto = 1;
  Config.HLE = 1;
  Config.RCntFix = 0;
  Config.VSyncWA = 0;
  Config.AnalogArrow = 0;
#ifdef PSXREC
  Config.Cpu = 0;
#else
  Config.Cpu = 1;
#endif
  Config.PsxType = 0;
#ifdef PSXREC
  cycle_multiplier = 0x200; // == 2.0
#endif
  return 0;
}

static MENUITEM gui_SettingsItems[] =
{
#ifdef PSXREC
  {(char *)"Emulation core       ", NULL, &emu_alter, &emu_show, NULL},
  {(char *)"Cycle multiplier     ", NULL, &cycle_alter, &cycle_show, NULL},
#endif
  {(char *)"HLE emulated BIOS    ", NULL, &bios_alter, &bios_show, NULL},
  {(char *)"Set BIOS file        ", &bios_set, NULL, NULL, NULL},
  {(char *)"RCntFix              ", NULL, &RCntFix_alter, &RCntFix_show, &RCntFix_hint},
  {(char *)"VSyncWA              ", NULL, &VSyncWA_alter, &VSyncWA_show, &VSyncWA_hint},
  {(char *)"Analog Arrow Keys    ", NULL, &Analog1_alter, &Analog1_show, &Analog1_hint},
  {(char *)"Restore defaults     ", &settings_defaults, NULL, NULL, NULL},
  {NULL, NULL, NULL, NULL, NULL},
  {(char *)"Back to main menu    ", &settings_back, NULL, NULL, NULL},
  {0}
};

#define SET_SIZE ((sizeof(gui_SettingsItems) / sizeof(MENUITEM)) - 1)
static MENU gui_SettingsMenu = { SET_SIZE, 0, 56, 112, (MENUITEM *)&gui_SettingsItems };

static int fps_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.ShowFps == 0) Config.ShowFps = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.ShowFps == 1) Config.ShowFps = 0;
  }
  return 0;
}

static char *fps_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", Config.ShowFps == 1 ? "on" : "off");
  return buf;
}

static int framelimit_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.FrameLimit == 0) Config.FrameLimit = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.FrameLimit == 1) Config.FrameLimit = 0;
  }

  return 0;
}

static char *framelimit_show()
{
  int idx = Config.FrameLimit ? 1 : 0;
  const char* str[] = { "off", "on" };
  return (char*)str[idx];
}

#ifdef USE_GPULIB
static int frameskip_alter(u32 keys)
{
  // Config.FrameSkip is -1 for auto-frameskip, 0 for 'frameskip off'
  //  or 1-3 for fixed frameskip. We want to have 'frameskip off' as
  //  the first setting and 'auto-frameskip' as the second setting
  //  in the gui settings, though.

  if (Config.FrameSkip < -1) Config.FrameSkip = -1;
  if (Config.FrameSkip > 3)  Config.FrameSkip = 3;

  int fs = Config.FrameSkip + 1;

  if (fs == 0) fs = 1;
  else if (fs == 1) fs = 0;

  if (keys & KEY_RIGHT)
  {
    if (fs < 4) fs++;
  }
  else if (keys & KEY_LEFT)
  {
    if (fs > 0) fs--;
  }

  if (fs == 0) fs = 1;
  else if (fs == 1) fs = 0;

  Config.FrameSkip = fs - 1;
  return 0;
}

static char *frameskip_show()
{
  const char* str[] = { "auto", "off", "1", "2", "3" };

  // Config.FrameSkip val range is -1..3
  int fs = Config.FrameSkip + 1;
  if (fs < 0) fs = 0;
  if (fs > 4) fs = 4;
  return (char*)str[fs];
}
#endif //USE_GPULIB

#ifdef GPU_UNAI
static int interlace_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (gpu_unai_config_ext.ilace_force == 0)
      gpu_unai_config_ext.ilace_force = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (gpu_unai_config_ext.ilace_force == 1)
      gpu_unai_config_ext.ilace_force = 0;
  }

  return 0;
}

static char *interlace_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", gpu_unai_config_ext.ilace_force == 1 ? "on" : "off");
  return buf;
}

static int dithering_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (gpu_unai_config_ext.dithering == 0)
      gpu_unai_config_ext.dithering = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (gpu_unai_config_ext.dithering == 1)
      gpu_unai_config_ext.dithering = 0;
  }

  return 0;
}

static char *dithering_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", gpu_unai_config_ext.dithering == 1 ? "on" : "off");
  return buf;
}

static int lighting_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (gpu_unai_config_ext.lighting == 0)
      gpu_unai_config_ext.lighting = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (gpu_unai_config_ext.lighting == 1)
      gpu_unai_config_ext.lighting = 0;
  }

  return 0;
}

static char *lighting_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", gpu_unai_config_ext.lighting == 1 ? "on" : "off");
  return buf;
}

static int fast_lighting_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (gpu_unai_config_ext.fast_lighting == 0)
      gpu_unai_config_ext.fast_lighting = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (gpu_unai_config_ext.fast_lighting == 1)
      gpu_unai_config_ext.fast_lighting = 0;
  }

  return 0;
}

static char *fast_lighting_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", gpu_unai_config_ext.fast_lighting == 1 ? "on" : "off");
  return buf;
}

static int blending_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (gpu_unai_config_ext.blending == 0)
      gpu_unai_config_ext.blending = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (gpu_unai_config_ext.blending == 1)
      gpu_unai_config_ext.blending = 0;
  }

  return 0;
}

static char *blending_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", gpu_unai_config_ext.blending == 1 ? "on" : "off");
  return buf;
}

static int clip_368_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (gpu_unai_config_ext.clip_368 == 0)
      gpu_unai_config_ext.clip_368 = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (gpu_unai_config_ext.clip_368 == 1)
      gpu_unai_config_ext.clip_368 = 0;
  }

  return 0;
}

static char *clip_368_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", gpu_unai_config_ext.clip_368 == 1 ? "on" : "off");
  return buf;
}
#endif

static int gpu_settings_defaults()
{
  Config.ShowFps = 0;
  Config.FrameLimit = 1;
  Config.FrameSkip = FRAMESKIP_OFF;

#ifdef GPU_UNAI
#ifndef USE_GPULIB
  gpu_unai_config_ext.frameskip_count = 0;
#endif
  gpu_unai_config_ext.ilace_force = 0;
  gpu_unai_config_ext.clip_368 = 0;
  gpu_unai_config_ext.lighting = 1;
  gpu_unai_config_ext.fast_lighting = 1;
  gpu_unai_config_ext.blending = 1;
  gpu_unai_config_ext.dithering = 0;
#endif

  return 0;
}

static MENUITEM gui_GPUSettingsItems[] =
{
  /* Not working with gpulib yet */
  {(char *)"Show FPS             ", NULL, &fps_alter, &fps_show, NULL},
  {(char *)"Frame limiter        ", NULL, &framelimit_alter, &framelimit_show, NULL},
#ifdef USE_GPULIB
  /* Only working with gpulib */
  {(char *)"Frame skip           ", NULL, &frameskip_alter, &frameskip_show, NULL},
#endif
#ifdef GPU_UNAI
  {(char *)"Interlace            ", NULL, &interlace_alter, &interlace_show, NULL},
  {(char *)"Dithering            ", NULL, &dithering_alter, &dithering_show, NULL},
  {(char *)"Lighting             ", NULL, &lighting_alter, &lighting_show, NULL},
  {(char *)"Fast lighting        ", NULL, &fast_lighting_alter, &fast_lighting_show, NULL},
  {(char *)"Blending             ", NULL, &blending_alter, &blending_show, NULL},
  {(char *)"Clip 368 -> 352      ", NULL, &clip_368_alter, &clip_368_show, NULL},
#endif
  {(char *)"Restore defaults     ", &gpu_settings_defaults, NULL, NULL, NULL},
  {NULL, NULL, NULL, NULL, NULL},
  {(char *)"Back to main menu    ", &settings_back, NULL, NULL, NULL},
  {0}
};

#define SET_GPUSIZE ((sizeof(gui_GPUSettingsItems) / sizeof(MENUITEM)) - 1)
static MENU gui_GPUSettingsMenu = { SET_GPUSIZE, 0, 56, 112, (MENUITEM *)&gui_GPUSettingsItems };

static int xa_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.Xa == 1) Config.Xa = 0;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.Xa == 0) Config.Xa = 1;
  }

  return 0;
}

static char *xa_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", Config.Xa ? "off" : "on");
  return buf;
}

static int cdda_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.Cdda == 1) Config.Cdda = 0;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.Cdda == 0) Config.Cdda = 1;
  }

  return 0;
}

static char *cdda_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", Config.Cdda ? "off" : "on");
  return buf;
}

static int forcedxa_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.ForcedXAUpdates < 7) Config.ForcedXAUpdates++;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.ForcedXAUpdates > 0) Config.ForcedXAUpdates--;
  }

  return 0;
}

static char *forcedxa_show()
{
  if (Config.ForcedXAUpdates < 0) Config.ForcedXAUpdates = 0;
  else if (Config.ForcedXAUpdates > 7) Config.ForcedXAUpdates = 7;

  const char* str[] = { "off", "auto", "1", "2", "4", "8", "16", "32" };
  return (char*)str[Config.ForcedXAUpdates];
}

static int syncaudio_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.SyncAudio < 1) Config.SyncAudio = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.SyncAudio > 0) Config.SyncAudio = 0;
  }

  return 0;
}

static char *syncaudio_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", Config.SyncAudio ? "on" : "off");
  return buf;
}

static int spuupdatefreq_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.SpuUpdateFreq < 5) Config.SpuUpdateFreq++;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.SpuUpdateFreq > 0) Config.SpuUpdateFreq--;
  }

  return 0;
}

static char *spuupdatefreq_show()
{
  if (Config.SpuUpdateFreq < 0) Config.SpuUpdateFreq = 0;
  else if (Config.SpuUpdateFreq > 5) Config.SpuUpdateFreq = 5;

  const char* str[] = { "1", "2", "4", "8", "16", "32" };
  return (char*)str[Config.SpuUpdateFreq];
}

static int spuirq_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (Config.SpuIrq < 1) Config.SpuIrq = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (Config.SpuIrq > 0) Config.SpuIrq = 0;
  }

  return 0;
}

static char *spuirq_show()
{
  static char buf[16] = "\0";
  sprintf(buf, "%s", Config.SpuIrq ? "on" : "off");
  return buf;
}

#ifdef SPU_PCSXREARMED
static int interpolation_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (spu_config.iUseInterpolation < 3) spu_config.iUseInterpolation++;
  }
  else if (keys & KEY_LEFT)
  {
    if (spu_config.iUseInterpolation > 0) spu_config.iUseInterpolation--;
  }

  return 0;
}

static char *interpolation_show()
{
  static char buf[16] = "\0";
  switch (spu_config.iUseInterpolation)
  {
    case 0:
      strcpy(buf, "none");
      break;
    case 1:
      strcpy(buf, "simple");
      break;
    case 2:
      strcpy(buf, "gaussian");
      break;
    case 3:
      strcpy(buf, "cubic");
      break;
    default:
      buf[0] = '\0';
      break;
  }
  return buf;
}

static int reverb_alter(u32 keys)
{
  if (keys & KEY_RIGHT)
  {
    if (spu_config.iUseReverb < 1) spu_config.iUseReverb = 1;
  }
  else if (keys & KEY_LEFT)
  {
    if (spu_config.iUseReverb > 0) spu_config.iUseReverb = 0;
  }

  return 0;
}

static char *reverb_show()
{
  int val = spu_config.iUseReverb ? 1 : 0;
  const char* str[] = { "off", "on" };
  return (char*)str[val];
}

static int volume_alter(u32 keys)
{
  // Convert volume range 0..1024 to 0..16
  int val = spu_config.iVolume / 64;
  if (keys & KEY_RIGHT)
  {
    if (val < 16) val++;
  }
  else if (keys & KEY_LEFT)
  {
    if (val > 0) val--;
  }
  spu_config.iVolume = val * 64;
  return 0;
}

static char *volume_show()
{
  int val = spu_config.iVolume / 64;
  static char buf[16] = "\0";
  sprintf(buf, "%d", val);
  return buf;
}
#endif //SPU_PCSXREARMED

static int spu_settings_defaults()
{
  /* Restores settings to default values. */
  Config.Xa = 0;
  Config.Cdda = 0;
  Config.SyncAudio = 0;
  Config.SpuUpdateFreq = SPU_UPDATE_FREQ_DEFAULT;
  Config.ForcedXAUpdates = FORCED_XA_UPDATES_DEFAULT;
  Config.SpuIrq = 0;
#ifdef SPU_PCSXREARMED
  spu_config.iUseInterpolation = 0;
  spu_config.iUseReverb = 0;
  spu_config.iVolume = 1024;
#endif
  return 0;
}

static MENUITEM gui_SPUSettingsItems[] =
{
  {(char *)"XA audio             ", NULL, &xa_alter, &xa_show, NULL},
  {(char *)"CDDA audio           ", NULL, &cdda_alter, &cdda_show, NULL},
  {(char *)"Audio sync           ", NULL, &syncaudio_alter, &syncaudio_show, NULL},
  {(char *)"SPU updates per frame", NULL, &spuupdatefreq_alter, &spuupdatefreq_show, NULL},
  {(char *)"Forced XA updates    ", NULL, &forcedxa_alter, &forcedxa_show, NULL},
  {(char *)"IRQ fix              ", NULL, &spuirq_alter, &spuirq_show, NULL},
#ifdef SPU_PCSXREARMED
  {(char *)"Interpolation        ", NULL, &interpolation_alter, &interpolation_show, NULL},
  {(char *)"Reverb               ", NULL, &reverb_alter, &reverb_show, NULL},
  {(char *)"Master volume        ", NULL, &volume_alter, &volume_show, NULL},
#endif
  {(char *)"Restore defaults     ", &spu_settings_defaults, NULL, NULL, NULL},
  {NULL, NULL, NULL, NULL, NULL},
  {(char *)"Back to main menu    ", &settings_back, NULL, NULL, NULL},
  {0}
};

#define SET_SPUSIZE ((sizeof(gui_SPUSettingsItems) / sizeof(MENUITEM)) - 1)
static MENU gui_SPUSettingsMenu = { SET_SPUSIZE, 0, 56, 112, (MENUITEM *)&gui_SPUSettingsItems };

static int gui_LoadIso()
{
  static char isoname[PATH_MAX];
  const char *name = FileReq(Config.LastDir, NULL, isoname);

  if (name)
  {
    SetIsoFile(name);

    //If a multi-CD Eboot .pbp is detected, cdriso.cpp's handlepbp() will
    // call this function later to allow choosing which CD to boot
    cdrIsoMultidiskCallback = gui_select_multicd_to_boot_from;
    CheckforCDROMid_applyhacks();

    return 1;
  }

  return 0;
}

static int gui_Settings()
{
  gui_RunMenu(&gui_SettingsMenu);

  return 0;
}

static int gui_GPUSettings()
{
  gui_RunMenu(&gui_GPUSettingsMenu);

  return 0;
}

static int gui_SPUSettings()
{
  gui_RunMenu(&gui_SPUSettingsMenu);

  return 0;
}

static int gui_Quit()
{
  exit(0);
  return 0;
}

static void ShowMenuItem(int x, int y, MENUITEM *mi)
{
  static char string[PATH_MAX];

  if (mi->name)
  {
    if (mi->showval)
    {
      sprintf(string, "%s %s", mi->name, (*mi->showval)());
      port_printf(x, y, string);
    }
    else
    {
      port_printf(x, y, mi->name);
    }
  }
}

static void ShowMenu(MENU *menu)
{
  MENUITEM *mi = menu->m;

  // show menu lines
  for(int i = 0; i < menu->num; i++, mi++)
  {
    ShowMenuItem(menu->x, menu->y + i * 10, mi);
    // show hint if available
    if (mi->hint && i == menu->cur)
      mi->hint();
  }

  // show cursor
  port_printf(menu->x - 3 * 8, menu->y + menu->cur * 10, "-->");

  // general copyrights info
#if defined(RS97)
  port_printf( 8 * 8, 10, "PCSX4ALL v2.4 for RetroGame");
#else
  port_printf( 8 * 8, 10, "pcsx4all 2.4 for GCW-Zero");
#endif
  port_printf( 4 * 8, 20, "Built on " __DATE__ " at " __TIME__);
}

static int gui_RunMenu(MENU *menu)
{
  MENUITEM *mi;
  u32 keys;

  for (;;)
  {
    mi = menu->m + menu->cur;
    keys = key_read();

    video_clear();

    // check keys
    if (keys & _KEY_BACK)
    {
      if (menu == &gui_GameMenu)
      {
        while (key_read() != 0)
        {
          timer_delay(50);
        }
      }
      key_reset();
      return 0;
    }
    else if (keys & KEY_UP)
    {
      do
      {
        if (--menu->cur < 0)
          menu->cur = menu->num - 1;
      }
      while (!(menu->m + menu->cur)->name);   // Skip over an empty menu entry.

    }
    else if (keys & KEY_DOWN)
    {
      do
      {
        if (++menu->cur == menu->num)
          menu->cur = 0;
      }
      while (!(menu->m + menu->cur)->name);   // Skip over an empty menu entry.
    }
    else if (keys & KEY_A)
    {
      if (mi->on_press_a)
      {
        key_reset();
        int result = (*mi->on_press_a)();
        if (result)
          return result;
      }
    }
    else if (keys & KEY_B)
    {
      menu->cur = menu->num - 1;
      key_reset();
    }

    if ((keys & (KEY_LEFT | KEY_RIGHT)) && mi->on_press)
    {
      int result = (*mi->on_press)(keys);
      if (result)
        return result;
    }

    // diplay menu
    ShowMenu(menu);

    video_flip();
    timer_delay(75);

    if (keys & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_L | KEY_R |
                KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))
      timer_delay(50);
  }

  return 0;
}

/* 0 - exit, 1 - game loaded */
int SelectGame()
{
  return gui_RunMenu(&gui_MainMenu);
}

int GameMenu()
{
  //NOTE: TODO - reset 'saveslot' var to -1 if a new game is loaded.
  // currently, we don't support loading a different game during a running game.
  return gui_RunMenu(&gui_GameMenu);
}
