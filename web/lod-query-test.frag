#version 450
layout(set=0,binding=0) uniform Gradients { vec4 gradient; } params;
layout(set=1,binding=41) uniform sampler2D sourceTexture;
layout(location=0) out vec4 result;
void main() {
    vec2 uv=gl_FragCoord.x*params.gradient.xy+gl_FragCoord.y*params.gradient.zw;
    result=vec4(textureQueryLod(sourceTexture,uv),0.0,1.0);
}
