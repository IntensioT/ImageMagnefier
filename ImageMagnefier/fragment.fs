#version 450 core
in vec2 uv;
out vec4 FragColor;
uniform sampler2D tex;
uniform vec2 offset;
uniform float zoom;
void main() {
    vec2 scaledUV = (uv - 0.5) / zoom + 0.5 + offset;
    FragColor = texture(tex, scaledUV);
}
