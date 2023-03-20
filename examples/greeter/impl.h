#include "interfaces.h"

#include "di.h"

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

class ConsolePrinterImpl : public Printer
{
public:
    void print(std::string_view text) override
    {
        std::cout << text << std::endl;
    }
};


class FilePrinterImpl : public Printer
{
public:
    explicit FilePrinterImpl(std::string_view fn) { f.open(std::string(fn)); }

    ~FilePrinterImpl() { f.close(); }

    void print(std::string_view text) override
    {
        f << text << std::endl;
    }

private:
    std::ofstream f;
};


class GreeterImpl : public Greeter
{
public:
    void greet() override
    {
        printer->print("Hello!");
    }

private:
    di::ServiceRef<Printer> printer;
};