#include "RigidBody.hpp"
#include "PhysicEngine.hpp"


void physics::RigidBody::run()
{
    if (p_mass != mass)
        RecomputeInverseMass();
    if (registered)
        return;

    // Register using the transform's current world position so Scene setup order
    // cannot leave the rigid body with a stale initial position.
    m_position = m_parent -> transform.getWorldPos(glm::vec3(0.0f, 0.0f, 0.0f));
    m_rotation = m_parent -> transform.getRotation();
    m_orientation = m_parent -> transform.getOrientation();
    m_previousPosition = m_position;
    m_previousRotation = m_rotation;
    m_previousOrientation = m_orientation;
    Collider * c = m_parent -> getComponent<Collider>();
    if (c != nullptr)
    {
        RecomputeMassProperties();

        PhysicBody pb;
        pb.collider = c;
        pb.rb = this;
        pb.transform = &(m_parent -> transform);

        PhysicEngine::getInstance() -> AddBody(pb);
        registered = true;
        return;
    }
}

void physics::RigidBody::BeginSimulationStep()
{
    m_previousPosition = m_position;
    m_previousRotation = m_rotation;
    m_previousOrientation = m_orientation;
    m_hadContact = false;
    m_accumulatedContactNormal = glm::vec3(0.0f, 0.0f, 0.0f);

    UpdateDerivedData();
}

void physics::RigidBody::RecomputeInverseMass()
{
    if (isStatic || isKinematic || mass <= 0.0f)
        inverseMass = 0.0f;
    else
        inverseMass = 1.0f / mass;

    RecomputeMassProperties();
}

void physics::RigidBody::RecomputeMassProperties()
{
    if (useAutomaticInertiaTensor)
    {
        Collider * collider = m_parent -> getComponent<Collider>();
        if (collider != nullptr && isDynamic())
            inverseInertiaTensorLocal = collider->computeLocalInverseInertiaTensor(mass);
        else
            inverseInertiaTensorLocal = glm::mat3(0.0f);
    }

    UpdateDerivedData();
}

void physics::RigidBody::UpdateDerivedData()
{
    if (!isDynamic() || !enableAngularDynamics)
    {
        inverseInertiaTensorWorld = glm::mat3(0.0f);
        return;
    }

    const glm::mat3 rotationMatrix = glm::mat3_cast(m_orientation);
    inverseInertiaTensorWorld = rotationMatrix * inverseInertiaTensorLocal * glm::transpose(rotationMatrix);
}

void physics::RigidBody::RegisterContact(const glm::vec3 & contactNormal)
{
    m_hadContact = true;
    m_accumulatedContactNormal += contactNormal;
}

glm::vec3 physics::RigidBody::GetAverageContactNormal() const
{
    if (!m_hadContact)
        return glm::vec3(0.0f, 1.0f, 0.0f);

    return CollisionUtils::NormalizeSafe(m_accumulatedContactNormal, glm::vec3(0.0f, 1.0f, 0.0f));
}

void physics::RigidBody::ApplyMotionConstraints()
{
    if (freezePositionX)
    {
        m_position.x = m_previousPosition.x;
        m_linearVelocity.x = 0.0f;
    }
    if (freezePositionY)
    {
        m_position.y = m_previousPosition.y;
        m_linearVelocity.y = 0.0f;
    }
    if (freezePositionZ)
    {
        m_position.z = m_previousPosition.z;
        m_linearVelocity.z = 0.0f;
    }

    if (freezeRotationX)
    {
        m_rotation.x = m_previousRotation.x;
        m_angularVelocity.x = 0.0f;
    }
    if (freezeRotationY)
    {
        m_rotation.y = m_previousRotation.y;
        m_angularVelocity.y = 0.0f;
    }
    if (freezeRotationZ)
    {
        m_rotation.z = m_previousRotation.z;
        m_angularVelocity.z = 0.0f;
    }

    if (freezeRotationX || freezeRotationY || freezeRotationZ)
        m_orientation = glm::normalize(glm::quat(glm::radians(m_rotation)));
    else
        m_rotation = glm::degrees(glm::eulerAngles(m_orientation));

    UpdateDerivedData();
}

