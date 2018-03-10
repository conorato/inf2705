#version 410

// Définition des paramètres des sources de lumière
layout (std140) uniform LightSourceParameters
{
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   vec4 position;      // dans le repère du monde
   vec3 spotDirection; // dans le repère du monde
   float spotExponent;
   float spotAngleOuverture; // ([0.0,90.0] ou 180.0)
   float constantAttenuation;
   float linearAttenuation;
   float quadraticAttenuation;
} LightSource[1];

// Définition des paramètres des matériaux
layout (std140) uniform MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
} FrontMaterial;

// Définition des paramètres globaux du modèle de lumière
layout (std140) uniform LightModelParameters
{
   vec4 ambient;       // couleur ambiante
   bool localViewer;   // observateur local ou à l'infini?
   bool twoSide;       // éclairage sur les deux côtés ou un seul?
} LightModel;

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

uniform sampler2D laTexture;

/////////////////////////////////////////////////////////////////

in Attribs {
   vec4 couleur;
   vec3 normal;
   vec3 lumiDir;
   vec3 obsVec;
   vec3 normalFace;
   vec3 lumiDirFace;
   vec3 obsVecFace;
} AttribsIn;

out vec4 FragColor;

float calculerSpot( in vec3 spotDir, in vec3 L )
{
   return( 0.0 );
}

vec4 calculerReflexion( in vec3 L, in vec3 N, in vec3 O )
{
  // Émission du matériau et du terme ambiant du modèle d'illumination
  vec4 coul = FrontMaterial.emission + FrontMaterial.ambient * LightModel.ambient;

  // Ajout de la composante ambiante de la source de lumière
  coul += FrontMaterial.ambient * LightSource[0].ambient;

  // Ajout de la composante diffure de la source de lumière
  coul += FrontMaterial.diffuse * LightSource[0].diffuse * max( 0.0, dot( N, L ));

  float composanteSpeculaire = utiliseBlinn ? max( 0.0, dot( normalize( L + O), N )):
                                              max( 0.0, dot( reflect( -L, N ), O));

  // Ajout de la composante spéculaire de la source lumineuse
  coul += FrontMaterial.specular * LightSource[0].specular * pow( composanteSpeculaire, FrontMaterial.shininess );

  return( clamp( coul, 0.0, 1.0 ) );
}

void main( void )
{
   vec3 L, N, O;
   // assigner la couleur finale
   //FragColor = AttribsIn.couleur;
   if( typeIllumination == 0 ) {
      N = normalize(AttribsIn.normalFace);
      O = normalize(AttribsIn.obsVecFace);
      L = normalize(AttribsIn.lumiDirFace);
      FragColor = vec4( calculerReflexion( L, N, O ).xyz, 1.0 );
   } else if( typeIllumination == 1 ) {
      FragColor = AttribsIn.couleur;
   } else if( typeIllumination == 2 ) {
      N = normalize(AttribsIn.normal);
      O = normalize(AttribsIn.obsVec);
      L = normalize(AttribsIn.lumiDir);
      FragColor = vec4( calculerReflexion( L, N, O ).xyz, 1.0 );
   }

   // vec4 coul = calculerReflexion( L, N, O );
   // ...
   //FragColor = afficheNormales ? vec4( 0.5 + 0.5 * N, 1.0 ) : vec4( 0.5, 0.5, 0.5, 1.0 );
}
