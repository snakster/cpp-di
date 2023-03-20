#include "di.h"

namespace di::detail {

const BindingsState& BindingsState::fromBindings(const Bindings& bindings)
{
    return bindings.state_;
}

BindingsState& BindingsState::fromBindings(Bindings& bindings)
{
    return bindings.state_;
}

const ScopeState& ScopeState::fromScope(const Scope& scope)
{
    return scope.state_;
}

ScopeState& ScopeState::fromScope(Scope& scope)
{
    return scope.state_;
}

void BindingsState::registerAtScope(ScopeState& scope) const
{
    for (const auto& [interfaceType, implMap] : interfaceMap_)
        for (const auto& [implType, implData] : implMap)
            scope.setServiceImpl(interfaceType, implData);
}

void ScopeState::setServiceImpl(std::type_index interfaceType, const ImplData& impl)
{
    serviceImpls_[interfaceType] = impl;
}

void ScopeStack::push(ScopeState& scope)
{
    scopes_.push_back(&scope);
}

void ScopeStack::pop(ScopeState& scope)
{
    if (scopes_.back() != &scope)
        throw std::runtime_error("detected mismatched dependency scope stack");

    scopes_.pop_back();
}

ScopeState& ScopeStack::top()
{
    if (scopes_.empty())
        throw std::runtime_error("no active dependency scope");

    return *scopes_.back();
}

ScopeGuard::ScopeGuard(ScopeStack& stack, ScopeState& scope) :
    stack_{&stack},
    scope_{&scope}
{
    stack.push(scope);
}

ScopeGuard::~ScopeGuard()
{
    if (stack_ != nullptr && scope_ != nullptr)
        stack_->pop(*scope_);
}

ScopeStack globalScopeStack;

}