#ifndef SDLPANEL_H
#define SDLPANEL_H

#include <wx/wx.h>
#include <SDL.h>

class Video;
class Cpu;

/**
 * Panel zur Darstellung des Emulator-Bildschirms
 */
class SDLPanel : public wxPanel
{
    DECLARE_CLASS(SDLPanel)
    DECLARE_EVENT_TABLE()

public:
    SDLPanel(wxWindow *parent);
    ~SDLPanel();

    void load(const char* path);
    void reset();

private:
    // Event-Handler
    void onPaint(wxPaintEvent &event);
    void onEraseBackground(wxEraseEvent &event);
    void onTimer(wxTimerEvent &event);
    void onKeyDown(wxKeyEvent &event);
    void onKeyUp(wxKeyEvent &event);

    // SDL-Bildschirm
    SDL_Surface *screen;
    
    // Emulator-Komponenten
    Video *video;
    Cpu *cpu;
    
    // Timer für regelmäßige Aktualisierung
    wxTimer *timer;
    
    // Joystick-Register
    Uint16 *joystickr;

    enum
    {
        ID_PANEL = 1000
    };
};

#endif // SDLPANEL_H
