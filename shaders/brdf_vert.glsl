#version 410

out vec2 fragTexCoord;

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

void main()
{
    fragTexCoord = texCoord;
    gl_position = vec4(position, 1.0);
}