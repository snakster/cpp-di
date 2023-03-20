#include "impl.h"

#include "di.h"

#include <iostream>

int main()
{
    try
    {
        // These bindings use ConsolePrinterImpl
        auto consoleApp = di::Bindings{}
            .service<Greeter, GreeterImpl>()
            .service<Printer, ConsolePrinterImpl>();

        // These binding use FilePrinterImpl
        auto loggingApp = di::Bindings{}
            .service<Greeter, GreeterImpl>()
            .service<Printer, FilePrinterImpl>("log.txt");

        {
            // This scope uses consoleApp bindings
            auto scope = di::Scope{consoleApp};

            di::ServiceRef<Greeter> greeter;

            // greets to console
            greeter->greet(); 
        }

        {
            // This scope uses loggingApp bindings
            auto scope = di::Scope{loggingApp};

            di::ServiceRef<Greeter> greeter;

            // greets to log.txt
            greeter->greet();
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << "error: " << ex.what() << std::endl;
    }

    return 0;
}