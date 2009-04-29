#ifndef CONF_H
#define CONF_H

#define VERSION 	"0.2"
#define DISPLAY		":0"
#define MOD_MASK 	Mod4Mask /* Modifier key */
#define KEY_PREFIX	't' /* key to enter command mode */
#define KEY_WINLIST	'w' /* key to show window list */
#define KEY_TERMINAL 	'c' /* key to spawn terminal */
#define KEY_KILL	'q' /* key to kill selected window */
#define KEY_MENU 	'm' /* key to spawn menu */
#define KEY_PREV	'p' /* select previous window in list */
#define KEY_NEXT	'n' /* select next window in list */
#define TERMINAL	"urxvt -display :0" /* terminal */
#define MENU		"`dmenu_path | dmenu -fn '-xos4-terminus-*-r-*-*-12-*-*-*-*-*-*-*' -nb '#222222' -nf '#FFFFFF' -sf '#ffffff' -sb '#666666'`"
/* launcher menu to run */
#define FONT		"-xos4-terminus-*-*-*-*-12-*-*-*-*-*-*-*"
/* font to use */
#define BORDER		1 /* border size for window lists / input box */
#define FGCOLOR		"#909090" /* window list and messaging foreground color */
#define BGCOLOR		"#191919" /* window list and messaging background color */
#define SELFGCOLOR	"#000000" /* window list foreground for selected window */
#define SELBGCOLOR	"#909090" /* window list background for selected window */
#define PADDING_NORTH	0 /* top screen edge unmanaged pixels */
#define PADDING_WEST	0 /* left screen edge unmanaged pixels */
#define PADDING_SOUTH	0 /* bottom screen edge unmanaged pixels */
#define PADDING_EAST	0 /* right screen edge unmanaged pixels */
#define WLISTPADDING	5 /* left and right space in window list */
#define WLISTPOS	1 /* 0 = NW, 1 = NE, 2 = SE, 3 = SW, 4 = C */
#define TIMEOUT		2

#endif
