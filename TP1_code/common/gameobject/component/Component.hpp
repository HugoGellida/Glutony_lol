#pragma once

class GameObject;

namespace component_meta
{
    struct ComponentDescriptor;
}

namespace component
{
    class Component
    {
    private:
        GameObject* m_owner = nullptr;

    public:
        unsigned int ownerCount = 0;

        virtual void run() = 0;
        void setOwner(GameObject* owner)
        {
            m_owner = owner;
        }

        GameObject* getOwner() const
        {
            return m_owner;
        }

        virtual const component_meta::ComponentDescriptor* getComponentDescriptor() const
        {
            return nullptr;
        }
        virtual ~Component(){}
    };
}