#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec2 uvs;
layout(location = 3) in vec3 colors;
layout(location = 4) in vec3 tangents;

//TODO create uniform transformations matrices Model View Projection
// Values that stay constant for the whole mesh.
uniform mat4 MVP;
uniform mat4 MVP_ORTHO; 
uniform mat4 MODEL;

out vec3 _normals;
out vec4 _clipPosition;
out vec2 _uvs;
out vec3 _colors;
out vec3 _worldPos;
out mat3 _TBN;

vec3 fallbackTangent(vec3 normal)
{
        vec3 referenceAxis = abs(normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        return normalize(cross(referenceAxis, normal));
}

void main(){
        gl_Position = MVP * vec4(vertices, 1);
        _clipPosition = gl_Position;
        vec3 worldNormal = normalize((MVP_ORTHO * vec4(normals, 0)).xyz);
        vec3 worldTangent = (MODEL * vec4(tangents, 0)).xyz;
        worldTangent -= worldNormal * dot(worldNormal, worldTangent);
        if (length(worldTangent) <= 0.0001)
                worldTangent = fallbackTangent(worldNormal);
        else
                worldTangent = normalize(worldTangent);
        vec3 worldBitangent = normalize(cross(worldNormal, worldTangent));

        _normals = worldNormal;
        _uvs = uvs;
        _colors = colors;
        _worldPos = (MODEL * vec4(vertices, 1)).xyz;
        _TBN = mat3(worldTangent, worldBitangent, worldNormal);
}
