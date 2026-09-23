#include "start_menu.h"

#include "../control_alias.h"
#include "../input.h"
#include "../txt_data.h"
#include "txt_defines.h"

StartMenu::StartMenu() {
    items_.AddEntry(txt_data[TXT_START_GAME1]);
    items_.AddEntry(txt_data[TXT_START_GAME2]);
    items_.AddEntry(txt_data[TXT_RETURN]);
}

int StartMenu::ProcessEvent() {
	if (in.menuBackPressed()) return MenuType::Main;
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
                return MenuType::Game_2;
            case 2:
                return MenuType::Main;
        }
    }
    return MenuType::Start;
}
