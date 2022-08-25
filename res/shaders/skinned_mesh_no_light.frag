#version 460 core

// Outputs colors in RGBA
out vec4 FragColor;

// Imports the texture coordinates from the Vertex Shader
in vec2 texCoord;

// Gets the Texture Unit from the main function
uniform sampler2D diffuse0;
uniform mat3 uv_transformation0;

void main()
{
    vec2 uvTransformed = (uv_transformation0 * vec3(texCoord.xy, 1)).xy;

    FragColor = texture(diffuse0, uvTransformed);

    // check if alpha value less than user-specified threshold
    if (FragColor.a < 0.1)
    {
        // discard this fragment because it is transparent
        discard;
    }
}
