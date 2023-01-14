// FILE: shaders/particle-lenia/fields_2d.frag
// this file get's prefixed with fields_functions_2d.glsl

out vec4 FragColor;
in vec2 TexCoord;

vec4 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    vec4 final = (c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0))).rgbr;
    final.a = 1.0;
    return final;
}

//uniform vec4 color1 = vec4(1.0, 0.78, 0.349, 1.0);
//uniform vec4 color2 = vec4(0.702, 0.949, 0.867, 1.0);
//uniform vec4 color1 = vec4(37. / 255., 77. / 255., 25. / 255., 1.0);
//uniform vec4 color2 = vec4(1.00, 0.11, 0.11, 1.0);
vec4 convert_rgb(vec3 a) {
    return vec4(a.r / 255., a.g / 255., a.b / 255., 1.0);
}

uniform vec4 background_color = vec4(1 / 255., 23 / 255., 47 / 255., 1.0);
uniform vec4 color1 = vec4(46 / 255., 134 / 255., 171 / 255., 1.0);
uniform vec4 color2 = vec4(241.0 / 255., 143.0 / 255., 1.0 / 255., 1.0);

// select fields to display
// 0: none
// 1: U, 2: R, 3: G, 4: E
uniform int render_1 = -1;
uniform int render_2 = -1;

// translate information
uniform float translate_x;
uniform float translate_y;

vec4 blend(vec4 color1, vec4 color2, float amount) {
    return (1.0 - amount) * color1 + amount * color2;
}

void main()
{
    if (render_1 == 0 && render_2 == 0) {
        FragColor = background_color;
    } else {
        float x = (TexCoord.x - 0.5) * 2 * internal_width + translate_x;
        float y = (TexCoord.y - 0.5) * 2 * internal_height + translate_y;

        vec4 fields = fields(vec2(x, y));

        float value_1 = 0.0;
        switch (render_1) {
            case 1:
                value_1 = min(fields.x, 1.0);
                break;
            case 2:
                value_1 = min(fields.y, 1.0);
                break;
            case 3:
                value_1 = min(fields.z, 1.0);
                break;
            case 4:
                value_1 = min(abs(fields.a), 1.0);
                break;
        }

        float value_2 = 0.0;
        switch (render_2) {
            case 1:
                value_2 = min(fields.x, 1.0);
                break;
            case 2:
                value_2 = min(fields.y, 1.0);
                break;
            case 3:
                value_2 = min(fields.z, 1.0);
                break;
            case 4:
                value_2 = min(abs(fields.a), 1.0);
                break;
        }

        FragColor =
        blend(
            blend(
                background_color, color1, value_1
            ),
            color2, value_2
        );
    }
}