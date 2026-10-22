#version 450
#ifdef ARRAY_IMAGE
layout(set=1,binding=40) uniform sampler2DArray image;
#elif defined(UINT_IMAGE)
layout(set=1,binding=40) uniform usampler2D image;
#elif defined(SINT_IMAGE)
layout(set=1,binding=40) uniform isampler2D image;
#else
layout(set=1,binding=40) uniform sampler2D image;
#endif
layout(location=0) out vec4 result;
void main(){
#ifdef ARRAY_IMAGE
result=textureLod(image,vec3(0.5,0.5,0.0),0.0);
#elif defined(GATHER_IMAGE)
result=textureGatherOffset(image,vec2(0.5),ivec2(0),3);
#elif defined(FETCH_IMAGE) || defined(UINT_IMAGE) || defined(SINT_IMAGE)
result=vec4(texelFetch(image,ivec2(0),0));
#else
result=textureLod(image,vec2(0.5),0.0);
#endif
}
