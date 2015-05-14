
// Entry point for a linux 32 bit version

#include <stdio.h>
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xlib.h>

int main(int argc, char* args[])
{

	// Create window
	int screen_num;
	int width;
	int height;
	unsigned long background;
	unsigned long border;
	Window win;
	XEvent event;
	Display *display;

	// Connect to display server
	// NULL means that it is the local machine

	display = XOpenDisplay(NULL);
	if (!display) {
		fprintf(stderr, "Unable to open display");
		return 7;
	}

	// collect useful data 
	screen_num = DefaultScreen(display);
	background = BlackPixel(display, screen_num);
	border = WhitePixel(display, screen_num);

	width = 400;
	height = 400;

	/* display, parent window
		position, overridden by window manager
		width and height of window
		border width and color
		background color
	*/
	win = XCreateSimpleWindow( display, DefaultRootWindow(display), 
				0, 0, 
				width, height, 
				2, border,
				background );

	// how to check if win was created?

	// Sign up for these events
	XSelectInput( display, win, ButtonPressMask|StructureNotifyMask );

	// put the window on the screen
	XMapWindow( display, win);

	// Support window manager closing the window
	Atom WM_DELETE_WINDOW = XInternAtom( display, "WM_DELETE_WINDOW", False );
	XSetWMProtocols(display, win, &WM_DELETE_WINDOW, 1);
	// Handle events

	while(1)
	{
		XNextEvent( display, &event );
		switch( event.type )
		{
			case ConfigureNotify:
			{
				if (width != event.xconfigure.width 
					|| height != event.xconfigure.height)
					{
						width = event.xconfigure.width;
						height = event.xconfigure.height;
						printf("Size changed to: %d x %d\n", width, height);
					}
			}
			break;
			case ButtonPress:
			{
				XCloseDisplay( display );
				return 0;
			}
			break;
			case ClientMessage:
			{
				// We only registered WM_DELETE_WINDOW
				// no need to check other types
				XCloseDisplay( display );
				return 0;
			} break;
		}
	}
}
