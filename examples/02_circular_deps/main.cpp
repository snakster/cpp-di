#include "di.h"

#include <iostream>

struct Interface1 {};
struct Interface2 {};
struct Interface3 {};

struct Impl1 : Interface1
{
    di::ServiceRef<Interface2> ref2;
};

struct Impl2 : Interface2
{
    di::ServiceRef<Interface3> ref3;
};

struct Impl3 : Interface3
{
    di::ServiceRef<Interface1> ref1;
};

int main()
{
    try
    {
        // There's a cycle Impl1 -> Impl2 -> Impl3 -> Impl1 ...
        auto app = di::Bindings{}
            .service<Interface1, Impl1>()
            .service<Interface2, Impl2>()
            .service<Interface3, Impl3>();

        auto scope = di::Scope{app};

        // This will throw
        di::ServiceRef<Interface1> ref;
    }
    catch (const std::exception& ex)
    {
        std::cout << "error: " << ex.what() << std::endl;
    }

    return 0;
}