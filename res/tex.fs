#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D ourTexture;

void main()
{
    //FragColor = texture(ourTexture, vec2(0.0f,0.0f));

    //vec4 x = texture(ourTexture, vec2(0.0f,0.0f));
    /*
    vec4 x = texture(ourTexture, TexCoord);
    float red = x.r;
    if(red < 0)
        red *= -1;
    red *= 0.8;

    if(TexCoord.x > 0.5f)
        red = 1.0;
    else
        red = 0.0;

    float blue = 0.0;

    if(TexCoord.y > 0.5f)
        blue = 1.0;
    */

    vec4 x = texture(ourTexture, TexCoord);
    float pressure = x.r;
    float p = x.r;
    if(pressure > 8)
        FragColor = vec4(1.0f,0.0f, 0, 1.0f); 
    else if (pressure > 1)
        FragColor = vec4(0,1, 0, 1.0f); 
    else if (pressure > 0)
        FragColor = vec4(0, 0.0f,1, 1.0f); 
    else
        FragColor = vec4(1, 1,1, 1.0f); 
        
    FragColor = vec4(p,p,p,1.0f);
        /*
    if(pressure> 256.0/255.0)
        FragColor = vec4(1, 0,0, 1.0f); 
    else
        FragColor = vec4(0, 1,0, 1.0f); 
        */
    /*
    if(pressure > 0)
        FragColor = vec4(0.0f,0.0f, 1 * pressure, 1.0f); 
    else
        FragColor = vec4( -1 * pressure, 0.0f,0.0f, 1.0f); 
    */

    if(p > 0)
    FragColor = vec4(0,0,0.01*p,1);
    else
    FragColor = vec4(-0.01*p,0,0,1);
    //FragColor = vec4(red,0.0f, blue, 1.0f);   
    //red *=  100;
    //FragColor.a = 
    //FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    //FragColor = vec4(red, red, red, 1.0f);
} 