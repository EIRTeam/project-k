#ifndef GAME_DEBUGGER_H
#define GAME_DEBUGGER_H

#include "core/object/object.h"
#include "scene/main/node.h"
class GameDebugger : public Node {
    GDCLASS(GameDebugger, Node);
    void _draw_ui();

    void _notification(int p_what);
};

#endif // GAME_DEBUGGER_H
