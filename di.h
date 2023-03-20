#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>
#include <typeindex>
#include <tuple>


namespace di {

class Bindings;
class Scope;

}// namespace di


namespace di::detail {

class BindingsState;
class ScopeState;

/// Helper class used to keep track of constructor call stack in the current scope.
/// If the same instance occurs more than once there's a cycle.
class CycleChecker
{
public:
    class Guard
    {
    public:
        Guard(CycleChecker& parent, std::type_index ctorType) :
            parent_{&parent},
            ctorType_{ctorType}
        {
            std::scoped_lock lock(parent.mtx_);

            if (auto e = parent.activeCtors_.find(ctorType); e != parent.activeCtors_.end())
                throw std::runtime_error("circular dependency"); 

            parent_->activeCtors_.emplace(ctorType, true);
        }

        ~Guard()
        {
            if (parent_)
            {
                std::scoped_lock lock(parent_->mtx_);
                parent_->activeCtors_.erase(ctorType_);
            }       
        }

    private:
        CycleChecker* parent_;
        std::type_index ctorType_;
    };

    Guard makeGuard(std::type_index ctorType)
    {
        return Guard(*this, ctorType);
    }

private:
    std::mutex mtx_;
    // Using map to avoid set include for this single use.
    std::unordered_map<std::type_index, bool> activeCtors_;
};


struct ImplData
{
    std::function<std::shared_ptr<void>()> factory;
};


class BindingsState
{
public:
    template <typename TInterface, typename TImpl, typename ... TArgs>
    void setService(TArgs&& ... args)
    {
        auto interfaceType = std::type_index(typeid(TInterface));
        auto implType = std::type_index(typeid(TImpl));

        ImplData impl;
        impl.factory = [storedArgs = std::make_tuple(std::forward<TArgs>(args) ...)] () -> std::shared_ptr<void> {
            return std::apply([](const auto& ... args){
                return std::make_shared<TImpl>(args ...);
            }, storedArgs);
        };

        interfaceMap_[interfaceType][implType] = std::move(impl);
    }

    void registerAtScope(ScopeState& scope) const;

    static const BindingsState& fromBindings(const Bindings&);
    static BindingsState& fromBindings(Bindings&);

private:
    using ImplMap = std::unordered_map<std::type_index, ImplData>;
    using InterfaceMap = std::unordered_map<std::type_index, ImplMap>;
    
    // InterfaceType -> ImplType -> ImplData
    InterfaceMap interfaceMap_;
};


template <typename Tag, typename U>
struct TaggedType {};


class ScopeState
{
public:
    template <typename TInterface, typename Tag>
    std::shared_ptr<TInterface> getService()
    {
        auto interfaceType = std::type_index{typeid(TInterface)};

        auto serviceEntry = serviceImpls_.find(interfaceType);
        if (serviceEntry == serviceImpls_.end())
            throw std::runtime_error("service interface is not bound");

        // Create non-cached instance.
        if constexpr (std::is_same_v<Tag, tags::Unique>)
        {
            return serviceEntry->second.factory();
        }
        
        auto instanceType = std::type_index{typeid(TaggedType<Tag, TInterface>)};
        
        // Check for tagged instance (read lock).
        {
            std::shared_lock lock(mtx_);
            auto e = serviceInstances_.find(instanceType);
            if (e != serviceInstances_.end())
                return std::static_pointer_cast<TInterface>(e->second);
        }

        auto cycleGuard = cycleChecker_.makeGuard(instanceType);

        // Create instance.
        std::shared_ptr<void> instance = serviceEntry->second.factory();

        // Check again, then set tagged instance (write lock).
        {
            std::unique_lock lock(mtx_);
            auto e = serviceInstances_.find(instanceType);
            if (e != serviceInstances_.end())
                return std::static_pointer_cast<TInterface>(e->second);

            serviceInstances_.emplace(instanceType, instance);
            return std::static_pointer_cast<TInterface>(instance);
        }
    }

    void setServiceImpl(std::type_index interfaceType, const ImplData& impl);

    static const ScopeState& fromScope(const Scope&);
    static ScopeState& fromScope(Scope&);

private:
    std::shared_mutex mtx_;

    std::unordered_map<std::type_index, std::shared_ptr<void>> serviceInstances_;
    std::unordered_map<std::type_index, ImplData> serviceImpls_;
    CycleChecker cycleChecker_;
};


class ScopeStack
{
public:
    void push(ScopeState& scope);

    void pop(ScopeState& scope);

    ScopeState& top();

private:
    std::vector<ScopeState*> scopes_;
};


class ScopeGuard
{
public:
    ScopeGuard(ScopeStack& stack, ScopeState& scope);

    ~ScopeGuard();

private:
    ScopeStack* stack_;
    ScopeState* scope_;
};

extern ScopeStack globalScopeStack;


template <typename TInterface, typename Tag>
std::shared_ptr<TInterface> getService()
{
    auto& scope = globalScopeStack.top();
    return scope.getService<TInterface, Tag>();
}

} // namespace di::detail


namespace di {

class Bindings
{
public:
    template <typename TInterface, typename TImpl, typename ... TArgs>
    Bindings& service(TArgs&& ... args)
    {
        state_.setService<TInterface, TImpl>(std::forward<TArgs>(args) ...);
        return *this;
    }

private:
    detail::BindingsState state_;

    friend class detail::BindingsState;
};


class Scope
{
public:
    template <typename ... TBindings>
    explicit Scope(const TBindings& ... bindings) :
        guard_{detail::globalScopeStack, state_}
    {
        using detail::BindingsState;
        (BindingsState::fromBindings(bindings).registerAtScope(state_), ...);
    }

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

    Scope(Scope&&) = delete;
    Scope& operator=(Scope&&) = delete;

    void validate();

private:
    detail::ScopeState state_;
    detail::ScopeGuard guard_;

    friend class detail::ScopeState;
};


namespace tags
{
    struct Unique {};
    struct Shared {};
}

template <typename TInterface, typename Tag = tags::Shared>
class ServiceRef
{
public:
    ServiceRef() :
        ptr_{detail::getService<TInterface, Tag>()}
    {}

    ServiceRef(const ServiceRef&) = default;
    ServiceRef& operator=(const ServiceRef&) = default;

    ServiceRef(ServiceRef&&) = default;
    ServiceRef& operator=(ServiceRef&&) = default;

    const TInterface& operator*() const { return ptr_.operator*(); }

    TInterface& operator*() { return ptr_.operator*(); }

    const TInterface* operator->() const { return ptr_.operator->(); }

    TInterface* operator->() { return ptr_.operator->(); }

    operator std::shared_ptr<TInterface>() const { return ptr_; }

private:
    std::shared_ptr<TInterface> ptr_;
};

}// namespace di
