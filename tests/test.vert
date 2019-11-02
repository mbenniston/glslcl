#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoords;

out vec2 v_texCoords;

uniform vec2 u_trans, u_scale;

void main()
{
    v_texCoords = texCoords;
    gl_Position = vec4(u_trans + u_scale * position, 0.0f, 1.0f);
}