attribute highp vec2 fragpos;
uniform highp mat4 transform;

void main(void)
{
    gl_Position = transform * vec4(fragpos, 0.0, 1.0);
}

