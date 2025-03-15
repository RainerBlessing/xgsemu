
/*******************************************************************************
// Global Declarations
*******************************************************************************/

enum {
    ID_FRAME = 10000,
    ID_PANEL,
    IDM_FILE_EXIT,
    IDM_HELP_ABOUT,
    IDB_KBD_UP,
    IDB_KBD_LEFT,
    IDB_KBD_RIGHT,
    IDB_KBD_PAUSE,
    IDB_KBD_DOWN,
    IDB_KBD_FIRE
};

#define XGSE_VERSION "0.06"

#ifndef _XGSEMU_H_
#define _XGSEMU_H_

/**
 * @short Application Main Window
 * @author Rainer Blessing <rainer@theblessing.net>
 * @version 0.06
 */

class
XGSEmuapp : public wxApp
{
public:
	virtual bool OnInit();
};

#endif // _XGSEMU_H_
