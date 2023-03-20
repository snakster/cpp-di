#include "interfaces.h"

#include "di.h"

#include <iostream>

class Console : public IOutputDevice
{
public:
    void Console::print(std::string_view msg) override
    {
        std::cout << msg << std::endl;
    }
};

class Logger : public ILogger
{
public:
    void log(std::string_view msg) override
    {
        output_->print(msg);
    }

private:
    di::ServiceRef<IOutputDevice> output_;
};