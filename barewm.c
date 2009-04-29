#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "conf.h"
#include <X11/cursorfont.h>

#define DEBUG 0

Display 		* display;
Window 			root;
Screen 			*screen;
int 			SCREEN_WIDTH;
int     		SCREEN_HEIGHT;
GC 			BARE_GC, BARE_SELECTEDFG_GC, BARE_SELECTEDBG_GC;
Colormap 		BARE_colormap = None;
XFontStruct  * fontstruct;

#define max_windows 999 
Window windows_container[max_windows];
Window selected;

void main_loop();
GC BARE_Colors(char *FG, char *BG);
void handle_keypress_event(XEvent *e);
void handle_maprequest_event(XEvent *e);
void handle_configure_event(XEvent *e);
void handle_destroy_event(XEvent *e);
void handle_expose_event(XEvent *e);
int handle_x_error(Display *display, XErrorEvent *e);
void handle_property_event(XEvent *e);
int init_gc();
void spawn(const char *cmd);
unsigned long name2color(const char *cid);
void LOG(const char *text, ...);
void LOG_DEBUG(const char *text, ...);
void list_windows();
int get_free_position();
int free_position(Window window);
int get_position(Window window);
int select_window(int window);
void grab_keyboard();
int get_prev_window();
int get_next_window();
void message(const char *text, ...);
void echo_output(char *cmd);

void LOG(const char *text, ...)
{
	va_list vl;
	va_start(vl, text);
	vfprintf(stderr, text, vl);
	va_end(vl);
}

void LOG_DEBUG(const char *text, ...)
{
	if(DEBUG == 1){
		va_list vl;
		va_start(vl, text);
		vfprintf(stderr, text, vl);
		va_end(vl);
	}
}

void spawn(const char *cmd)
{
	if(fork() == 0) {
		if(display) close(ConnectionNumber(display));
		setsid();
		execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
	}
}

int TextWidth(XFontStruct *fs, const char *text)
{
	if(strlen(text) >= 1 && text != NULL) {
		return XTextWidth(fs, text, strlen(text));
	} else {
		return 1;
	}
}

int TextHeight(XFontStruct *fs) {
	return fs->ascent + fs->descent;
}

unsigned long name2color(const char *cid)
{
	XColor tmpcol;

	if(!XParseColor(display, BARE_colormap, cid, &tmpcol)) {
		LOG("Cannot allocate \"%s\" color. Defaulting to black!\n", cid);
		return BlackPixel(display, XDefaultScreen(display));
	}
	if(!XAllocColor(display, BARE_colormap, &tmpcol)) {
		LOG("Cannot allocate \"%s\" color. Defaulting to black!\n", cid);
		return BlackPixel(display, XDefaultScreen(display));
	}
	
	return tmpcol.pixel;
}

int init_gc()
{
	XGCValues gcv;
   	gcv.font = fontstruct->fid;
   	gcv.foreground = name2color(FGCOLOR);
   	gcv.background = name2color(BGCOLOR);
   	gcv.function = GXcopy;
   	gcv.subwindow_mode = IncludeInferiors;
   	gcv.line_width = 1;
   	BARE_GC = XCreateGC(display, root, GCForeground | GCBackground | GCFunction | GCLineWidth | GCSubwindowMode | GCFont, &gcv);
   	gcv.foreground = name2color(SELFGCOLOR);
   	BARE_SELECTEDFG_GC = XCreateGC(display, root, GCForeground | GCBackground | GCFunction | GCLineWidth | GCSubwindowMode | GCFont, &gcv);
	gcv.foreground = name2color(SELBGCOLOR);
	BARE_SELECTEDBG_GC = XCreateGC(display, root, GCForeground | GCBackground | GCFunction | GCLineWidth | GCSubwindowMode | GCFont, &gcv);

	return 0;
}
int get_prev_window()
{
	int x;
	for(x = get_position(selected) - 1; x >= 0; x--)
	{
		if(windows_container[x] != None)
		{
			LOG_DEBUG("Found previous window at: %d\n", x);
			return x;
		}
	}

	return -1;
}
int get_next_window()
{
	int x;
	for(x = get_position(selected) + 1; x < max_windows; x++)
	{
		if(windows_container[x] != None)
		{
			LOG_DEBUG("Found next window at: %d\n", x);
			return x;
		}
	}
	return -1;
}
void grab_keyboard()
{
	XGrabKey(display, XKeysymToKeycode (display, KEY_PREFIX), MOD_MASK, root, True, GrabModeAsync, GrabModeAsync);
}

