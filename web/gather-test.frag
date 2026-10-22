#version 450
layout(set=1,binding=0) uniform sampler2D image;
layout(location=0) out vec4 result;
#ifndef FIXTURE_CONSTANT
layout(set=0,binding=0) uniform Offsets { ivec2 delta; } offsets;
#endif
void main() {
#ifdef FIXTURE_CONSTANT
  result=textureGatherOffset(image,vec2(0.5),ivec2(1,-1),3);
#else
  result=textureGatherOffset(image,vec2(0.5),offsets.delta,3);
#endif
}
