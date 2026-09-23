#include "main_menu.h"

#include "../control_alias.h"
#include "../input.h"
#include "../txt_data.h"
#include "txt_defines.h"

MainMenu::MainMenu() {
    items_.AddEntry(txt_data[TXT_START_GAME]);
    items_.AddEntry("OPTIONS");
    items_.AddEntry(txt_data[TXT_EXIT]);
}

int MainMenu::ProcessEvent() {
	in.menuBackPressed(); // No back action on the root menu; keep the B edge in sync.
    const int move = in.menuVerticalMove();
    if (move < 0) {
        items_.MoveUp();
    } else if (move > 0) {
        items_.MoveDown();
    }
    if (in.menuConfirmActionPressed()) {
        switch (items_.focused()) {
            case 0:
                return MenuType::Start;
            case 1:
                return MenuType::Options;
            case 2:
                return MenuType::Exit;
        }
    }
    return MenuType::Main;
}
