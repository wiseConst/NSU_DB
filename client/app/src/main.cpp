#include <Application.hpp>

int main(int argc, char** argv)
{
    using namespace nsudb;

    auto app = std::make_unique<Application>();
    app->Run();

    return 0;
}
