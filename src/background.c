/**
 * @file background.c
 * @author Joe Wingbermuehle
 * @date 2007
 *
 * @brief Background control functions.
 *
 */

#include "jwm.h"
#include "background.h"
#include "misc.h"
#include "error.h"
#include "command.h"
#include "color.h"
#include "main.h"
#include "icon.h"
#include "image.h"
#include "gradient.h"
#include "hint.h"

/** Enumeration of background types. */
typedef unsigned char BackgroundType;
#define BACKGROUND_SOLID      0  /**< Solid color background. */
#define BACKGROUND_GRADIENT   1  /**< Gradient background. */
#define BACKGROUND_COMMAND    2  /**< Command to run. */
#define BACKGROUND_STRETCH    3  /**< Stretched image. */
#define BACKGROUND_TILE       4  /**< Tiled image. */

/** Structure to represent a background for one or more desktops. */
typedef struct BackgroundNode {
   int desktop;                  /**< The desktop. */
   BackgroundType type;          /**< The type of background. */
   char *value;
   Pixmap pixmap;
   struct BackgroundNode *next;  /**< Next background in the list. */
} BackgroundNode;

/** Linked list of backgrounds. */
static BackgroundNode *backgrounds;

/** The default background. */
static BackgroundNode *defaultBackground;

/** The last background loaded. */
static BackgroundNode *lastBackground;

static void LoadGradientBackground(BackgroundNode *bp);
static void LoadImageBackground(BackgroundNode *bp);

/** Initialize any data needed for background support. */
void InitializeBackgrounds(void)
{
   backgrounds = NULL;
   defaultBackground = NULL;
   lastBackground = NULL;
}

/** Startup background support. */
void StartupBackgrounds(void)
{

   BackgroundNode *bp;

   for(bp = backgrounds; bp; bp = bp->next) {

      /* Load background data. */
      switch(bp->type) {
      case BACKGROUND_SOLID:
      case BACKGROUND_GRADIENT:
         LoadGradientBackground(bp);
         break;
      case BACKGROUND_COMMAND:
         /* Nothing to do. */
         break;
      case BACKGROUND_STRETCH:
      case BACKGROUND_TILE:
         LoadImageBackground(bp);
         break;
      default:
         Debug("invalid background type in LoadBackground: %d", bp->type);
         break;
      }

      if(bp->desktop == -1) {
         defaultBackground = bp;
      }

   }

}

/** Shutdown background support. */
void ShutdownBackgrounds(void)
{

   BackgroundNode *bp;

   for(bp = backgrounds; bp; bp = bp->next) {
      if(bp->pixmap != None) {
         JXFreePixmap(display, bp->pixmap);
         bp->pixmap = None;
      }
   }

}

/** Release any data needed for background support. */
void DestroyBackgrounds(void)
{

   BackgroundNode *bp;

   while(backgrounds) {
      bp = backgrounds->next;
      Release(backgrounds->value);
      Release(backgrounds);
      backgrounds = bp;
   }

}

/** Set the background to use for the specified desktops. */
void SetBackground(int desktop, const char *type, const char *value)
{

   BackgroundType bgType;
   BackgroundNode *bp;
   BackgroundNode **bpp;

   /* Make sure we have a value. */
   if(JUNLIKELY(!value)) {
      Warning(_("no value specified for background"));
      return;
   }

   /* Parse the background type. */
   if(!type || !strcmp(type, "solid")) {
      bgType = BACKGROUND_SOLID;
   } else if(!strcmp(type, "gradient")) {
      bgType = BACKGROUND_GRADIENT;
   } else if(!strcmp(type, "command")) {
      bgType = BACKGROUND_COMMAND;
   } else if(!strcmp(type, "image")) {
      bgType = BACKGROUND_STRETCH;
   } else if(!strcmp(type, "tile")) {
      bgType = BACKGROUND_TILE;
   } else {
      Warning(_("invalid background type: \"%s\""), type);
      return;
   }

   /* Remove the existing background if this is a duplicate.
    * This allows later settings to override older settings.
    * Note that there can be at most one duplicate.
    */
   bpp = &backgrounds;
   while(*bpp) {
      bp = *bpp;
      if(bp->desktop == desktop) {
         *bpp = bp->next;
         Release(bp->value);
         Release(bp);
         break;
      }
      bpp = &bp->next;
   }

   /* Create the background node. */
   bp = Allocate(sizeof(BackgroundNode));
   bp->desktop = desktop;
   bp->type = bgType;
   bp->value = CopyString(value);

   /* Insert the node into the list. */
   bp->next = backgrounds;
   backgrounds = bp;

}

