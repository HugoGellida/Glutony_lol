#include "PointLight.hpp"

namespace component
{
const component_meta::ComponentDescriptor& PointLight::componentDescriptor()
{
    static const component_meta::ComponentDescriptor descriptor = []() {
        component_meta::ComponentDescriptor value;
        value.typeKey = "render.point_light";
        value.displayName = "Point Light";
        value.version = 1;
        value.factory = [](GameObject* parent) -> component::Component* {
            (void)parent;
            return new PointLight();
        };
        value.fields = {
            {
                "enabled",
                "Enabled",
                component_meta::FieldKind::Bool,
                [](const component::Component& component) -> component_meta::SerializedValue {
                    return static_cast<const PointLight&>(component).isEnabled();
                },
                [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                    const bool* parsed = std::get_if<bool>(&value);
                    if (parsed == nullptr)
                        return false;

                    static_cast<PointLight&>(component).setEnabled(*parsed);
                    return true;
                }
            },
            {
                "color",
                "Color",
                component_meta::FieldKind::Vec3,
                [](const component::Component& component) -> component_meta::SerializedValue {
                    return static_cast<const PointLight&>(component).getColor();
                },
                [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                    const glm::vec3* parsed = std::get_if<glm::vec3>(&value);
                    if (parsed == nullptr)
                        return false;

                    static_cast<PointLight&>(component).setColor(*parsed);
                    return true;
                }
            },
            {
                "intensity",
                "Intensity",
                component_meta::FieldKind::Float,
                [](const component::Component& component) -> component_meta::SerializedValue {
                    return static_cast<const PointLight&>(component).getIntensity();
                },
                [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                    const float* parsed = std::get_if<float>(&value);
                    if (parsed == nullptr)
                        return false;

                    static_cast<PointLight&>(component).setIntensity(*parsed);
                    return true;
                }
            }
        };
        return value;
    }();

    static const bool registered = []() {
        component_meta::registerComponentDescriptor(descriptor);
        return true;
    }();
    (void)registered;
    return descriptor;
}
}