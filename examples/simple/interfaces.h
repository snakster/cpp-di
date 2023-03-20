#include <string_view>

class IOutputDevice
{
public:
    virtual void print(std::string_view msg) = 0;
};

class ILogger
{
public:
    virtual void log(std::string_view msg) = 0;
};