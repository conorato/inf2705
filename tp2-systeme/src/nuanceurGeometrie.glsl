
#version 410

layout(triangles) in;
layout(triangle_strip, max_vertices = 8) out;

// in gl_PerVertex // <-- déclaration implicite
// {
//   vec4 gl_Position;
//   float gl_PointSize;
//   float gl_ClipDistance[];
// } gl_in[];

// out gl_PerVertex // <-- déclaration implicite
// {
//   vec4 gl_Position;
//   float gl_PointSize;
//   float gl_ClipDistance[];
// };
// out int gl_Layer;
// out int gl_ViewportIndex;

in Attribs {
   vec4 couleur;
   float clipDistance;
} AttribsIn[];

out Attribs {
   vec4 couleur;
} AttribsOut;

void main()
{
   // émettre les sommets
   for ( int i = 0 ; i < gl_in.length() ; ++i )
   {
      gl_ViewportIndex = 0;
      gl_Position = gl_in[i].gl_Position;
      AttribsOut.couleur = AttribsIn[i].couleur;
      gl_ClipDistance[0] = -AttribsIn[i].clipDistance;
      EmitVertex();
   }
   EndPrimitive();
   for ( int i = 0 ; i < gl_in.length() ; ++i )
   {
      gl_ViewportIndex = 1;
      gl_Position = gl_in[i].gl_Position;
      AttribsOut.couleur = AttribsIn[i].couleur;
      gl_ClipDistance[0] = AttribsIn[i].clipDistance;
      EmitVertex();
   }
   EndPrimitive();
}
