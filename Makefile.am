SUBDIRS = data

INCLUDES = $(GCC_FLAGS) $(MB_CFLAGS) -DPREFIX=\"@prefix@\"

bin_PROGRAMS = mb-applet-startup-monitor
mb_applet_startup_monitor_SOURCES = mbmonitor.c
mb_applet_startup_monitor_LDADD   = $(MB_LIBS)
