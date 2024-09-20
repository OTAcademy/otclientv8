
varying vec2 v_TexCoord;

uniform vec4 u_Color;
uniform sampler2D u_Tex0;
uniform sampler2D u_Tex1;
uniform float u_Time;
uniform vec2 u_WalkOffset;

vec2 snowDirection = vec2(1.0, 0.2);
float snowSpeed = 0.08;
float snowPressure = 0.4;
float snowZoom = 0.6;

void main()
{
    gl_FragColor = texture2D(u_Tex0, v_TexCoord) * u_Color;

    vec2 snowCoord = (v_TexCoord + vec2(u_WalkOffset.x, u_WalkOffset.y) + (snowDirection * u_Time * snowSpeed)) / snowZoom;
    vec4 effectPixel = texture2D(u_Tex1, snowCoord) * snowPressure;

    gl_FragColor += (effectPixel.rgba);
    if(gl_FragColor.a < 0.01)
        discard;
}
