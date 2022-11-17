#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D ourTexture;

void main()
{
    vec4 x = texture(ourTexture, TexCoord);
    float p = x.r;

    if(p > 0)
        FragColor = vec4(0,0,0.1*p,1);
    else
        FragColor = vec4(-0.1*p,0,0,1);
} 