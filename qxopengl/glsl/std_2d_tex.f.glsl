uniform highp sampler2D texture;
varying highp vec2 f_texpos;

void main(void)
{
//    vec2 flipped_texpos = vec2(f_texpos.x, 1.0 - f_texpos.y);
    
    gl_FragColor = texture2D(texture, f_texpos);
}
