#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>

#define VERSION "0.1"
static char *program_name;

static void usage(const char *errmsg)
{
  if (errmsg != NULL)
    fprintf (stderr, "%s: %s\n\n", program_name, errmsg);

  fprintf(stderr, "Usage: %s [options]\n%s\n", program_name,
          "  where options are:\n"
          "  -h, --help                     Print this help\n"
          "  -v, --version                  Print a version message\n"
          "  -s, --solid   <color>          Set the background of the root window\n"
          "  -e, --command <string>         Run the command after startup\n"
          "  -d, --display <display>        Specifies the server to connect to\n"
          );
  exit(1);
}

static void signal_handler(int signum)
{
  /* Take appropriate actions for signal delivery */
  switch (signum) {
    case SIGUSR1:
      fprintf(stderr, "Caught signal %d. Restarting.", signum);
      break;
    case SIGUSR2:
      fprintf(stderr, "Caught signal %d. Reconfiguring.", signum);
      break;
    case SIGCHLD:
      /* reap children */
      while (waitpid(-1, NULL, WNOHANG) > 0);
      break;
    case SIGTTIN:
    case SIGTTOU:
      fprintf(stderr, "Caught signal %d. Ignoring.", signum);
      break;
    default:
      fprintf(stderr, "Caught signal %d. Exiting.", signum);
      /* TERM and INT return a 0 code */
      exit(!(signum == SIGTERM || signum == SIGINT));
  }
}


int main(int argc, char *argv[])
{
  Display *dpy;
  Screen  *screen;
  XColor   ecolor;
  int screen_width;
  int screen_height;

  char *display_name = NULL;
  char *command      = NULL;
  char *solid_color  = NULL;
  int command_running = 0;

  program_name=argv[0];

  for (int i = 1; i < argc; i++) {
    if (!strcmp("-h", argv[i]) || !strcmp("--help", argv[i])) {
      usage(NULL);
    }
    if (!strcmp("-v", argv[i]) || !strcmp("--version", argv[i])) {
      printf("%s\n", VERSION);
      exit(0);
    }
    if (!strcmp ("-d", argv[i]) || !strcmp ("--display", argv[i])) {
      if (++i>=argc) usage ("--display requires an argument");
      display_name = argv[i];
      continue;
    }
    if (!strcmp("-s", argv[i]) || !strcmp("--solid", argv[i])) {
      if (++i>=argc) usage("--solid requires an argument");
      solid_color = argv[i];
      continue;
    }
    if (!strcmp("-e", argv[i]) || !strcmp("--command", argv[i])) {
      if (++i>=argc) usage("--command requires an argument");
      command = argv[i];
      continue;
    }
  }

  if (command == NULL) {
    command = "xterm";
  }

  signal(SIGUSR1, signal_handler);
  signal(SIGUSR2, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGINT,  signal_handler);
  signal(SIGHUP,  signal_handler);
  signal(SIGPIPE, signal_handler);
  signal(SIGCHLD, signal_handler);
  signal(SIGTTIN, signal_handler);
  signal(SIGTTOU, signal_handler);

  dpy = XOpenDisplay(display_name);
  if (!dpy) {
    fprintf(stderr, "%s:  unable to open display '%s'\n",
            program_name, XDisplayName (display_name));
    exit (2);
  }

  screen = DefaultScreenOfDisplay(dpy);
  screen_width = XWidthOfScreen(screen);
  screen_height = XHeightOfScreen(screen);

  setenv("DISPLAY", DisplayString(dpy), 1);

  /* Setup cursor */
  XDefineCursor(dpy, DefaultRootWindow(dpy), XCreateFontCursor(dpy, XC_left_ptr));

  /* Setup background color */
  if (solid_color != NULL) {
    if (XParseColor(dpy, XDefaultColormapOfScreen(screen), solid_color, &ecolor)) {
      /* Setup background color */
      if (XAllocColor(dpy,XDefaultColormapOfScreen(screen), &ecolor)) {
        XSetWindowBackground(dpy, XRootWindowOfScreen(screen), ecolor.pixel);
        XClearWindow(dpy, XRootWindowOfScreen(screen));
      }
    }
  }

  XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureNotifyMask | SubstructureRedirectMask | KeyPressMask);
  XGrabKey (dpy, XKeysymToKeycode (dpy, XStringToKeysym("t")), Mod4Mask,
            DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);

  while(1) {
    while (XPending(dpy)) {
      XEvent ev;
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
                execl("/usr/bin/xterm", "xterm", (char *)NULL);
              }
            }
            XUngrabKey(dpy, AnyKey, AnyModifier, DefaultRootWindow(dpy));
            XGrabKey(dpy, XKeysymToKeycode (dpy, XStringToKeysym("t")), Mod4Mask,
                     DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);
          }
          break;
        case MapRequest:
          {
            fprintf(stderr, "map request\n");
            XWindowAttributes wa;
            XMapWindow(dpy, ev.xmaprequest.window);
            if (XGetWindowAttributes(dpy, ev.xmaprequest.window, &wa))
            {
              XMoveWindow(dpy, ev.xmaprequest.window,
                          (screen_width - wa.width)/2,
                          (screen_height - wa.height)/2);
            }
            XRaiseWindow(dpy, ev.xmaprequest.window);
          }
          break;
        case ConfigureRequest:
          int req_x = ev.xconfigurerequest.x;
          int req_y = ev.xconfigurerequest.y;
          int req_width = ev.xconfigurerequest.width;
          int req_height = ev.xconfigurerequest.height;

          if (req_x + req_width >= screen_width) {
            req_width = screen_width - req_x;
          }
          if (req_y + req_height >= screen_height) {
            req_height = screen_height - req_y;
          }

          XConfigureWindow(dpy, ev.xconfigurerequest.window, ev.xconfigurerequest.value_mask, &(XWindowChanges) {
            .x            = ev.xconfigurerequest.x,
            .y            = ev.xconfigurerequest.y,
            .width        = req_width,
            .height       = req_height,
            .border_width = ev.xconfigurerequest.border_width,
            .sibling      = ev.xconfigurerequest.above,
            .stack_mode   = ev.xconfigurerequest.detail
          });

          break;
        case ClientMessage:
          if (ev.xclient.message_type == XInternAtom(dpy, "_NET_WM_STATE", False)) {
            if ((Atom)ev.xclient.data.l[1] == XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False) ||
              (Atom)ev.xclient.data.l[2] == XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False)) {
              if (ev.xclient.data.l[0] > 0) {
                XMoveResizeWindow(dpy, ev.xclient.window, 0, 0, screen_width, screen_height);
              }
            }
          }
          break;
        default:
          fprintf(stderr, "got event type: %d\n", ev.type);
          break;
      }
    }
    if (!command_running) {
      if(fork() == 0) {
        execl("/usr/bin/sh", "sh", "-c", command, (char *)NULL);
      }
      command_running = 1;
    }
  }
}
