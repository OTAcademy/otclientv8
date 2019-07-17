#ifndef NEWSHADER_H
#define NEWSHADER_H

#include <string>
// VERTEX
static const std::string newVertexShader = "\n\
    attribute mediump vec2 a_Vertex;\n\
    attribute mediump vec2 a_TexCoord;\n\
    attribute mediump float a_Depth;\n\
    attribute mediump vec4 a_Color;\n\
    \n\
    uniform mediump mat3 u_ProjectionMatrix;\n\
    uniform mediump mat3 u_TransformMatrix;\n\
    uniform mediump mat3 u_TextureMatrix;\n\
    \n\
    varying mediump vec2 v_TexCoord;\n\
    varying mediump vec4 v_Color;\n\
    void main()\n\
    {\n\
        gl_Position = vec4((u_ProjectionMatrix * u_TransformMatrix * vec3(a_Vertex.xy, 1.0)).xy, a_Depth / 16384.0, 1.0);\n\
        v_TexCoord = (u_TextureMatrix * vec3(a_TexCoord,1.0)).xy;\n\
        v_Color = a_Color;\n\
    }\n";

// LIGHT
static const std::string lightVertexShader = "\n\
    attribute mediump vec2 a_Vertex;\n\
    attribute mediump vec2 a_TexCoord;\n\
    attribute mediump float a_Depth;\n\
    attribute mediump vec4 a_Color;\n\
    \n\
    uniform mediump mat3 u_ProjectionMatrix;\n\
    uniform mediump mat3 u_TransformMatrix;\n\
    uniform mediump mat3 u_TextureMatrix;\n\
    \n\
    varying mediump vec2 v_TexCoord;\n\
    varying mediump vec2 v_DepthTexCoord;\n\
    varying mediump vec4 v_Color;\n\
    varying mediump float v_Depth;\n\
    void main()\n\
    {\n\
        gl_Position = vec4((u_ProjectionMatrix * u_TransformMatrix * vec3(a_Vertex.xy, 1.0)).xy, 1.0, 1.0);\n\
        v_TexCoord = (u_TextureMatrix * vec3(a_TexCoord,1.0)).xy;\n\
        v_Color = a_Color;\n\
        v_Depth = (a_Depth / 16384.0) * 0.5 + 0.5;\n\
    }\n";

// DEPTH SCALING
static const std::string lightDepthScalingVertexShader = "\n\
    attribute highp vec2 a_Vertex;\n\
    attribute highp vec2 a_TexCoord;\n\
    \n\
    uniform highp mat3 u_ProjectionMatrix;\n\
    uniform highp mat3 u_TransformMatrix;\n\
    uniform highp mat3 u_TextureMatrix;\n\
    \n\
    varying highp vec2 v_TexCoord;\n\
    void main()\n\
    {\n\
        gl_Position = vec4((u_ProjectionMatrix * u_TransformMatrix * vec3(a_Vertex.xy, 1.0)).xy, 1.0, 1.0);\n\
        v_TexCoord = (u_TextureMatrix * vec3(a_TexCoord,1.0)).xy;\n\
    }\n";

// FRAGMENT
static const std::string newFragmentShader = "\n\
    varying mediump vec2 v_TexCoord;\n\
    varying mediump vec4 v_Color;\n\
    uniform sampler2D u_Atlas;\n\
    void main()\n\
    {\n\
        gl_FragColor = texture2D(u_Atlas, v_TexCoord) * v_Color;\n\
        if(gl_FragColor.a < 0.01)\n\
            discard;\n\
    }\n";

// LIGHT
static const std::string lightFragmentShader = "\n\
    varying mediump vec2 v_TexCoord;\n\
    varying mediump vec2 v_DepthTexCoord;\n\
    varying mediump vec4 v_Color;\n\
    varying mediump float v_Depth;\n\
    uniform sampler2D u_Tex0;\n\
    //uniform sampler2D u_TexDepth;\n\
    uniform highp vec2 u_Resolution;\n\
    void main()\n\
    {\n\
        //highp vec2 pos = gl_FragCoord.xy / u_Resolution;\n\
        //if(v_Depth > texture2D(u_TexDepth, pos).r) discard;\n\
        gl_FragColor = texture2D(u_Tex0, v_TexCoord) * v_Color;\n\
        //gl_FragColor = texture2D(u_TexDepth, pos);\n\
        //gl_FragColor.b = v_Depth;\n\
    }\n";

// LIGHT DEPTH SCALING
static const std::string lightDepthScalingFragmentShader = "\n\
    varying highp vec2 v_TexCoord;\n\
    uniform highp sampler2D u_Tex0;\n\
    uniform highp vec2 u_Scaling;\n\
    uniform highp vec2 u_Resolution;\n\
    void main()\n\
    {\n\
        highp vec2 pos = gl_FragCoord.xy / u_Resolution;\n\
        gl_FragColor.r = texture2D(u_Tex0, pos).r;\n\
        gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(pos.x, pos.y + u_Scaling.y)).r);\n\
        gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(pos.x, pos.y - u_Scaling.y)).r);\n\
        gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(pos.x + u_Scaling.x, pos.y)).r);\n\
        gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(pos.x - u_Scaling.x, pos.y)).r);\n\
        gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(pos.x + u_Scaling.x, pos.y + u_Scaling.y)).r);\n\
        gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(pos.x + u_Scaling.x, pos.y - u_Scaling.y)).r);\n\
        gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(pos.x - u_Scaling.x, pos.y + u_Scaling.y)).r);\n\
        gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(pos.x - u_Scaling.x, pos.y - u_Scaling.y)).r);\n\
        if(gl_FragColor.r > 0.95) gl_FragColor.r = 0.;\n\
        //gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(v_TexCoord.x, v_TexCoord.y + u_Scaling.y * 2)).r);\n\
        //gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(v_TexCoord.x, v_TexCoord.y - u_Scaling.y * 2)).r);\n\
        //gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(v_TexCoord.x + u_Scaling.x * 2, v_TexCoord.y)).r);\n\
        //gl_FragColor.r = max(gl_FragColor.r, texture2D(u_Tex0, vec2(v_TexCoord.x - u_Scaling.x * 2, v_TexCoord.y)).r);\n\
        //gl_FragColor.g = (mod(gl_FragCoord.y, 32.)) / 32.;\n\
        //gl_FragColor.b = (mod(gl_FragCoord.x, 32.)) / 32.;\n\
    }\n";

#endif