#version 330 core

uniform vec3 _emissiveColor = vec3(1.0, 1.0, 1.0);
uniform float _emissiveIntensity = 1.0;

out vec4 color;

void main()
{
        color = vec4(_emissiveColor * max(_emissiveIntensity, 0.0), 1.0);
}
