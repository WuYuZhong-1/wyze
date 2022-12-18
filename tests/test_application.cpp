#include "../wyze/wyze.h"

int main(int argc, char** argv)
{
    wyze::Application app;
    if(app.init(argc, argv)) {
        app.run();
    }
    return 0;
}
