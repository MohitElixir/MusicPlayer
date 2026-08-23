#include "MusicManager.h"
#include "UIManager.h"

int main() {
    MusicManager manager;   // Member 2: core engine
    manager.refreshFromFolder("E:\\MusicPlayer\\MusicPlayer\\Music");

    UIManager ui(manager);  // Member 3: UI & navigation
    ui.run();

    return 0;
}
