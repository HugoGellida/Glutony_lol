#pragma once

#include <common/ui/EditorUiCommon.hpp>

#include <glm/glm.hpp>

#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

class Camera;
class GameObject;
class Scene;
namespace component
{
    class Component;
}
namespace physics
{
    class SphereCollider;
    class BoxCollider;
    class PlaneCollider;
}

namespace editor_gizmo
{
enum class TargetKind
{
    None,
    Transform,
    Component,
};

enum class TransformGizmoMode
{
    Move,
    Rotate,
    Scale,
};

struct ActiveTarget
{
    TargetKind kind = TargetKind::None;
    int gameObjectId = -1;
    size_t componentIndex = 0;
    TransformGizmoMode transformMode = TransformGizmoMode::Move;

    static ActiveTarget none();
    static ActiveTarget transform(int gameObjectId, TransformGizmoMode mode = TransformGizmoMode::Move);
    static ActiveTarget component(int gameObjectId, size_t componentIndex);

    bool isNone() const;

    friend bool operator==(const ActiveTarget& lhs, const ActiveTarget& rhs)
    {
        return lhs.kind == rhs.kind
            && lhs.gameObjectId == rhs.gameObjectId
            && lhs.componentIndex == rhs.componentIndex
            && lhs.transformMode == rhs.transformMode;
    }

    friend bool operator!=(const ActiveTarget& lhs, const ActiveTarget& rhs)
    {
        return !(lhs == rhs);
    }
};

struct FrameContext
{
    const Scene* scene = nullptr;
    const Camera* camera = nullptr;
    UiRect viewportRect;
    glm::vec3 rayOrigin{0.0f, 0.0f, 0.0f};
    glm::vec3 rayDirection{0.0f, 0.0f, -1.0f};
    float deltaTime = 0.0f;
    bool interactionEnabled = false;
};

struct HitResult
{
    bool hit = false;
    float distance = std::numeric_limits<float>::max();
    int handleId = -1;
};

struct InteractionState
{
    bool dragging = false;
    size_t gizmoIndex = std::numeric_limits<size_t>::max();
    int handleId = -1;
    glm::vec3 dragAxisWorld{0.0f, 0.0f, 0.0f};
    glm::vec3 dragPlanePointWorld{0.0f, 0.0f, 0.0f};
    glm::vec3 dragPlaneNormalWorld{0.0f, 0.0f, 1.0f};
    glm::vec3 dragStartObjectWorldPosition{0.0f, 0.0f, 0.0f};
    glm::vec3 dragStartObjectLocalScale{1.0f, 1.0f, 1.0f};
    glm::vec3 dragStartHitPointWorld{0.0f, 0.0f, 0.0f};
    float dragReferenceScalar = 1.0f;
};

class Gizmo
{
public:
    virtual ~Gizmo() = default;

    virtual HitResult hitTest(const FrameContext& context) const = 0;
    virtual bool beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const = 0;
    virtual bool updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const = 0;
    virtual void endInteraction(InteractionState& state) const = 0;
    virtual void render(const FrameContext& context, const InteractionState* interactionState) const = 0;
};

class Collector
{
public:
    void clear();
    bool empty() const;
    const std::vector<std::unique_ptr<Gizmo>>& gizmos() const;

    template <typename GizmoType, typename... Args>
    GizmoType& emplace(Args&&... args)
    {
        std::unique_ptr<GizmoType> gizmo = std::make_unique<GizmoType>(std::forward<Args>(args)...);
        GizmoType& reference = *gizmo;
        m_gizmos.push_back(std::move(gizmo));
        return reference;
    }

private:
    std::vector<std::unique_ptr<Gizmo>> m_gizmos;
};

void collectBuiltInComponentGizmos(const component::Component& component, Collector& collector);

class TransformMoveGizmo : public Gizmo
{
public:
    explicit TransformMoveGizmo(GameObject& target);

    HitResult hitTest(const FrameContext& context) const override;
    bool beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const override;
    bool updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const override;
    void endInteraction(InteractionState& state) const override;
    void render(const FrameContext& context, const InteractionState* interactionState) const override;

private:
    GameObject* m_target = nullptr;
};

class TransformRotateGizmo : public Gizmo
{
public:
    explicit TransformRotateGizmo(GameObject& target);

    HitResult hitTest(const FrameContext& context) const override;
    bool beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const override;
    bool updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const override;
    void endInteraction(InteractionState& state) const override;
    void render(const FrameContext& context, const InteractionState* interactionState) const override;

private:
    GameObject* m_target = nullptr;
};

class TransformScaleGizmo : public Gizmo
{
public:
    explicit TransformScaleGizmo(GameObject& target);

    HitResult hitTest(const FrameContext& context) const override;
    bool beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const override;
    bool updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const override;
    void endInteraction(InteractionState& state) const override;
    void render(const FrameContext& context, const InteractionState* interactionState) const override;

private:
    GameObject* m_target = nullptr;
};

class SphereColliderGizmo : public Gizmo
{
public:
    explicit SphereColliderGizmo(const physics::SphereCollider& collider);

    HitResult hitTest(const FrameContext& context) const override;
    bool beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const override;
    bool updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const override;
    void endInteraction(InteractionState& state) const override;
    void render(const FrameContext& context, const InteractionState* interactionState) const override;

private:
    const physics::SphereCollider* m_collider = nullptr;
};

class BoxColliderGizmo : public Gizmo
{
public:
    explicit BoxColliderGizmo(const physics::BoxCollider& collider);

    HitResult hitTest(const FrameContext& context) const override;
    bool beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const override;
    bool updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const override;
    void endInteraction(InteractionState& state) const override;
    void render(const FrameContext& context, const InteractionState* interactionState) const override;

private:
    const physics::BoxCollider* m_collider = nullptr;
};

class PlaneColliderGizmo : public Gizmo
{
public:
    explicit PlaneColliderGizmo(const physics::PlaneCollider& collider);

    HitResult hitTest(const FrameContext& context) const override;
    bool beginInteraction(const FrameContext& context, const HitResult& hitResult, InteractionState& state, GameObject& target) const override;
    bool updateInteraction(const FrameContext& context, InteractionState& state, GameObject& target) const override;
    void endInteraction(InteractionState& state) const override;
    void render(const FrameContext& context, const InteractionState* interactionState) const override;

private:
    const physics::PlaneCollider* m_collider = nullptr;
};
}
