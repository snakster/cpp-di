#include <string_view>

class Greeter
{
public:
    virtual void greet() = 0;
};

class Printer
{
public:
    virtual void print(std::string_view msg) = 0;
};

