#version 410

uniform sampler2D laTexture;

in Attribs {
   vec4 couleur;
   vec2 texCoord;
   flat int texNumero;
} AttribsIn;

out vec4 FragColor;

void main( void )
{
   FragColor = AttribsIn.couleur;
    if ( AttribsIn.texNumero != 0 ){
        vec4 texel = texture( laTexture, AttribsIn.texCoord );
        if ( texel.a < 0.1 ) discard;
        FragColor.rgb = mix( AttribsIn.couleur.rgb, texel.rgb, 0.7 );
        FragColor.a = AttribsIn.couleur.a;
    }
}
