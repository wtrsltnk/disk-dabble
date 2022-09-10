#ifndef SERVICEPROVIDER_H
#define SERVICEPROVIDER_H

#include <functional>
#include <map>
#include <typeindex>
#include <typeinfo>

typedef void *GenericServicePtr;

class ServiceProvider
{
public:
    typedef std::function<GenericServicePtr(ServiceProvider &sp)> ServiceFactoryFunc;
    ServiceProvider();

    template <class T>
    void Add(
        ServiceFactoryFunc factory)
    {
        auto found = _services.find(typeid(T));

        if (found != _services.end())
        {
            _services.erase(found);
        }

        _services.insert(std::make_pair((std::type_index) typeid(T), factory));
    }

    template <class T>
    T Resolve()
    {
        auto found = _services.find(typeid(T));

        if (found != _services.end())
        {
            return (T)found->second(*this);
        }

        return nullptr;
    }

private:
    std::map<std::type_index, ServiceFactoryFunc> _services;
};

#endif // SERVICEPROVIDER_H
