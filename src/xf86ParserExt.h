#define xf86confmalloc malloc
#define xf86confrealloc realloc
#define xf86confcalloc calloc
extern void xf86freeFiles (XF86ConfFilesPtr p);
extern void xf86freeModules (XF86ConfModulePtr ptr);
extern void xf86freeFlags (XF86ConfFlagsPtr flags);
extern void xf86freeVideoPortList (XF86ConfVideoPortPtr ptr);
extern void xf86freeModeLineList (XF86ConfModeLinePtr ptr);
extern void xf86freeModeList (XF86ModePtr ptr);
extern void xf86optionListFree (XF86OptionPtr opt);
extern void xf86freeAdaptorLinkList (XF86ConfAdaptorLinkPtr ptr);
extern void xf86freeDisplayList (XF86ConfDisplayPtr ptr);
extern void xf86freeInputrefList (XF86ConfInputrefPtr ptr);
extern void xf86freeAdjacencyList (XF86ConfAdjacencyPtr ptr);
extern void xf86freeDRI (XF86ConfDRIPtr ptr);
extern void xf86freeVendorSubList (XF86ConfVendSubPtr ptr);

/* My versions: */  
extern void xf86freeInput (XF86ConfInputPtr ptr);
extern void xf86freeOption (XF86OptionPtr opt);
extern void xf86freeLoad (XF86LoadPtr ptr);
extern void xf86freeVideoAdaptor (XF86ConfVideoAdaptorPtr ptr);
extern void xf86freeVideoPort (XF86ConfVideoPortPtr ptr);
extern void xf86freeModes (XF86ConfModesPtr ptr);
extern void xf86freeModeLine (XF86ConfModeLinePtr ptr);
extern void xf86freeMonitor (XF86ConfMonitorPtr ptr);
extern void xf86freeModesLink (XF86ConfModesLinkPtr ptr);
extern void xf86freeDevice (XF86ConfDevicePtr ptr);
extern void xf86freeAdaptorLink (XF86ConfAdaptorLinkPtr ptr);
extern void xf86freeMode  (XF86ModePtr ptr);
extern void xf86freeDisplay (XF86ConfDisplayPtr ptr);
extern void xf86freeScreen (XF86ConfScreenPtr ptr);
extern void xf86freeAdjacency (XF86ConfAdjacencyPtr ptr);
extern void xf86freeInactive (XF86ConfInactivePtr ptr);
extern void xf86freeInputref (XF86ConfInputrefPtr ptr);
extern void xf86freeLayout (XF86ConfLayoutPtr ptr);
extern void xf86freeBuffers (XF86ConfBuffersPtr ptr);
extern void xf86freeVendSub (XF86ConfVendSubPtr ptr);
extern void xf86freeVendor (XF86ConfVendorPtr p);