/** Load the background for the specified desktop. */
void LoadBackground(int desktop)
{

   XSetWindowAttributes attr;
   long attrValues;
   BackgroundNode *bp;

   /* Determine the background to load. */
   for(bp = backgrounds; bp; bp = bp->next) {
      if(bp->desktop == desktop) {
         break;
      }
   }
   if(!bp) {
      bp = defaultBackground;
   }

   /* If there is no background specified for this desktop, just return. */
   if(!bp || !bp->value) {
      return;
   }

   /* If the background isn't changing, don't do anything. */
   if(   lastBackground
      && bp->type == lastBackground->type
      && !strcmp(bp->value, lastBackground->value)) {
      return;
   }
   lastBackground = bp;

   /* Load the background based on type. */
   if(bp->type == BACKGROUND_COMMAND) {
      RunCommand(bp->value);
      return;
   }

   attrValues = CWBackPixmap;
   attr.background_pixmap = bp->pixmap;
   JXChangeWindowAttributes(display, rootWindow, attrValues, &attr);
   SetPixmapAtom(rootWindow, ATOM_XROOTPMAP_ID, bp->pixmap);
   JXClearWindow(display, rootWindow);

}

/** Load a gradient background. */
void LoadGradientBackground(BackgroundNode *bp)
{

   XColor color1;
   XColor color2;
   char *temp;
   char *sep;
   int len;

   sep = strchr(bp->value, ':');
   if(sep) {

      /* Gradient background. */

      /* Get the first color. */
      len = (int)(sep - bp->value);
      temp = AllocateStack(len + 1);
      memcpy(temp, bp->value, len);
      temp[len] = 0;
      ParseColor(temp, &color1);
      ReleaseStack(temp);

      /* Get the second color. */
      len = strlen(sep + 1);
      temp = AllocateStack(len + 1);
      memcpy(temp, sep + 1, len);
      temp[len] = 0;
      ParseColor(temp, &color2);
      ReleaseStack(temp);

   } else {

      /* Solid background. */
      ParseColor(bp->value, &color1);
      color2.pixel = color1.pixel;

   }

   /* Create the background pixmap. */
   if(color1.pixel == color2.pixel) {
      bp->pixmap = JXCreatePixmap(display, rootWindow, 1, 1,
                                  rootVisual.depth);
      JXSetForeground(display, rootGC, color1.pixel);
      JXDrawPoint(display, bp->pixmap, rootGC, 0, 0);
   } else {
      bp->pixmap = JXCreatePixmap(display, rootWindow, 1, rootHeight,
                                  rootVisual.depth);
      DrawHorizontalGradient(bp->pixmap, rootGC, color1.pixel,
                             color2.pixel, 0, 0, 1, rootHeight);
   }

}

/** Load an image background. */
void LoadImageBackground(BackgroundNode *bp)
{

   IconNode *ip;
   int width, height;

   /* Load the icon. */
   ExpandPath(&bp->value);
   ip = LoadNamedIcon(bp->value, 0);
   if(JUNLIKELY(!ip)) {
      bp->pixmap = None;
      Warning(_("background image not found: \"%s\""), bp->value);
      return;
   }

   /* Determine the size of the background pixmap. */
   if(bp->type == BACKGROUND_TILE) {
      width = ip->image->width;
      height = ip->image->height;
   } else {
      width = rootWidth;
      height = rootHeight;
   }

   /* Create the pixmap. */
   bp->pixmap = JXCreatePixmap(display, rootWindow,
                               width, height, rootVisual.depth);

   /* Clear the pixmap in case it is too small. */
   JXSetForeground(display, rootGC, 0);
   JXFillRectangle(display, bp->pixmap, rootGC, 0, 0, width, height);

   /* Draw the icon on the background pixmap. */
   PutIcon(&rootVisual, ip, bp->pixmap, 0, 0, 0, width, height);

   /* We don't need the icon anymore. */
   DestroyIcon(ip);

}
