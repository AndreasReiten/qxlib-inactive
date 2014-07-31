uniform sampler2D texture;
varying vec2 f_texpos;
varying vec4 f_bounds; 
varying float f_pixel_size;

void main(void)
{
    vec4 color = texture2D(texture, f_texpos);
 
    vec4 dark = vec4(0.0,0.0,0.0,1.0);
    
    // Width of inner border
    float border_width = min(f_bounds.z - f_bounds.x, f_bounds.w - f_bounds.y) * 0.05;
    
    // If outside of bounds
    if ((f_texpos.x < f_bounds.x) || (f_texpos.x > f_bounds.z) || (f_texpos.y < f_bounds.y) || (f_texpos.y > f_bounds.w))
    {
//        color.xyz = 1.0 - color.xyz;
        float average = 0.21 * color.x + 0.72 * color.y + 0.07 * color.z;
//        color.x = 0.0;
//        color.y = 0.0;
//        color.z = 0.0;
//        color.w = 1.0;
//        color.xyz = vec3(average);
//        color.w = 1.0;
//        vec4 dark = (vec4)(1.0,0.0,0.0,0.5);
        
//        color = dark;
        
//        if (f_texpos.x < f_bounds.x) average = min(average, mix(average, 0.0, f_texpos.x/f_bounds.x));
//        if (f_texpos.x > f_bounds.z) average = min(average, mix(average, 0.0, (1.0 - f_texpos.x)/(1.0 - f_bounds.z)));
//        if (f_texpos.y < f_bounds.y) average = min(average, mix(average, 0.0, f_texpos.y/f_bounds.y));
//        if (f_texpos.y > f_bounds.w) average = min(average, mix(average, 0.0, (1.0 - f_texpos.y)/(1.0 - f_bounds.w)));
        
//        avg + (vec4 - avg) * a
        
//        if (f_texpos.y < 0.5) color.xyz = mix(color.xyz, (vec3)(1.0),0.5);
        
        color.xyz = vec3(average);
        
        color = mix(color, dark, 1.0 - color.w);
        
//        color.w = 0.5;
    }
    // Else within bounds
//    else if ((f_texpos.x < f_bounds.x + f_pixel_size) || (f_texpos.x > f_bounds.z - f_pixel_size) || (f_texpos.y < f_bounds.y + f_pixel_size) || (f_texpos.y > f_bounds.w - f_pixel_size))
//    {
//        color.xyz = 1.0 - color.xyz;
//    }
    if ((f_texpos.x > f_bounds.x - f_pixel_size) && (f_texpos.x < f_bounds.x))
    {
        color = 1.0 - color;
        color = mix(color,dark,0.5);
    }
    else if ((f_texpos.x < f_bounds.z + f_pixel_size) && (f_texpos.x > f_bounds.z))
    {
        color = 1.0 - color;
        color = mix(color,dark,0.5);
    }
    else if ((f_texpos.y > f_bounds.y - f_pixel_size) && (f_texpos.y < f_bounds.y))
    {
        color = 1.0 - color;
        color = mix(color,dark,0.5);
    }
    else if ((f_texpos.y < f_bounds.w + f_pixel_size) && (f_texpos.y > f_bounds.w))
    {
        color = 1.0 - color;
        color = mix(color,dark,0.5);
    }

        
    
    gl_FragColor = color;
}
