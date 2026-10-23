#version 410

out vec2 fragTexCoord;

// The quad creator passes the position and tex coordinate attribute vertices.
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

void main()
{
    fragTexCoord = texCoord;
    gl_Position = vec4(position, 1.0);
}