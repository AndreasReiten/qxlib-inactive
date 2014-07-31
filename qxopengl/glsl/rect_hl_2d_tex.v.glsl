attribute vec2 fragpos;
attribute vec2 texpos;

uniform mat4 transform;
uniform vec4 bounds;
uniform float pixel_size;

varying vec2 f_texpos;
varying vec4 f_bounds;
varying float f_pixel_size;

void main(void)
{
    gl_Position =  transform * vec4(fragpos, 0.0, 1.0);
    f_texpos = texpos;
    f_bounds = bounds; 
    f_pixel_size = pixel_size;
}

