#version 410

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform int texnumero;
uniform mat4 matrProj;

in Attribs {
    vec4 couleur;
    float tempsRestant;
    float sens; // du vol
} AttribsIn[];

out Attribs {
    vec4 couleur;
    vec2 texCoord;
    float texNumero;
} AttribsOut;

vec2 rotate(vec2 v, float angle) {
	return mat2( cos(angle) , - sin(angle) ,  sin(angle) , cos(angle) ) * v;
}

vec2 reflectX(vec2 v) {
    return mat2( 1, 0, 0, -1 ) * v;
}

void main()
{
    const float pointSize = 5.0, numberElementsInTexture = 16.0;
    AttribsOut.couleur = AttribsIn[0].couleur;
    gl_Position = gl_in[0].gl_Position;
    AttribsOut.texNumero = float(texnumero);
    vec2 coins[4];
    coins[0] = vec2( -0.5,  0.5 );
    coins[1] = vec2( -0.5, -0.5 );
    coins[2] = vec2(  0.5,  0.5 );
    coins[3] = vec2(  0.5, -0.5 );
    for(int i = 0; i < 4; ++i) {
        AttribsOut.texCoord = coins[i] + vec2( 0.5, 0.5 );

        vec2 decalage = (texnumero == 1)? rotate(coins[i], 6.0 * AttribsIn[0].tempsRestant): coins[i];
        if( AttribsIn[0].sens == -1.0 ) {
            decalage = reflect(decalage, vec2(1, 0));
        }

        gl_Position = matrProj * vec4( gl_in[0].gl_Position.xy + 0.025 * pointSize * decalage, gl_in[0].gl_Position.zw );

        if( texnumero > 1) {
            AttribsOut.texCoord.s /= numberElementsInTexture;
            AttribsOut.texCoord.s += mod(int(18 * AttribsIn[0].tempsRestant), 16) / 16;
        }
        EmitVertex();
    }
}
