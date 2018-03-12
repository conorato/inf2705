#version 410

layout(triangles) in;
  layout(triangle_strip, max_vertices = 3) out;

uniform mat4 matrModel;
uniform mat4 matrVisu;
uniform mat4 matrProj;

layout (std140) uniform varsUnif
{
   // partie 1: illumination
   int typeIllumination;     // 0:Lambert, 1:Gouraud, 2:Phong
   bool utiliseBlinn;        // indique si on veut utiliser modèle spéculaire de Blinn ou Phong
   bool utiliseDirect;       // indique si on utilise un spot style Direct3D ou OpenGL
   bool afficheNormales;     // indique si on utilise les normales comme couleurs (utile pour le débogage)
   // partie 3: texture
   int texnumero;            // numéro de la texture appliquée
   bool utiliseCouleur;      // doit-on utiliser la couleur de base de l'objet en plus de celle de la texture?
   int afficheTexelFonce;    // un texel noir doit-il être affiché 0:noir, 1:mi-coloré, 2:transparent?
};

in Attribs {
   vec4 couleur;
   vec3 normal;
   vec3 lumiDir;
   vec3 obsVec;
   vec3 spotDir;
   vec2 texCoord;
} AttribsIn[];

out Attribs {
   vec4 couleur;
   vec3 normal;
   vec3 lumiDir;
   vec3 obsVec;
   vec3 spotDir;
   vec3 normalFace;
   vec3 lumiDirFace;
   vec3 obsVecFace;
   vec2 texCoord;
} AttribsOut;

vec3 CalculerMoyenneVec( vec3 listeVecteurs[3] ){
   vec3 sumVec = vec3( 0.0, 0.0, 0.0 );
   for(int i = 0; i < gl_in.length(); ++i) {
      sumVec += listeVecteurs[i];
   }
   return sumVec / gl_in.length();
}

void main()
{
   if( typeIllumination == 0 ) {
      vec3 u = vec3( gl_in[1].gl_Position - gl_in[0].gl_Position );
      vec3 v = vec3( gl_in[2].gl_Position - gl_in[0].gl_Position );
      AttribsOut.normalFace = normalize( cross( u, v ) );
      AttribsOut.lumiDirFace = CalculerMoyenneVec( vec3[](AttribsIn[0].lumiDir,
                                                          AttribsIn[1].lumiDir,
                                                          AttribsIn[2].lumiDir ) );
      AttribsOut.obsVecFace = CalculerMoyenneVec( vec3[](AttribsIn[0].obsVec,
                                                         AttribsIn[1].obsVec,
                                                         AttribsIn[2].obsVec ) );
   } else {
     AttribsOut.normalFace = vec3( 0.0, 0.0, 0.0 );
     AttribsOut.lumiDirFace = vec3( 0.0, 0.0, 0.0 );
     AttribsOut.obsVecFace = vec3( 0.0, 0.0, 0.0 );
   }
   // émettre les sommets
   for ( int i = 0 ; i < gl_in.length() ; ++i )
   {
      gl_Position = matrProj * gl_in[i].gl_Position;
      AttribsOut.couleur = AttribsIn[i].couleur;
      AttribsOut.normal  = normalize(AttribsIn[i].normal);
      AttribsOut.lumiDir = AttribsIn[i].lumiDir;
      AttribsOut.obsVec  = AttribsIn[i].obsVec;
      AttribsOut.spotDir = AttribsIn[i].spotDir;
      AttribsOut.texCoord = AttribsIn[i].texCoord;
      EmitVertex();
   }
}
