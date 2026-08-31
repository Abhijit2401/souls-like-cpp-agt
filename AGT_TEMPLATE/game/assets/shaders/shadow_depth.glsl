// Shadow pass: renders shadow-casting geometry from the light's point of view into a depth-only
// target (see engine::shadow_map). The vertex stage is a trimmed copy of mesh.glsl's - same
// attribute layout and bone-skinning logic, so animated meshes (player/enemies/boss/priest)
// cast a shadow in their actual current pose, not a bind pose - but it writes gl_Position using
// whatever u_view_projection happens to be bound (the light's matrix here, the camera's during
// the normal pass) instead of assuming one or the other. The fragment stage does nothing at all:
// this target has no colour attachment, only depth, which GL writes automatically.

#type vertex
#version 430 core

layout (location = 0) in vec3 a_position;
layout (location = 3) in ivec4 bone_ids;
layout (location = 4) in vec4 weights;

const int MAX_BONES = 100;

uniform mat4 u_view_projection;
uniform mat4 u_transform;
uniform mat4 gBones[MAX_BONES];
uniform int num_bones;

void main()
{
    vec4 pos_l;
    if (num_bones > 0)
    {
        mat4 bone_transform;
        if (weights[0] <= 0)
        {
            bone_transform = mat4(1.0f);
            bone_transform += gBones[bone_ids[0]] * weights[0];
        }
        else
        {
            bone_transform = gBones[bone_ids[0]] * weights[0];
        }
        bone_transform += gBones[bone_ids[1]] * weights[1];
        bone_transform += gBones[bone_ids[2]] * weights[2];
        bone_transform += gBones[bone_ids[3]] * weights[3];
        pos_l = bone_transform * vec4(a_position, 1.0);
    }
    else
    {
        pos_l = vec4(a_position, 1.0);
    }

    gl_Position = u_view_projection * u_transform * pos_l;
}

#type fragment
#version 430 core
void main()
{
}
