#include "TriangleApp.hpp"
#include <stdexcept>
#include <iostream>

int main(int argc, char** argv)
{
    TriangleApp app;

    try
    {
        app.Run();
    }
    catch (const std::exception e)
    {
        std::cerr << "\n" << std::endl;
        std::cerr << "--------------------------------------------------" << std::endl;
        std::cerr << "An error was thrown while running the application\n" << std::endl;
        std::cerr << "[Vulkan]: " << e.what() << "\n" << std::endl;
        std::cerr << "[SDL]: " << SDL_GetError() << "\n" << std::endl;
        std::cerr << "--------------------------------------------------" << std::endl;
        std::cerr << "\nFailure. Press any key to exit..." << std::endl;


        std::getchar();

        return EXIT_FAILURE;
    }

    std::cout << "Application exited succesfully!" << std::endl;
    return EXIT_SUCCESS;
}