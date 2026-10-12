#pragma once
#include <glm/glm.hpp>

//ATTACK AREA for melee attacks, its a slice of a circle at weapon height that tilts to match the swing
//(flat for a sideways swing, upright for an overhead chop).
//The same shape is used for the indicator in CombatVfx and for checking hits, so if you can see it you can
//dodge it by moving out of it or rolling through it
struct AttackArc
{
    glm::vec3 origin{ 0.f };            //Attackers position (at the feet)
    glm::vec3 forward{ 0.f, 0.f, 1.f }; //Direction of the attack flattened onto the ground
    float radius = 2.0f;
    float half_angle_degrees = 60.0f;   //180 or more makes it a full circle
    float height = 1.0f;                //How high the swing is above the feet (weapon height)
    float roll_degrees = 0.0f;          //Tilt of the swing, 0 is flat and 90 or -90 is straight up and down

    bool is_full_circle() const { return half_angle_degrees >= 180.0f; }

    //Rotation around the Y axis so the indicator faces the same way as the attack
    float yaw() const { return atan2(forward.x, forward.z); }

    glm::vec3 centre() const { return origin + glm::vec3(0.f, height, 0.f); }
    glm::vec3 right() const { return glm::vec3(forward.z, 0.f, -forward.x); } //Right side of the attacker

    //Sideways direction and the up direction of the swing after its been tilted
    glm::vec3 side() const
    {
        float r = glm::radians(roll_degrees);
        return right() * cos(r) + glm::vec3(0.f, 1.f, 0.f) * sin(r);
    }
    glm::vec3 normal() const
    {
        float r = glm::radians(roll_degrees);
        return -right() * sin(r) + glm::vec3(0.f, 1.f, 0.f) * cos(r);
    }

    //Checks if a target standing at point (their feet) is inside the attack area, using their body size
    bool contains(const glm::vec3& point, float target_radius = 0.4f, float target_half_height = 0.6f) const
    {
        glm::vec3 to_target = (point + glm::vec3(0.f, target_half_height, 0.f)) - centre();

        //Checks the body is close enough to the swing (bodies are taller than they are wide)
        glm::vec3 n = normal();
        float body_extent = target_radius + target_half_height * glm::abs(n.y) + 0.15f;
        if (glm::abs(glm::dot(to_target, n)) > body_extent) return false;

        //Checks the target is in range and inside the angle of the swing
        float a = glm::dot(to_target, forward);
        float b = glm::dot(to_target, side());
        float dist = sqrt(a * a + b * b);
        if (dist > radius + target_radius) return false;
        if (is_full_circle() || dist < target_radius + 0.1f) return true;

        float angle = glm::degrees(atan2(glm::abs(b), a));
        float body_angle = glm::degrees(atan(target_radius / dist)); //So hits on the edge of the body still count
        return angle <= half_angle_degrees + body_angle;
    }

    static AttackArc make(const glm::vec3& origin, const glm::vec3& facing, float radius, float half_angle_degrees,
        float height = 1.0f, float roll_degrees = 0.0f)
    {
        AttackArc arc;
        arc.origin = origin;
        glm::vec3 flat(facing.x, 0.f, facing.z);
        arc.forward = glm::length(flat) > 0.001f ? glm::normalize(flat) : glm::vec3(0.f, 0.f, 1.f);
        arc.radius = radius;
        arc.half_angle_degrees = half_angle_degrees;
        arc.height = height;
        arc.roll_degrees = roll_degrees;
        return arc;
    }
};
