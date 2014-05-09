#ifndef MATE_RR_PRIVATE_H
#define MATE_RR_PRIVATE_H

#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif

typedef struct ScreenInfo ScreenInfo;

struct ScreenInfo
{
    int			min_width;
    int			max_width;
    int			min_height;
    int			max_height;

#ifdef HAVE_RANDR
    XRRScreenResources *resources;
#endif
    
    MateRROutput **	outputs;
    MateRRCrtc **	crtcs;
    MateRRMode **	modes;
    
    MateRRScreen *	screen;

    MateRRMode **	clone_modes;

#ifdef HAVE_RANDR
    RROutput            primary;
#endif
};

struct MateRRScreenPrivate
{
    GdkScreen *			gdk_screen;
    GdkWindow *			gdk_root;
    Display *			xdisplay;
    Screen *			xscreen;
    Window			xroot;
    ScreenInfo *		info;
    
    int				randr_event_base;
    int				rr_major_version;
    int				rr_minor_version;
    
    Atom                        connector_type_atom;
};

#endif
