#include "impl.h"

#include "di.h"

#include <iostream>

int main()
{
    try
    {
        auto app = di::Bindings{}
            .set<IOutputDevice, Console>()
            .set<ILogger, Logger>();

        auto scope = di::Scope{app};

        di::ServiceRef<ILogger> ref;
        ref->log("Hi");
    }
    catch (const std::exception& ex)
    {
        std::cout << "error: " << ex.what() << std::endl;
    }

    return 0;
}