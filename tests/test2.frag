#version 330 core

out vec4 FragColor;

in vec2 v_texCoords;

uniform sampler2D u_texture;

void main()
{
    FragColor = texture(u_texture, v_texCoords);
    if(FragColor.a < 0.5f) {
        discard;
    }
}