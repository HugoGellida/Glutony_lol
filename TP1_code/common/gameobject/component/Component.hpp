#pragma once

namespace component
{
    class Component
    {
    public:
        virtual void run() = 0;
        virtual ~Component(){}
    };
}