#version 130

in vec2 aPos;  

uniform vec2 uOffset;               
uniform vec3 uColor;              

out vec3 vColor;                    

void main()
{
    vColor     = uColor;                     
    gl_Position = vec4(aPos + uOffset, 0.0, 1.0);
}
