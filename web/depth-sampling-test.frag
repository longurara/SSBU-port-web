#version 450
layout(set=1,binding=0) uniform sampler2D color_image;
layout(set=1,binding=2) uniform sampler2D depth_image;
layout(location=0) out vec4 result;
void main() {
    result=vec4(textureLod(color_image,vec2(0.5),0.0).r,
                textureLod(depth_image,vec2(0.5),0.0).r,0.0,1.0);
}
