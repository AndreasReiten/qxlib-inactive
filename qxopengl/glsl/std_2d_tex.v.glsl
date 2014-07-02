attribute highp vec2 fragpos;
attribute highp vec2 texpos;
varying highp vec2 f_texpos;
uniform highp mat4 transform;

void main(void)
{
    gl_Position =  transform * vec4(fragpos, 0.0, 1.0);
    f_texpos = texpos;
}

