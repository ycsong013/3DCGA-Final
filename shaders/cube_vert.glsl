// Shader for the environment map vertices to then convert them into locations in a cubemap.
#version 410

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

layout(location == 0) in vec3 position;

out vec3 fragPosition;

void main() 
{
    gl_Position = projectionMatrix * viewMatrix * vec4(position, 1.0);
    fragPosition = position;

}