#include "pause_menu.h"

#include "../control_alias.h"
#include "../input.h"
#include "../txt_data.h"
#include "txt_defines.h"

PauseMenu::PauseMenu() {
    items_.AddEntry(txt_data[TXT_RESUME]);
    items_.AddEntry(txt_data[TXT_OPTIONS]);
    items_.AddEntry(txt_data[TXT_EXIT]);
}
int PauseMenu::ProcessEvent() {
	if (in.menuBackPressed()) return MenuType::Game_1;
    const int move = in.menuVerticalMove();
    if (move < 0) {
        items_.MoveUp();
    } else if (move > 0) {
        items_.MoveDown();
    }
    if (in.menuConfirmActionPressed()) {
        switch (items_.focused()) {
            case 0:
                return MenuType::Game_1;
            case 1:
                return MenuType::Options;
            case 2:
                return MenuType::Exit;
        }
        return MenuType::Main;
    }
    return MenuType::Main;
}
