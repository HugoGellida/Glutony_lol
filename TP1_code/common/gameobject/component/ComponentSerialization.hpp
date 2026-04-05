#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "Component.hpp"

namespace component_meta
{
enum class FieldKind
{
    Bool,
    Int,
    Float,
    Vec3,
    String,
    Enum,
    Asset,
};

enum class AssetReferenceKind
{
    None,
    Generic,
    Mesh,
    Shader,
    Material,
    Texture,
    Data,
    SceneScript,
    ComponentScript,
    Scene,
};

using SerializedValue = std::variant<bool, int, float, glm::vec3, std::string>;

struct EnumOption
{
    std::string value;
    std::string label;
};

struct SerializedField
{
    std::string key;
    SerializedValue value;
};

struct ComponentSnapshot
{
    std::string typeKey;
    std::uint32_t version = 1;
    std::vector<SerializedField> fields;
};

struct ComponentFieldDescriptor
{
    std::string key;
    std::string label;
    FieldKind kind = FieldKind::String;
    std::function<SerializedValue(const component::Component&)> read;
    std::function<bool(component::Component&, const SerializedValue&)> write;
    std::vector<EnumOption> enumOptions;
    AssetReferenceKind assetReferenceKind = AssetReferenceKind::None;
};

struct ComponentDescriptor
{
    std::string typeKey;
    std::string displayName;
    std::uint32_t version = 1;
    std::function<component::Component*(GameObject*)> factory;
    std::vector<ComponentFieldDescriptor> fields;
};

inline std::unordered_map<std::string, const ComponentDescriptor*>& componentDescriptorRegistry()
{
    static std::unordered_map<std::string, const ComponentDescriptor*> registry;
    return registry;
}

inline void registerComponentDescriptor(const ComponentDescriptor& descriptor)
{
    componentDescriptorRegistry()[descriptor.typeKey] = &descriptor;
}

inline const ComponentDescriptor* findComponentDescriptor(const std::string& typeKey)
{
    const auto it = componentDescriptorRegistry().find(typeKey);
    return it != componentDescriptorRegistry().end() ? it->second : nullptr;
}

inline const ComponentFieldDescriptor* findComponentFieldDescriptor(const ComponentDescriptor& descriptor, const std::string& fieldKey)
{
    for (const ComponentFieldDescriptor& field : descriptor.fields)
    {
        if (field.key == fieldKey)
            return &field;
    }

    return nullptr;
}

inline std::optional<ComponentSnapshot> trySerializeComponent(const component::Component& component)
{
    const ComponentDescriptor* descriptor = component.getComponentDescriptor();
    if (descriptor == nullptr)
        return std::nullopt;

    ComponentSnapshot snapshot;
    snapshot.typeKey = descriptor->typeKey;
    snapshot.version = descriptor->version;
    snapshot.fields.reserve(descriptor->fields.size());

    for (const ComponentFieldDescriptor& field : descriptor->fields)
    {
        if (!field.read)
            continue;

        snapshot.fields.push_back({field.key, field.read(component)});
    }

    return snapshot;
}

inline bool applyComponentSnapshot(component::Component& component, const ComponentSnapshot& snapshot)
{
    const ComponentDescriptor* descriptor = component.getComponentDescriptor();
    if (descriptor == nullptr || descriptor->typeKey != snapshot.typeKey)
        return false;

    for (const SerializedField& fieldSnapshot : snapshot.fields)
    {
        const ComponentFieldDescriptor* fieldDescriptor = findComponentFieldDescriptor(*descriptor, fieldSnapshot.key);
        if (fieldDescriptor == nullptr || !fieldDescriptor->write)
            continue;

        if (!fieldDescriptor->write(component, fieldSnapshot.value))
            return false;
    }

    return true;
}

inline std::unique_ptr<component::Component> createComponentFromSnapshot(const ComponentSnapshot& snapshot, GameObject* parent = nullptr)
{
    const ComponentDescriptor* descriptor = findComponentDescriptor(snapshot.typeKey);
    if (descriptor == nullptr || !descriptor->factory)
        return nullptr;

    std::unique_ptr<component::Component> component(descriptor->factory(parent));
    if (component)
        component->setOwner(parent);
    if (!component || !applyComponentSnapshot(*component, snapshot))
        return nullptr;

    return component;
}
}