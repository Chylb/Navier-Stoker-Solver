#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in mat4 aInstanceMatrix;

void main()
{
    gl_Position = aInstanceMatrix * vec4(aPos, 1.0f); 
    //gl_Position = vec4(aPos, 1.0);
}