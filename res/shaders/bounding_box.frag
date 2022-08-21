#version 460 core

// Outputs colors in RGBA
out vec4 FragColor;

uniform vec3 Color;

void main()
{
    FragColor = vec4(Color, 1.0);
}
