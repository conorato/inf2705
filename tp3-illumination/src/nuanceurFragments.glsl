#version 410
#define M_PI 3.141592653589793238

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
   vec3 spotDir;
   vec3 normalFace;
   vec3 lumiDirFace;
   vec3 obsVecFace;
   vec2 texCoord;
} AttribsIn;

out vec4 FragColor;

const float DEG_TO_RAD = M_PI / 180;

float calculerSpot( in vec3 spotDir, in vec3 L )
{
   float cosAngleOuverture = cos(LightSource[0].spotAngleOuverture * DEG_TO_RAD);
   float cosAngleFrag = dot(L, spotDir);
   float factDirect3D = smoothstep(pow(cosAngleOuverture, 1.01 + LightSource[0].spotExponent / 2), cosAngleOuverture, cosAngleFrag);
   if( cosAngleFrag >= cosAngleOuverture) {
      return utiliseDirect ? pow(cosAngleFrag, LightSource[0].spotExponent): factDirect3D;
   }
   return( 0.0 );
}

vec4 calculerReflexion( in vec3 L, in vec3 N, in vec3 O )
{
  // Émission du matériau et du terme ambiant du modèle d'illumination
  vec4 coul = FrontMaterial.emission + FrontMaterial.ambient * LightModel.ambient;

  // Ajout de la composante ambiante de la source de lumière
  coul += FrontMaterial.ambient * LightSource[0].ambient;

  // Ajout de la composante diffure de la source de lumière

  coul += utiliseCouleur ? FrontMaterial.diffuse * LightSource[0].diffuse * max( 0.0, dot( N, L )) :
                           vec4(0.7,0.7,0.7,1.0) * LightSource[0].diffuse * max( 0.0, dot( N, L )) ;

  float composanteSpeculaire = utiliseBlinn ? max( 0.0, dot( normalize( L + O), N )):
                                              max( 0.0, dot( reflect( - L, N ), O));

  // Ajout de la composante spéculaire de la source lumineuse
  coul += FrontMaterial.specular * LightSource[0].specular * pow( composanteSpeculaire, FrontMaterial.shininess );

  return( clamp( coul, 0.0, 1.0 ) );
}

vec4 modifierTexelFonce(vec4 colorText, vec4 color) {
   vec4 modifiedColor = color * colorText;
   float meanColor = (colorText.r + colorText.g + colorText.b) / 3.0;
   if( meanColor < 0.5 ) {
      if( afficheTexelFonce == 1) {
         modifiedColor = ( colorText + color ) / 2.0 ;
      } else if (afficheTexelFonce == 2) {
         discard;
      }
   }
   return modifiedColor;
}

vec4 choisirCoulFrag(vec4 color) {
   vec4 couleurTexture = texture( laTexture, AttribsIn.texCoord );
   return texnumero == 0 ? color : modifierTexelFonce(couleurTexture, color);
}

void main( void )
{
   vec3 L = vec3( 0.0 ), N = vec3( 0.0 ), O = vec3( 0.0 );
   vec4 color = vec4( 0.0, 0.0, 0.0, 1.0);
   if( typeIllumination == 0 ) {
      N = normalize(AttribsIn.normalFace);
      O = normalize(AttribsIn.obsVecFace);
      L = normalize(AttribsIn.lumiDirFace);
      color.rgb = calculerReflexion( L, N, O ).xyz;
   } else if( typeIllumination == 1 ) {
      color = AttribsIn.couleur; //Gouraud a été calculé dans le nuanceur de sommets.
      L = normalize(AttribsIn.lumiDir);
   } else if( typeIllumination == 2 ) {
      N = normalize(AttribsIn.normal);
      O = normalize(AttribsIn.obsVec);
      L = normalize(AttribsIn.lumiDir);
      color.rgb = calculerReflexion( L, N, O ).xyz;
   }
   color = choisirCoulFrag(color) * calculerSpot( normalize( AttribsIn.spotDir ), L);
   FragColor.rgb = afficheNormales ? vec3(0.5 + 0.5 * N) : color.rgb;
}
