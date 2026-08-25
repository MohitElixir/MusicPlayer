#include "MusicManager.h"
#include "UIManager.h"


using namespace std;

int main() {
    MusicManager manager;   // Member 2: core engine
    manager.refreshFromFolder("Music");

    UIManager ui(manager);  // Member 3: UI & navigation
    ui.run();

    return 0;
}
