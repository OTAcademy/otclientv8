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
    varying mediump vec4 v_Color;\n\
    void main()\n\
    {\n\
        gl_Position = vec4((u_ProjectionMatrix * u_TransformMatrix * vec3(a_Vertex.xy, 1.0)).xy, a_Depth / 16384.0, 1.0);\n\
        v_TexCoord = (u_TextureMatrix * vec3(a_TexCoord,1.0)).xy;\n\
        v_Color = a_Color;\n\
    }\n";

// LIGHT DEPTH
static const std::string lightDepthVertexShader = "\n\
    attribute mediump vec2 a_Vertex;\n\
    attribute mediump float a_Depth;\n\
    \n\
    uniform mediump mat3 u_ProjectionMatrix;\n\
    uniform mediump mat3 u_TransformMatrix;\n\
    void main()\n\
    {\n\
        gl_Position = vec4((u_ProjectionMatrix * u_TransformMatrix * vec3(a_Vertex.xy, 1.0)).xy, a_Depth / 16384.0, 1.0);\n\
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
    varying mediump vec4 v_Color;\n\
    uniform sampler2D u_Tex0;\n\
    void main()\n\
    {\n\
        gl_FragColor = texture2D(u_Tex0, v_TexCoord) * v_Color;\n\
        if(gl_FragColor.a < 0.01)\n\
            discard;\n\
    }\n";

// LIGHT DEPTH
static const std::string lightDepthFragmentShader = "\n\
    void main()\n\
    {\n\
        gl_FragColor = vec4(0.,0.,0.,1.);\n\
    }\n";

#endif