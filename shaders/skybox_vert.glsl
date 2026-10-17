// Shader for the skybox vertices. The skybox trick concepts for this shader were learned from 
// the LearnOpenGL tutorial: https://learnopengl.com/PBR/IBL/Diffuse-irradiance. Again, this shader
// was written without looking at the code and remembering the concepts taught in the tutorial.

#version 410

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

layout(location = 0) in vec3 position;

out vec3 fragPosition;

void main() 
{
    mat4 newViewMatrix = mat4(mat3(viewMatrix)); // The skybox should look infinitely far away, so the translation part of the matrix is removed, the sky should not move if the camera moves.
    gl_Position = (projectionMatrix * newViewMatrix * vec4(position, 1.0)).xyww; // This is another depth trick, discussed by LearnOpenGL
    fragPosition = position;

}