void echo_output(char *cmd)
{
	int ch = 0;
	int x = 0;
	char s[256];
	FILE *out = popen(cmd, "r");
	if(out)
	{
		while(ch != EOF && ch != '\n') {
			ch = fgetc(out);
			s[x] = ch;
			x++;
		}
		s[x -1] = 0;
		pclose(out);
		message(s);
		s[0] = 0
	}
}

void handle_keypress_event(XEvent * e)
{
	XEvent event;
	XGrabKey(display, AnyKey, AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
        XMaskEvent (display, KeyPressMask, &event);
	XDefineCursor(display, selected, (XCreateFontCursor(display, CURSOR)));
	unsigned int key = XLookupKeysym((XKeyEvent *) &event, 0);
	if (key >= '0' && key <= '9')
	{
		XUngrabKey(display, AnyKey, AnyModifier, root);
		grab_keyboard();
		select_window(key - '0');
		return;
	}
	if(key == '-')
	{
		XUngrabKey(display, AnyKey, AnyModifier, root);
		grab_keyboard();
		select_window(root);
		return;
	}
	switch (key)
        {       
		case KEY_TERMINAL:
			spawn(TERMINAL);
			break;
		case KEY_MENU:
			spawn(MENU);
			break;
		case KEY_STATUS:
			echo_output(STATUS);
			break;
		case KEY_WINLIST:
			if(TIMEOUT > 0)
			{
				list_windows();
			}			
			break;
		case KEY_KILL:
			XDestroyWindow(display, selected);
			selected = root;
			break;
		case KEY_PREV:
			if(get_prev_window() != -1){
				select_window(get_prev_window());
			} else {
				message("Can't access previous window!");
			}
			break;
		case KEY_NEXT:
			if(get_next_window() != -1)
			{
				select_window(get_next_window());
			} else {
				message("Can't access next window!");
			}
			break;
                default:
                        message("Key \"%c\" is unbound!", (char)key);
	}

	XUngrabKey(display, AnyKey, AnyModifier, root);
	grab_keyboard();
	XSetInputFocus (display, selected, RevertToParent, CurrentTime);
}
void message(const char *text, ...)
{
	char tmp[256];
	va_list vl;
	va_start(vl, text);
	vsprintf(tmp, text, vl);
	va_end(vl);
	tmp[strlen(tmp)] = 0;
        
	int th = TextHeight(fontstruct);
	int char_width = TextWidth(fontstruct, " ");
	int win_width = (strlen(tmp) * char_width) + (WLISTPADDING * 2);		
	int win_height = th;
	int win_x, win_y;
	switch(WLISTPOS)
		{
			case 0:
				win_x = PADDING_WEST;
				win_y = PADDING_NORTH;
				break;
			case 1:
				win_x = SCREEN_WIDTH - PADDING_EAST - win_width - (BORDER * 2);
				win_y = PADDING_NORTH;
				break;
			case 2:
				win_x = 0 + SCREEN_WIDTH - PADDING_EAST - win_width;
				win_y = 0 + SCREEN_HEIGHT - PADDING_SOUTH - win_height;
				break;
			case 3:
				win_x = PADDING_WEST;
				win_y = 0 + SCREEN_HEIGHT - PADDING_SOUTH - win_height;
				break;
			case 4:
				win_x = (SCREEN_WIDTH / 2) - (win_width / 2);
				win_y = (SCREEN_HEIGHT / 2) - (win_height / 2);
				break;
			default:
				win_x = PADDING_WEST;
				win_y = PADDING_NORTH;
				break;
		}
	Window message_window = XCreateSimpleWindow(display, root, win_x,  win_y, win_width, win_height, BORDER, name2color(FGCOLOR), name2color(BGCOLOR));
	XSetWindowBorderWidth(display, message_window, BORDER);		
	XSetWindowBorder(display, message_window, name2color(SELBGCOLOR));		
	XMapRaised(display, message_window);
	XDrawString(display, message_window, BARE_GC, WLISTPADDING, 0 + th - fontstruct->max_bounds.descent, tmp, strlen(tmp));
	XFlush(display);
	sleep(TIMEOUT);
	XFlush(display);
	if(message_window)
	{
		XDestroyWindow(display, message_window);
	}
}


void list_windows()
{
	int th, ypos, x;
        th = TextHeight(fontstruct);
	ypos = 0 + th - fontstruct->max_bounds.descent;
	Window WINDOW_LIST_WINDOW = None;
	char *tmp;
	char title[256];
	int number = 0;
	int max_title = 0;
	XWindowAttributes winattr;
	Window root_return;
	int char_width = TextWidth(fontstruct, " ");

 	for (x = 0; x< max_windows; x++)
	{
		if(windows_container[x] != None)
		{
			if(!XGetWindowAttributes(display, windows_container[x], &winattr) || winattr.override_redirect || XGetTransientForHint(display, windows_container[x], &root_return)) continue;
				if(winattr.map_state == IsViewable)
				{
					if(XFetchName(display, windows_container[x], &tmp))
					{
						number++;
						if(windows_container[x] == selected)
						{	
							sprintf(title, "%d - %s", get_position(windows_container[x]), tmp);
						} else {
							sprintf(title, "%d - %s", get_position(windows_container[x]), tmp);
						}
						if(strlen(title) > max_title) max_title = strlen(title);
						title[0] = 0;	
					}
				}
			}
		}
	if(number > 0)
	{	
		int win_width = (max_title * char_width) + (WLISTPADDING * 2);		
		int win_height = number * th;
		int win_x, win_y;
		switch(WLISTPOS)
		{
			case 0:
				win_x = PADDING_WEST;
				win_y = PADDING_NORTH;
				break;
			case 1:
				win_x = SCREEN_WIDTH - PADDING_EAST - win_width - (BORDER * 2);
				win_y = PADDING_NORTH;
				break;
			case 2:
				win_x = 0 + SCREEN_WIDTH - PADDING_EAST - win_width;
				win_y = 0 + SCREEN_HEIGHT - PADDING_SOUTH - win_height;
				break;
			case 3:
				win_x = PADDING_WEST;
				win_y = 0 + SCREEN_HEIGHT - PADDING_SOUTH - win_height;
				break;
			case 4:
				win_x = (SCREEN_WIDTH / 2) - (win_width / 2);
				win_y = (SCREEN_HEIGHT / 2) - (win_height / 2);
				break;
			default:
				win_x = PADDING_WEST;
				win_y = PADDING_NORTH;
				break;
		}
		WINDOW_LIST_WINDOW = XCreateSimpleWindow(display, root, win_x,  win_y, win_width, win_height, BORDER, name2color(FGCOLOR), name2color(BGCOLOR));
		XSetWindowBorderWidth(display, WINDOW_LIST_WINDOW, BORDER);		
		XSetWindowBorder(display, WINDOW_LIST_WINDOW, name2color(SELBGCOLOR));		
		XMapRaised(display, WINDOW_LIST_WINDOW);

		for (x = 0; x< max_windows; x++)
		{
			if(windows_container[x] != None)
			{
				if(!XGetWindowAttributes(display, windows_container[x], &winattr) || winattr.override_redirect || XGetTransientForHint(display, windows_container[x], &root_return)) continue;
					if(winattr.map_state == IsViewable)
					{
						if(XFetchName(display, windows_container[x], &tmp))
						{
							if(windows_container[x] == selected)
							{
								sprintf(title, "%d - %s", get_position(windows_container[x]), tmp);
								XFillRectangle(display, WINDOW_LIST_WINDOW, BARE_SELECTEDBG_GC, 0, ypos - th  + fontstruct->max_bounds.descent, win_width, th);
								XDrawString(display, WINDOW_LIST_WINDOW, BARE_SELECTEDFG_GC, WLISTPADDING, ypos, title, strlen(title));
								ypos+=th;
							} else {
								sprintf(title, "%d - %s", get_position(windows_container[x]), tmp);
								XDrawString(display, WINDOW_LIST_WINDOW, BARE_GC, WLISTPADDING, ypos, title, strlen(title));
								ypos+=th;
							}
						
						} else {
							sprintf(title, "%d - %s", get_position(windows_container[x]), tmp);
							XDrawString(display, WINDOW_LIST_WINDOW, BARE_GC, WLISTPADDING, ypos, title, strlen(title));
							ypos+=th;
						}
						title[0] = 0;
					}
			}
		}
		XFlush(display);
		sleep(TIMEOUT);
		XFlush(display);
		if(WINDOW_LIST_WINDOW)
		{
			XDestroyWindow(display, WINDOW_LIST_WINDOW);
		}
	} else {
		message("No windows to list!");
	}
}

int select_window(int window)
{
	if(windows_container[window] != None)
	{
		LOG_DEBUG("Selecting window at position: %d\n", window);
		XRaiseWindow(display, windows_container[window]);
		XSetInputFocus(display, windows_container[window], RevertToParent, CurrentTime);
		selected = windows_container[window];
		return 0;
	} else {
		return -1;
	}
}

int get_free_position()
{
	int x;
	for(x = 0; x < max_windows; x++)
	{
		if(windows_container[x] == None)
		{
			LOG_DEBUG("Asigning position: %d\n", x);
			return x;
		}
	}
	return -1;
}
int get_position(Window window)
{
	int x;
	for(x = 0; x < max_windows; x++)
	{
		if(windows_container[x] == window)
		{
			LOG_DEBUG("Window has position: %d\n", x);
			return x;
		}
	}
	return -1;
}

int free_position(Window window)
{
	int x;
	for(x = 0; x < max_windows; x++)
	{
		if(windows_container[x] == window)
		{
			LOG_DEBUG("Freeing position: %d\n", x);
			windows_container[x] = None;
		}
	}
	return 1;
}

void handle_maprequest_event(XEvent *e)
{
	XMapWindow(display, e->xmaprequest.window);
	XMoveResizeWindow(display, e->xmaprequest.window, PADDING_WEST, PADDING_NORTH, SCREEN_WIDTH - PADDING_WEST - PADDING_EAST, SCREEN_HEIGHT - PADDING_NORTH - PADDING_SOUTH);
	XRaiseWindow(display, e->xmaprequest.window);
	XSetInputFocus(display, e->xmaprequest.window,RevertToParent, CurrentTime);
	selected = e->xmaprequest.window;
	windows_container[get_free_position()] = selected;
}

void handle_destroy_event(XEvent *e)
{
	free_position(e->xdestroywindow.window);
	XDestroyWindow(display, e->xdestroywindow.window);
}


void handle_configure_event(XEvent *e)
{
      e->xconfigurerequest.type = ConfigureNotify;
      e->xconfigurerequest.x = 0;
      e->xconfigurerequest.y = 0;
      e->xconfigurerequest.width = SCREEN_WIDTH;
      e->xconfigurerequest.height = SCREEN_HEIGHT;
      e->xconfigurerequest.window = e->xconfigure.window;
      e->xconfigurerequest.border_width = 0;      
      e->xconfigurerequest.above = None;
      XSendEvent(display, e->xconfigurerequest.window, False, StructureNotifyMask, (XEvent*)&e->xconfigurerequest);
}

void handle_expose_event(XEvent *e)
{
	// Redraw stuff in here...window title etc
	// Not handled yet but I'ma add it here for future dev
}

void handle_property_event(XEvent *e)
{
	// In case properties like name etc change ... handle them in here
	// Not handled yet but I'ma add it here for future dev
}

int handle_x_error(Display *display, XErrorEvent *e)
{
	LOG_DEBUG("Xevent error: %d\n", e->error_code);
	LOG_DEBUG("Operation: %d\n", e->request_code);
	LOG_DEBUG("Resource: %lu (0x%lx)\n", e->resourceid, e->resourceid);
	return 0;
}

void main_loop()
{
	XEvent event;
	XSetErrorHandler(handle_x_error); //Ignore X errors otherwise the WM would crash every other minute :)
	while(1){
       		XNextEvent(display, &event);
       		switch(event.type){
		case KeyPress:
			if ((XKeycodeToKeysym(display, event.xkey.keycode, 0) == KEY_PREFIX) && (event.xkey.state & MOD_MASK))
			{       
				LOG_DEBUG("Switching to command mode\n");
				XDefineCursor(display, selected, (XCreateFontCursor(display, CMD_CURSOR)));
				handle_keypress_event(&event);
			}
			break;
		case MapRequest:
			handle_maprequest_event(&event);
			break;
		case DestroyNotify: 
			handle_destroy_event(&event);
			break;
		case ConfigureNotify: 
			handle_configure_event(&event);
			break;
		case Expose: 
			handle_expose_event(&event);
			break;
		case PropertyNotify:
			handle_property_event(&event);
			break;
		default:
			LOG_DEBUG("Received an unhandled event:  %d\n", event.type);
			break;
			}
	}
}

int main(int argc, char *argv[])
{
	if(!(display = XOpenDisplay(DISPLAY))){
		LOG("BARE: cannot open display! Ending session.\n");
		return -1;
	}
	if((root = DefaultRootWindow(display)))
	{
		XSetWindowBackground(display, root, BlackPixel(display, XDefaultScreen(display)));
		XClearWindow(display, root);
	} else {
		LOG("BARE: cannot get root window! Ending session.\n");
		return -1;
	}
	if((screen = DefaultScreenOfDisplay(display)))
	{
		SCREEN_WIDTH 	= XWidthOfScreen(screen);
		SCREEN_HEIGHT 	= XHeightOfScreen(screen);
		LOG("Screen: %d x %d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
	} else {
		LOG("BARE: cannot get screen! Ending session.\n");
		return -1;	
	}
	selected = root;
	fontstruct = XLoadQueryFont(display, FONT);
	if (!fontstruct) {
		LOG("Couldn't find font \"%s\", loading default\n", FONT);
		fontstruct = XLoadQueryFont(display, "-*-fixed-medium-r-*-*-12-*-*-*-*-*-iso8859-1");
		if (!fontstruct) {
			LOG("Couldn't load default fixed font. Something is seriouslly wrong. Ending session.\n");
			return -1;		
		}

	}
	XDefineCursor(display, selected, (XCreateFontCursor(display, CURSOR)));
	grab_keyboard();	
	XSelectInput(display, root, SubstructureNotifyMask | SubstructureRedirectMask | KeyPressMask); 

	BARE_colormap = DefaultColormap(display, 0);
	init_gc();
	message("Welcome to Bare WM v%s", VERSION);
	main_loop();

	XFree(BARE_GC);
	XFree(BARE_SELECTEDFG_GC);
	XCloseDisplay(display);
	return 0;
}
