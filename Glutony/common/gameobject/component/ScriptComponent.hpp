#pragma once

#include "Component.hpp"
#include "ComponentSerialization.hpp"

#include <string>

namespace asset
{
struct DataAssetDefinition;
}

namespace component
{
class ScriptComponent : public Component
{
private:
    std::string m_scriptAssetPath;
    std::string m_dataAssetPath;
    bool m_runtimeStarted = false;

public:
    void run() override
    {
    }

    void setScriptAssetPath(const std::string& assetPath);
    const std::string& getScriptAssetPath() const
    {
        return m_scriptAssetPath;
    }

    void setDataAssetPath(const std::string& assetPath);
    const std::string& getDataAssetPath() const
    {
        return m_dataAssetPath;
    }

    asset::DataAssetDefinition* loadDataAssetDefinition() const;

    void resetRuntimeState()
    {
        m_runtimeStarted = false;
    }

    bool hasRuntimeStarted() const
    {
        return m_runtimeStarted;
    }

    void markRuntimeStarted()
    {
        m_runtimeStarted = true;
    }

    static const component_meta::ComponentDescriptor& componentDescriptor();

    const component_meta::ComponentDescriptor* getComponentDescriptor() const override
    {
        return &componentDescriptor();
    }
};
}