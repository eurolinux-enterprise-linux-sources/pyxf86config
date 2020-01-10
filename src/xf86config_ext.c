#include <glib.h>
#include <stdlib.h>
#include <xf86Parser.h>
#include "xf86ParserExt.h"

#define xf86confmalloc malloc
#define xf86confrealloc realloc
#define xf86confcalloc calloc
#define xf86conffree free
#define TestFree(a) if (a) { xf86conffree (a); a = NULL; }

extern void xf86freeFiles (XF86ConfFilesPtr p);

void
xf86freeInput (XF86ConfInputPtr ptr)
{
  TestFree (ptr->inp_identifier);
  TestFree (ptr->inp_driver);
  TestFree (ptr->inp_comment);
  xf86optionListFree (ptr->inp_option_lst);
  
  xf86conffree (ptr);
}

void
xf86freeOption (XF86OptionPtr opt)
{
  TestFree (opt->opt_name);
  TestFree (opt->opt_val);
  TestFree (opt->opt_comment);
  xf86conffree (opt);
}

void
xf86freeLoad (XF86LoadPtr lptr)
{
  TestFree (lptr->load_name);
  TestFree (lptr->load_comment);
  xf86conffree (lptr);
}

void
xf86freeVideoAdaptor (XF86ConfVideoAdaptorPtr ptr)
{
  TestFree (ptr->va_identifier);
  TestFree (ptr->va_vendor);
  TestFree (ptr->va_board);
  TestFree (ptr->va_busid);
  TestFree (ptr->va_driver);
  TestFree (ptr->va_fwdref);
  TestFree (ptr->va_comment);
  xf86optionListFree (ptr->va_option_lst);
  xf86conffree (ptr);
}

void
xf86freeVideoPort (XF86ConfVideoPortPtr ptr)
{
  TestFree (ptr->vp_identifier);
  TestFree (ptr->vp_comment);
  xf86optionListFree (ptr->vp_option_lst);
  xf86conffree (ptr);
}

void
xf86freeModes (XF86ConfModesPtr ptr)
{
  TestFree (ptr->modes_identifier);
  TestFree (ptr->modes_comment);
  xf86conffree (ptr);
}

void
xf86freeModeLine (XF86ConfModeLinePtr ptr)
{
  TestFree (ptr->ml_identifier);
  TestFree (ptr->ml_comment);
  xf86conffree (ptr);
}


void
xf86freeMonitor (XF86ConfMonitorPtr ptr)
{
  TestFree (ptr->mon_identifier);
  TestFree (ptr->mon_vendor);
  TestFree (ptr->mon_modelname);
  TestFree (ptr->mon_comment);
  xf86optionListFree (ptr->mon_option_lst);
  xf86conffree (ptr);
}

void
xf86freeModesLink (XF86ConfModesLinkPtr ptr)
{
  TestFree (ptr->ml_modes_str);
}

void
xf86freeDevice (XF86ConfDevicePtr ptr)
{
  TestFree (ptr->dev_identifier);
  TestFree (ptr->dev_vendor);
  TestFree (ptr->dev_board);
  TestFree (ptr->dev_chipset);
  TestFree (ptr->dev_card);
  TestFree (ptr->dev_driver);
  TestFree (ptr->dev_ramdac);
  TestFree (ptr->dev_clockchip);
  TestFree (ptr->dev_comment);
  xf86optionListFree (ptr->dev_option_lst);
  
  xf86conffree (ptr);
}

void
xf86freeAdaptorLink (XF86ConfAdaptorLinkPtr ptr)
{
  TestFree (ptr->al_adaptor_str);
  xf86conffree (ptr);
}

void
xf86freeMode (XF86ModePtr ptr)
{
  TestFree (ptr->mode_name);
  xf86conffree (ptr);
}

void
xf86freeDisplay (XF86ConfDisplayPtr ptr)
{
  xf86freeModeList (ptr->disp_mode_lst);
  xf86optionListFree (ptr->disp_option_lst);
  xf86conffree (ptr);
}

void
xf86freeScreen (XF86ConfScreenPtr ptr)
{
  TestFree (ptr->scrn_identifier);
  TestFree (ptr->scrn_monitor_str);
  TestFree (ptr->scrn_device_str);
  TestFree (ptr->scrn_comment);
  xf86optionListFree (ptr->scrn_option_lst);
  xf86freeAdaptorLinkList (ptr->scrn_adaptor_lst);
  xf86freeDisplayList (ptr->scrn_display_lst);
  xf86conffree (ptr);
}

void
xf86freeAdjacency (XF86ConfAdjacencyPtr ptr)
{
  TestFree (ptr->adj_screen_str);
  TestFree (ptr->adj_top_str);
  TestFree (ptr->adj_bottom_str);
  TestFree (ptr->adj_left_str);
  TestFree (ptr->adj_right_str);
  
  xf86conffree (ptr);
}

void
xf86freeInactive (XF86ConfInactivePtr ptr)
{
  TestFree (ptr->inactive_device_str);

  xf86conffree (ptr);
}

void
xf86freeInputref (XF86ConfInputrefPtr ptr)
{
  TestFree (ptr->iref_inputdev_str);
  xf86optionListFree (ptr->iref_option_lst);
  xf86conffree (ptr);
}

void
xf86freeLayout (XF86ConfLayoutPtr ptr)
{
  TestFree (ptr->lay_identifier);
  TestFree (ptr->lay_comment);
  xf86conffree (ptr);
}

void
xf86freeBuffers (XF86ConfBuffersPtr ptr)
{
  TestFree (ptr->buf_flags);
  TestFree (ptr->buf_comment);
  xf86conffree (ptr);
}

void
xf86freeVendSub (XF86ConfVendSubPtr ptr)
{
  TestFree (ptr->vs_identifier);
  TestFree (ptr->vs_name);
  TestFree (ptr->vs_comment);
  xf86optionListFree (ptr->vs_option_lst);
  xf86conffree (ptr);
}

void
xf86freeVendor (XF86ConfVendorPtr p)
{
  xf86freeVendorSubList (p->vnd_sub_lst);
  TestFree (p->vnd_identifier);
  TestFree (p->vnd_comment);
  xf86optionListFree (p->vnd_option_lst);
  xf86conffree (p);
}

void
ErrorF(const char *format, ...)
{
  gchar *buffer;
  va_list args;

  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);

  g_print ("%s", buffer);
}

void
VErrorF(const char *format, va_list args)
{
  gchar *buffer;

  buffer = g_strdup_vprintf (format, args);

  g_print ("%s", buffer);
}
