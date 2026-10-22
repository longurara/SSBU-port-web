#version 460
#extension GL_ARB_shader_viewport_layer_array : require
layout(location=0) flat out vec4 shade;
void main() {
  const vec2 points[3]=vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));
  gl_Position=vec4(points[gl_VertexIndex],0,1);
  uint relative_instance=uint(gl_InstanceIndex-gl_BaseInstance);
#ifdef FIXTURE_NONZERO
  gl_ViewportIndex=1;
#else
  gl_ViewportIndex=int(relative_instance);
#endif
  shade=vec4(float(relative_instance),1,0,1);
}
