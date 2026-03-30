#pragma once

namespace component_meta
{
    struct ComponentDescriptor;
}

namespace component
{
    class Component
    {
    public:
        virtual void run() = 0;
        virtual const component_meta::ComponentDescriptor* getComponentDescriptor() const
        {
            return nullptr;
        }
        virtual ~Component(){}
    };
}