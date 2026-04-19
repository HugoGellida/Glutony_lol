#version 330 core

uniform vec3 _mainCol = vec3(1.0, 1.0, 1.0);

out vec4 color;

void main()
{
        vec3 ambient = _mainCol * 0.03;
        color = vec4(ambient, 1.0);
}
