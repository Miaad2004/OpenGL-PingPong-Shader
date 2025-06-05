#version 130

in vec3 vColor;
out vec4 FRAG_COLOR;

void main()
{
    FRAG_COLOR = vec4(vColor, 1.0);
}