bool physics::RigidBody::isDynamic() const
{
    return !isStatic && !isKinematic && inverseMass > 0.0f;
}

void physics::RigidBody::AddForce(glm::vec3 force)
{
    accumulatedForce+=force;
}

void physics::RigidBody::AddForceAtPosition(const glm::vec3 & force, const glm::vec3 & worldPoint)
{
    accumulatedForce += force;
    if (enableAngularDynamics)
        accumulatedTorque += glm::cross(worldPoint - m_position, force);
}

void physics::RigidBody::AddTorque(glm::vec3 torque)
{
    if (!enableAngularDynamics)
        return;

    accumulatedTorque += torque;
}

void physics::RigidBody::AddRelativeTorque(const glm::vec3 & localTorque)
{
    const glm::mat3 rotationMatrix = glm::mat3_cast(m_orientation);
    AddTorque(rotationMatrix * localTorque);
}

void physics::RigidBody::Impulse(glm::vec3 force)
{
    ApplyImpulseAtWorldPoint(force, m_position);
}

void physics::RigidBody::ApplyImpulseAtWorldPoint(const glm::vec3 & impulse, const glm::vec3 & worldPoint)
{
    m_linearVelocity += impulse * inverseMass;

    if (!enableAngularDynamics)
        return;

    const glm::vec3 contactOffset = worldPoint - m_position;
    const glm::vec3 angularImpulse = glm::cross(contactOffset, impulse);
    m_angularVelocity += inverseInertiaTensorWorld * angularImpulse;
}

glm::vec3 physics::RigidBody::GetVelocityAtWorldPoint(const glm::vec3 & worldPoint) const
{
    if (!enableAngularDynamics)
        return m_linearVelocity;

    const glm::vec3 contactOffset = worldPoint - m_position;
    return m_linearVelocity + glm::cross(m_angularVelocity, contactOffset);
}

void physics::RigidBody::SetInverseInertiaTensorLocal(const glm::mat3 & inverseTensor)
{
    useAutomaticInertiaTensor = false;
    inverseInertiaTensorLocal = inverseTensor;
    UpdateDerivedData();
}

const glm::mat3 & physics::RigidBody::GetInverseInertiaTensorWorld() const
{
    return inverseInertiaTensorWorld;
}

void physics::RigidBody::Teleport(glm::vec3 newPos)
{
    m_position = newPos;
    m_previousPosition = newPos;
}

void physics::RigidBody::SetRotation(glm::vec3 eulerDegrees)
{
    m_rotation = eulerDegrees;
    m_previousRotation = eulerDegrees;
    m_orientation = glm::normalize(glm::quat(glm::radians(eulerDegrees)));
    m_previousOrientation = m_orientation;
    UpdateDerivedData();
}

void physics::RigidBody::SetVelocity(glm::vec3 v)
{
    m_linearVelocity = v;
}

void physics::RigidBody::SetAngularVelocity(glm::vec3 v)
{
    if (!enableAngularDynamics)
    {
        m_angularVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        return;
    }

    m_angularVelocity = v;
}

void physics::RigidBody::RefreshSerializedState()
{
    Teleport(m_position);
    SetRotation(m_rotation);
    SetVelocity(m_linearVelocity);
    SetAngularVelocity(m_angularVelocity);
    accumulatedForce = glm::vec3(0.0f, 0.0f, 0.0f);
    accumulatedTorque = glm::vec3(0.0f, 0.0f, 0.0f);
    m_hadContact = false;
    m_accumulatedContactNormal = glm::vec3(0.0f, 0.0f, 0.0f);
    RecomputeInverseMass();
    ApplyMotionConstraints();
}

physics::RigidBody::~RigidBody()
{
    if (registered)
    {
        Collider * c = m_parent -> getComponent<Collider>();
        if (c != nullptr)
        {
            PhysicBody pb;
            pb.collider = c;
            pb.rb = this;
            pb.transform = &(m_parent -> transform);

            PhysicEngine::getInstance() -> RemoveBody(pb);
        }
    }
}