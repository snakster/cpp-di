# cpp-di
A minimalistic C++17 dependency injection framework.

## Motivation

A key characteristic of software is that it needs to be changed over time as requirements evolve.
By de-coupling different parts of a program from each other, we can reduce the effort that comes with such changes. The resuling architecture this often looks like this:
* The program is divided into components.
* Component interface and implementation are separate.
* Dependencies between components are against interfaces, not implementations.

In this context, [dependency injection](https://en.wikipedia.org/wiki/Dependency_injection) solves the problem of constructing components and assembling them in a flexible and convenient way.

## Usage

#### 1. Setup
Add `di.h` and `di.cpp` to your project and
```C++
#include "di.h"
```

#### 2. Create interfaces

```C++
class Greeter
{
  virtual void greet() = 0;
};
```
```C++
class Printer
{
  virtual void print(std::string_view text) = 0;
};
```

#### 3. Create implementations

```C++
class GreeterImpl : public Greeter
{
  void greet() override
  {
    printer->print("Hello!");
  }
  
  di::ServiceRef<Printer> printer;
};
```
```C++
class ConsolePrinterImpl : public Printer
{
  void print(std::string_view text) override
  {
    std::cout << text << std::endl;
  }
};
```
```C++
class FilePrinterImpl : public Printer
{
  FilePrinterImpl(std::string_view fn) { f.open(std::string(fn)); }
  ~FilePrinterImpl() { f.close(); }
  
  void print(std::string_view text) override
  {
    f << text << std::endl;
  }
  
  std::ofstream f;
};
```

#### 4. Define bindings
```C++
auto consoleApp = di::Bindings{}
  .service<Greeter, GreeterImpl>()
  .service<Printer, ConsolePrinterImpl>();
```
```C++
auto loggingApp = di::Bindings{}
  .service<Greeter, GreeterImpl>()
  .service<Printer, FilePrinterImpl>("log.txt");
```

#### 5. Create instances within a scope
```C++
{
  auto scope = di::Scope{consoleApp};
  di::ServiceRef<Greeter> greeter;
  greeter->greet(); // Greets to console
}
```
```C++
{
  auto scope = di::Scope{loggingApp};
  di::ServiceRef<Greeter> greeter;
  greeter->greet(); // Greets to log.txt
}
```

## Status
This framework is work-in-progress and should not be used in production yet.
