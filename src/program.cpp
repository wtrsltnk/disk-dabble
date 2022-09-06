#include <app.hpp>
#include <config.h>

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[])
{
    const std::vector<std::string> args(argv + 1, argv + argc);
    App app(args);

    std::cout << APP_NAME << " version " << APP_VERSION << std::endl;

    if (!app.Init())
    {
        std::cout << "Failed to initialize app" << std::endl;

        return 1;
    }

    return app.Run();
}
