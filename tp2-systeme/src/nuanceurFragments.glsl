#version 410

uniform int coulProfondeur;
const float debAttenuation = 30.0;
const float finAttenuation = 50.0;

in Attribs {
   vec4 couleur;
} AttribsIn;

out vec4 FragColor;

float  smoothstep( float x1, float x2, float x )
{ 
	float t = clamp((x-x1)/(x2-x1), 0.0, 1.0 );  
	return t*t*(3.0 -2.0*t); 
}

void main( void )
{
   // la couleur du fragment est la couleur interpolée
   FragColor = AttribsIn.couleur;

   // atténuer selon la profondeur
   if ( coulProfondeur == 1 )
   {
      // Obtenir la distance à la caméra du sommet dans le repère de la caméra
      float dist = gl_FragCoord.z / gl_FragCoord.w;
      // Obtenir un facteur d'interpolation entre 0 et 1
      float factDist = smoothstep( debAttenuation, finAttenuation, dist );
      // Modifier la couleur du fragment en utilisant ce facteur
      if ( dist > finAttenuation )
      { 
         FragColor *= 0;
      } else if ( dist <= finAttenuation && dist >= debAttenuation) {
         FragColor *= (1 - factDist);
      }

      // pour déboguer et « voir » la dist, on peut utiliser:
      //FragColor = vec4( vec3(dist-floor(dist)), 1.0 );
      //FragColor = vec4( vec3(factDist), 1.0 );
   }


   // pour déboguer et « voir » le comportement de z ou w, on peut utiliser:
   //FragColor = vec4( vec3(gl_FragCoord.z), 1.0 );
   //FragColor = vec4( vec3(gl_FragCoord.w), 1.0 );
}
