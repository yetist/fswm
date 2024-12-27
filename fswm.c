#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>

int main(void)
{
  Display *dpy;
  Screen  *screen;
  XEvent   ev;
  XColor   ecolor;
  int SCREEN_WIDTH;
  int SCREEN_HEIGHT;

  if(!(dpy = XOpenDisplay(0x0))) return 1;

  screen = DefaultScreenOfDisplay(dpy);
  SCREEN_WIDTH = XWidthOfScreen(screen);
  SCREEN_HEIGHT = XHeightOfScreen(screen);

  /* Setup cursor */
  XDefineCursor(dpy, DefaultRootWindow(dpy), XCreateFontCursor(dpy, XC_left_ptr));

  /* Setup background color */
  if (XParseColor(dpy, XDefaultColormapOfScreen(screen), "blue", &ecolor)) {
    if (XAllocColor(dpy,XDefaultColormapOfScreen(screen), &ecolor)) {
      XSetWindowBackground(dpy, XRootWindowOfScreen(screen), ecolor.pixel);
      XClearWindow(dpy, XRootWindowOfScreen(screen));
    }
  }

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
              setsid();
              execl("/bin/sh", "/bin/sh", "-c", "/usr/bin/xterm", (char *)NULL);
            }
          }
          XUngrabKey(dpy, AnyKey, AnyModifier, DefaultRootWindow(dpy));
          XGrabKey(dpy, XKeysymToKeycode (dpy, XStringToKeysym("t")), Mod4Mask,
                   DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);
        }
        break;
      case MapRequest:
        {
          XWindowAttributes wa;
          XMapWindow(dpy, ev.xmaprequest.window);
          if (XGetWindowAttributes(dpy, ev.xmaprequest.window, &wa))
          {
            XMoveResizeWindow(dpy, ev.xmaprequest.window,
                              (SCREEN_WIDTH - wa.width)/2,
                              (SCREEN_HEIGHT - wa.height)/2,
                              wa.width, wa.height);
          }
          XRaiseWindow(dpy, ev.xmaprequest.window);
        }
        break;
      case ConfigureNotify:
        ev.xconfigurerequest.type = ConfigureNotify;
        ev.xconfigurerequest.x = (SCREEN_WIDTH - ev.xconfigure.width)/2;
        ev.xconfigurerequest.y = (SCREEN_HEIGHT - ev.xconfigure.height)/2;
        ev.xconfigurerequest.width = ev.xconfigure.width;
        ev.xconfigurerequest.height = ev.xconfigure.height;
        ev.xconfigurerequest.window = ev.xconfigure.window;
        ev.xconfigurerequest.border_width = 0;
        ev.xconfigurerequest.above = None;
        XSendEvent(dpy, ev.xconfigurerequest.window, False, StructureNotifyMask, (XEvent*)&ev.xconfigurerequest);
        break;
      case ClientMessage:
        if (ev.xclient.message_type == XInternAtom(dpy, "_NET_WM_STATE", False)) {
          if ((Atom)ev.xclient.data.l[1] == XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False) ||
            (Atom)ev.xclient.data.l[2] == XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False)) {
            if (ev.xclient.data.l[0] > 0) {
              XMoveResizeWindow(dpy, ev.xclient.window, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            }
          }
        }
        break;
    }
  }
}
