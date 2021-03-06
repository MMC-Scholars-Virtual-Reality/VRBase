// This software is under partial ownership by The Ohio State University, for it
// is a product of student employees. For official policy, see
// https://tco.osu.edu/sites/default/files/pdfs/IP-Policy.pdf or contact The
// Ohio State University's Office of Legal Affairs.

#ifndef GLOBALS_H
#define GLOBALS_H

#include "NLogger.h"
#include "predefs.h"
#include <chrono>

using namespace std::chrono;

class CGlobalVars {
public:
    ftime curtime;
    ftime frametime;
    bool  ineditor     = true;  // true if in Unreal editor or in Play In Editor
    bool  worldcreated = false; // true if the world has already been created and
                                // BeginPlays have started.

    void update();
    void reset();
    void checkReset();
    void markReset();

private:
    steady_clock::time_point m_tPrevious;
    bool                     m_bReset = true;
};

extern CGlobalVars* g_pGlobals;
extern UWorld*      g_pWorld;

inline bool IsCookedBuild() { return !WITH_EDITOR; }

#endif // GLOBALS_H