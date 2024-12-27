#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

int main(void)
{
  Display *dpy;
  Screen  *screen;
  XEvent ev;
  int SCREEN_WIDTH;
  int SCREEN_HEIGHT;

  if(!(dpy = XOpenDisplay(0x0))) return 1;

  screen = DefaultScreenOfDisplay(dpy);
  SCREEN_WIDTH = XWidthOfScreen(screen);
  SCREEN_HEIGHT = XHeightOfScreen(screen);

  XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureNotifyMask | SubstructureRedirectMask | KeyPressMask);

  XGrabKey (dpy, XKeysymToKeycode (dpy, XStringToKeysym("t")), Mod4Mask,
	   DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);
  while(1) {
      XNextEvent(dpy, &ev);
      switch(ev.type) {
	case KeyPress:
	  if ((XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0, 0) == 't') &&
	      ev.xkey.state & Mod4Mask)
	    {
	      XEvent event;
	      XGrabKey(dpy, AnyKey, AnyModifier, DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);
	      XMaskEvent(dpy, KeyPressMask, &event);
	      unsigned int key = XLookupKeysym((XKeyEvent *) &event, 0);
	      if (key == 'c')
		{
		  if(fork() == 0) {
		      execl("/bin/sh", "/bin/sh", "-c", "/usr/bin/xterm", (char *)NULL);
		  }
		}
	      XUngrabKey(dpy, AnyKey, AnyModifier, DefaultRootWindow(dpy));
	      XGrabKey(dpy, XKeysymToKeycode (dpy, XStringToKeysym("t")), Mod4Mask,
		       DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);
	    }
	  break;
	case MapRequest:
	  XMapWindow(dpy, ev.xmaprequest.window);
	  XMoveResizeWindow(dpy, ev.xmaprequest.window, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	  XRaiseWindow(dpy, ev.xmaprequest.window);
	  break;
	case ConfigureNotify:
	  ev.xconfigurerequest.type = ConfigureNotify;
	  ev.xconfigurerequest.x = 0;
	  ev.xconfigurerequest.y = 0;
	  ev.xconfigurerequest.width = SCREEN_WIDTH;
	  ev.xconfigurerequest.height = SCREEN_HEIGHT;
	  ev.xconfigurerequest.window = ev.xconfigure.window;
	  ev.xconfigurerequest.border_width = 0;
	  ev.xconfigurerequest.above = None;
	  XSendEvent(dpy, ev.xconfigurerequest.window, False, StructureNotifyMask, (XEvent*)&ev.xconfigurerequest);
	  break;
      }
  }
}
