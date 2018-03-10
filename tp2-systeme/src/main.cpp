// Prénoms, noms et matricule des membres de l'équipe:
// - WILLIAM HARVEY (1851388)
// - CLAUDIA ONORATO (1845448)


#include <stdlib.h>
#include <iostream>
#include "inf2705-matrice.h"
#include "inf2705-nuanceur.h"
#include "inf2705-fenetre.h"
#include "inf2705-forme.h"
#include <glm/gtx/io.hpp>
#include <vector>

// variables pour l'utilisation des nuanceurs
GLuint prog;      // votre programme de nuanceurs
GLint locVertex = -1;
GLint locColor = -1;
GLint locmatrModel = -1;
GLint locmatrVisu = -1;
GLint locmatrProj = -1;
GLint locplanCoupe = -1;
GLint loccoulProfondeur = -1;
GLuint progBase;  // le programme de nuanceurs de base
GLint locVertexBase = -1;
GLint locColorBase = -1;
GLint locmatrModelBase = -1;
GLint locmatrVisuBase = -1;
GLint locmatrProjBase = -1;

// matrices du pipeline graphique
MatricePipeline matrProj, matrVisu, matrModel;

// les formes
FormeCube *cube = NULL;
FormeSphere *sphere = NULL;
FormeTheiere *theiere = NULL;
FormeTore *toreTerre = NULL;
FormeTore *toreMars = NULL;
FormeTore *toreJupiter = NULL;
GLuint vao = 0;
GLuint vbo[2] = {0,0};

//
// variables d'état
//
struct Etat
{
   int modele;           // le modèle à afficher comme CorpsCeleste (1-sphère, 2-cube, 3-théière).
   bool modeSelection;   // on est en mode sélection?
   bool enmouvement;     // le modèle est en mouvement/rotation automatique ou non
   bool afficheAxes;     // indique si on affiche les axes
   bool coulProfondeur;  // indique si on veut colorer selon la profondeur
   GLenum modePolygone;  // comment afficher les polygones
   glm::ivec2 sourisPosPrec;
   // partie 1: utiliser un plan de coupe
   glm::vec4 planCoupe;  // équation du plan de coupe (partie 1)
   GLfloat angleCoupe;   // angle (degrés) autour de x (partie 1)
   // apprentissage supplémentaire: facteur de réchauffement
   float facteurRechauffement; // un facteur qui sert à calculer la couleur des pôles (0.0=froid, 1.0=chaud)
} etat = { 1, false, true, true, false, GL_FILL, glm::ivec2(0), glm::vec4( 0, 0, 1, 0 ), 0.0, 0.2 };

//
// variables pour définir le point de vue
//
class Camera
{
public:
   void definir()
   {
      matrVisu.LookAt( dist*cos(glm::radians(theta))*sin(glm::radians(phi)),
                       dist*sin(glm::radians(theta))*sin(glm::radians(phi)),
                       dist*cos(glm::radians(phi)),
                       0, 0, 0,
                       0, 0, 1 );

      // (pour apprentissage supplémentaire): La caméra est sur la Terre et voir passer les autres objets célestes en utilisant l'inverse de la matrice mm
   }
   void verifierAngles() // vérifier que les angles ne débordent pas les valeurs permises
   {
      const GLdouble MINPHI = 0.01, MAXPHI = 180.0 - 0.01;
      phi = glm::clamp( phi, MINPHI, MAXPHI );
   }
   double theta;         // angle de rotation de la caméra (coord. sphériques)
   double phi;           // angle de rotation de la caméra (coord. sphériques)
   double dist;          // distance (coord. sphériques)
   bool modeLookAt;      // on utilise LookAt (au lieu de Rotate et Translate)
} camera = { 90.0, 75.0, 35.0, true };


//
// les corps célestes
//
class CorpsCeleste
{
public:
   CorpsCeleste( float r, float dist, float rot, float rev, float vitRot, float vitRev,
                 glm::vec4 coul=glm::vec4(1.,1.,1.,1.), glm::vec4 coulSel=glm::vec4(1.,1.,1., 1.) ) :
      rayon(r), distance(dist),
      rotation(rot), revolution(rev),
      vitRotation(vitRot), vitRevolution(vitRev),
      couleur(coul), estSelectionne(false), couleurSel(coulSel) 
   { }

   void ajouteEnfant( CorpsCeleste &bebe )
   {
      enfants.push_back( &bebe );
   }

   void afficher( )
   {
      matrModel.PushMatrix(); {
         matrModel.Rotate( revolution, 0, 0, 1 ); // révolution du corps autour de son parent
         matrModel.Translate( distance, 0, 0 ); // position par rapport à son parent

         // afficher d'abord les enfants
         std::vector<CorpsCeleste*>::iterator it;
         for ( it  = enfants.begin() ; it!=enfants.end() ; it++ )
         {
            (*it)->afficher();
         }

         // afficher le parent
         matrModel.PushMatrix(); {
            matrModel.Rotate( rotation, 0, 0, 1 ); // rotation sur lui-même
            matrModel.Scale( rayon, rayon, rayon ); // la taille du corps
            glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );

            // la couleur du corps
            if( etat.modeSelection ) 
			    glVertexAttrib3fv( locColor, glm::value_ptr(couleurSel) );
			else 
			    glVertexAttrib4fv( locColor, glm::value_ptr(couleur) );
			
			// Fusion des couleurs (Partie 1)
			if( this->couleur.a < 1.0 && !etat.modeSelection ) {
				glEnable( GL_BLEND );
				glDepthMask( GL_FALSE );
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			}
			
            switch ( etat.modele )
            {
            default:
            case 1:
               sphere->afficher();
               break;
            case 2:
               cube->afficher();
               break;
            case 3:
               matrModel.Scale( 0.5, 0.5, 0.5 );
               matrModel.Translate( 0.0, 0.0, -1.0 );
               glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );
               theiere->afficher( );
               break;
            }
            
            // Désactiver la fusion de couleurs et activer le tampon de profondeur
            if( this->couleur.a < 1.0 && !etat.modeSelection ) {
				glDepthMask( GL_TRUE );
				glDisable( GL_BLEND );
			}
			
         } matrModel.PopMatrix(); glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );

      } matrModel.PopMatrix(); glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );
   }

   void avancerPhysique()
   {
      if( !this->estSelectionne ) 
      {
         const float dt = 0.5; // intervalle entre chaque affichage (en secondes)
         rotation += dt * vitRotation;
         revolution += dt * vitRevolution;
      }
   }

   std::vector<CorpsCeleste*> enfants; // la liste des enfants
   float rayon;          // le rayon du corps
   float distance;       // la distance au parent
   float rotation;       // l'angle actuel de rotation en degrés
   float revolution;     // l'angle actuel de révolution en degrés
   float vitRotation;    // la vitesse de rotation
   float vitRevolution;  // la vitesse de révolution
   glm::vec4 couleur;    // la couleur du corps
   bool estSelectionne;  // le corps est sélectionné ?
   glm::vec4 couleurSel; // la couleur en mode sélection
};

// Couleur affichee

const glm::vec4 coulAffichee[11] = { 
    glm::vec4(1.0, 1.0, 0.0, 0.5), 
    glm::vec4(0.5, 0.5, 1.0, 1.0),
    glm::vec4(0.6, 0.6, 0.6, 1.0),
    glm::vec4(0.6, 1.0, 0.5, 1.0),
    glm::vec4(0.4, 0.4, 0.8, 1.0),
    glm::vec4(0.5, 0.5, 0.1, 1.0),
    glm::vec4(1.0, 0.5, 0.5, 1.0),
    glm::vec4(0.7, 0.4, 0.5, 1.0),
    glm::vec4(0.4, 0.4, 0.8, 1.0),
    glm::vec4(0.5, 0.5, 0.1, 1.0),
    glm::vec4(0.7, 0.5, 0.1, 1.0)
};


//                     rayon  dist  rota revol vrota  vrevol
CorpsCeleste Soleil(   4.00,  0.0,  0.0,  0.0, 0.05, 0.0, coulAffichee[0], glm::vec4(0.2, 0.0, 0.0, 1.0) );

CorpsCeleste Terre(    0.70,  7.0, 30.0, 30.0, 2.5,  0.10, coulAffichee[1], glm::vec4(0.0, 0.2, 0.0, 1.0) );
CorpsCeleste Lune(     0.20,  1.5, 20.0, 30.0, 2.5, -0.35, coulAffichee[2], glm::vec4(0.0, 0.0, 0.2, 1.0) );

CorpsCeleste Mars(     0.50, 11.0, 20.0,140.0, 2.5,  0.13, coulAffichee[3], glm::vec4(0.4, 0.0, 0.0, 1.0) );
CorpsCeleste Phobos(   0.20,  1.0,  5.0, 15.0, 3.5,  1.7,  coulAffichee[4], glm::vec4(0.0, 0.4, 0.0, 1.0) );
CorpsCeleste Deimos(   0.25,  1.7, 10.0,  2.0, 4.0,  0.5,  coulAffichee[5], glm::vec4(0.0, 0.0, 0.4, 1.0) );

CorpsCeleste Jupiter(  1.20, 16.0, 10.0, 40.0, 0.2,  0.02, coulAffichee[6], glm::vec4(0.6, 0.0, 0.0, 1.0) );
CorpsCeleste Io(       0.20,  1.7,  5.0,  1.5, 2.5,  4.3,  coulAffichee[7], glm::vec4(0.0, 0.6, 0.0, 1.0) );
CorpsCeleste Europa(   0.25,  2.5, 87.0, 11.9, 3.5,  3.4,  coulAffichee[8], glm::vec4(0.0, 0.0, 0.6, 1.0) );
CorpsCeleste Ganymede( 0.30,  3.1, 10.0, 42.4, 4.0,  1.45, coulAffichee[9], glm::vec4(0.8, 0.0, 0.0, 1.0) );
CorpsCeleste Callisto( 0.35,  4.0, 51.0, 93.1, 1.0,  0.45, coulAffichee[10], glm::vec4(0.0, 0.8, 0.0, 1.0) );


void calculerPhysique( )
{
   if ( etat.enmouvement )
   {
      // incrémenter rotation[] et revolution[] pour faire tourner les planètes
      Soleil.avancerPhysique();
      Terre.avancerPhysique();
      Lune.avancerPhysique();
      Mars.avancerPhysique();
      Phobos.avancerPhysique();
      Deimos.avancerPhysique();
      Jupiter.avancerPhysique();
      Io.avancerPhysique();
      Europa.avancerPhysique();
      Ganymede.avancerPhysique();
      Callisto.avancerPhysique();
   }
}

void chargerNuanceurs()
{
   // charger le nuanceur de base
   {
      // créer le programme
      progBase = glCreateProgram();

      // attacher le nuanceur de sommets
      {
         GLuint nuanceurObj = glCreateShader( GL_VERTEX_SHADER );
         glShaderSource( nuanceurObj, 1, &ProgNuanceur::chainesSommetsMinimal, NULL );
         glCompileShader( nuanceurObj );
         glAttachShader( progBase, nuanceurObj );
         ProgNuanceur::afficherLogCompile( nuanceurObj );
      }
      // attacher le nuanceur de fragments
      {
         GLuint nuanceurObj = glCreateShader( GL_FRAGMENT_SHADER );
         glShaderSource( nuanceurObj, 1, &ProgNuanceur::chainesFragmentsMinimal, NULL );
         glCompileShader( nuanceurObj );
         glAttachShader( progBase, nuanceurObj );
         ProgNuanceur::afficherLogCompile( nuanceurObj );
      }

      // faire l'édition des liens du programme
      glLinkProgram( progBase );
      ProgNuanceur::afficherLogLink( progBase );

      // demander la "Location" des variables
      if ( ( locVertexBase = glGetAttribLocation( progBase, "Vertex" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de Vertex" << std::endl;
      if ( ( locColorBase = glGetAttribLocation( progBase, "Color" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de Color" << std::endl;
      if ( ( locmatrModelBase = glGetUniformLocation( progBase, "matrModel" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrModel" << std::endl;
      if ( ( locmatrVisuBase = glGetUniformLocation( progBase, "matrVisu" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrVisu" << std::endl;
      if ( ( locmatrProjBase = glGetUniformLocation( progBase, "matrProj" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrProj" << std::endl;
   }

   {
      // charger le nuanceur de ce TP

      // créer le programme
      prog = glCreateProgram();

      // attacher le nuanceur de sommets
      const GLchar *chainesSommets = ProgNuanceur::lireNuanceur( "nuanceurSommets.glsl" );
      if ( chainesSommets != NULL )
      {
         GLuint nuanceurObj = glCreateShader( GL_VERTEX_SHADER );
         glShaderSource( nuanceurObj, 1, &chainesSommets, NULL );
         glCompileShader( nuanceurObj );
         glAttachShader( prog, nuanceurObj );
         ProgNuanceur::afficherLogCompile( nuanceurObj );
         delete [] chainesSommets;
      }

      // partie 2:
      const GLchar *chainesGeometrie = ProgNuanceur::lireNuanceur( "nuanceurGeometrie.glsl" );
      if ( chainesGeometrie != NULL )
      {
         GLuint nuanceurObj = glCreateShader( GL_GEOMETRY_SHADER );
         glShaderSource( nuanceurObj, 1, &chainesGeometrie, NULL );
         glCompileShader( nuanceurObj );
         glAttachShader( prog, nuanceurObj );
         ProgNuanceur::afficherLogCompile( nuanceurObj );
         delete [] chainesGeometrie;
      }

      // attacher le nuanceur de fragments
      const GLchar *chainesFragments = ProgNuanceur::lireNuanceur( "nuanceurFragments.glsl" );
      if ( chainesFragments != NULL )
      {
         GLuint nuanceurObj = glCreateShader( GL_FRAGMENT_SHADER );
         glShaderSource( nuanceurObj, 1, &chainesFragments, NULL );
         glCompileShader( nuanceurObj );
         glAttachShader( prog, nuanceurObj );
         ProgNuanceur::afficherLogCompile( nuanceurObj );
         delete [] chainesFragments;
      }

      // faire l'édition des liens du programme
      glLinkProgram( prog );
      ProgNuanceur::afficherLogLink( prog );

      // demander la "Location" des variables
      if ( ( locVertex = glGetAttribLocation( prog, "Vertex" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de Vertex" << std::endl;
      if ( ( locColor = glGetAttribLocation( prog, "Color" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de Color" << std::endl;
      if ( ( locmatrModel = glGetUniformLocation( prog, "matrModel" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrModel" << std::endl;
      if ( ( locmatrVisu = glGetUniformLocation( prog, "matrVisu" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrVisu" << std::endl;
      if ( ( locmatrProj = glGetUniformLocation( prog, "matrProj" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de matrProj" << std::endl;
      if ( ( locplanCoupe = glGetUniformLocation( prog, "planCoupe" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de planCoupe" << std::endl;
      if ( ( loccoulProfondeur = glGetUniformLocation( prog, "coulProfondeur" ) ) == -1 ) std::cerr << "!!! pas trouvé la \"Location\" de coulProfondeur" << std::endl;
   }
}

void FenetreTP::initialiser()
{
   // donner la couleur de fond
   glClearColor( 0.1, 0.1, 0.1, 1.0 );

   // activer les etats openGL
   glEnable( GL_DEPTH_TEST );

   // activer le mélange de couleur pour la transparence
   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

   // charger les nuanceurs
   chargerNuanceurs();
   glUseProgram( prog );

   // les valeurs à utiliser pour tracer le quad
   const GLfloat taille = Jupiter.distance + Callisto.distance + Callisto.rayon;
   GLfloat coo[] = { -taille,  taille, 0,
                      taille,  taille, 0,
                      taille, -taille, 0,
                     -taille, -taille, 0 };
   const GLuint connec[] = { 0, 1, 2, 2, 3, 0 };
   
   // partie 1: initialiser le VAO (quad)
   glGenVertexArrays( 1, &vao );
   glBindVertexArray( vao );
   
   // partie 1: créer les deux VBO pour les sommets et la connectivité
   glGenBuffers( 2, vbo );
   
   glBindBuffer( GL_ARRAY_BUFFER, vbo[0] );
   glBufferData( GL_ARRAY_BUFFER, sizeof(coo), coo, GL_STATIC_DRAW );
   glVertexAttribPointer( locVertex, 3, GL_FLOAT, GL_FALSE, 0, 0 );
   glEnableVertexAttribArray( locVertex );

   glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo[1] );
   glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(connec), connec, GL_STATIC_DRAW );
   
   glBindVertexArray( 0 );
   
   // construire le graphe de scène
   Soleil.ajouteEnfant(Terre);
   Terre.ajouteEnfant(Lune);

   Soleil.ajouteEnfant(Mars);
   Mars.ajouteEnfant(Phobos);
   Mars.ajouteEnfant(Deimos);

   Soleil.ajouteEnfant(Jupiter);
   Jupiter.ajouteEnfant(Io);
   Jupiter.ajouteEnfant(Europa);
   Jupiter.ajouteEnfant(Ganymede);
   Jupiter.ajouteEnfant(Callisto);

   // créer quelques autres formes
   cube = new FormeCube( 1.5 );
   sphere = new FormeSphere( 1.0, 16, 16 );
   theiere = new FormeTheiere( );
   toreTerre = new FormeTore( 0.08, Terre.distance, 8, 200 );
   toreMars = new FormeTore( 0.08, Mars.distance, 8, 200 );
   toreJupiter = new FormeTore( 0.08, Jupiter.distance, 8, 200 );
}

void FenetreTP::conclure()
{
   delete cube;
   delete sphere;
   delete theiere;
   delete toreTerre;
   delete toreMars;
   delete toreJupiter;
   glDeleteBuffers( 2, vbo );
   glDeleteVertexArrays( 1, &vao );
}

void afficherQuad( GLfloat alpha ) // le plan qui ferme les solides
{
   glVertexAttrib4f( locColor, 1.0, 1.0, 1.0, alpha );
   // afficher le plan tourné selon l'angle courant et à la position courante
   // partie 1: modifs ici ...
   if( alpha < 1.0 ) {
      glEnable( GL_BLEND );
      glDepthMask( GL_FALSE );
      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
   }
   
   matrModel.PushMatrix();{
      matrModel.Translate( 0.0, 0.0, -etat.planCoupe.z * etat.planCoupe.w); 	
      matrModel.Rotate( etat.angleCoupe, 0.0, 1.0, 0.0 );
	  glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );
   	  glBindVertexArray( vao );
      glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );
      glBindVertexArray(0);
   }matrModel.PopMatrix();
   
   if( alpha < 1.0 ) {
	 glDepthMask( GL_TRUE );
	 glDisable( GL_BLEND );
   }
}

void afficherModele()
{
   glVertexAttrib4f( locColor, 1.0, 1.0, 1.0, 1.0 );

#if 1
   // afficher les deux tores pour identifier les orbites des planetes
   glVertexAttrib3f( locColor, 0.0, 0.0, 1.0 );
   toreTerre->afficher();
   glVertexAttrib3f( locColor, 0.0, 1.0, 0.0 );
   toreMars->afficher();
   glVertexAttrib3f( locColor, 1.0, 0.0, 0.0 );
   toreJupiter->afficher();
#endif

   // afficher le système solaire en commençant à la racine
   Soleil.afficher( );
}

void FenetreTP::afficherScene( )
{
   // effacer l'ecran et le tampon de profondeur et le stencil
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

   glUseProgram( progBase );

   // définir le pipeline graphique
   matrProj.Perspective( 50.0, (GLdouble) largeur_ / (GLdouble) hauteur_, 0.1, 100.0 );
   glUniformMatrix4fv( locmatrProjBase, 1, GL_FALSE, matrProj );

   camera.definir();
   glUniformMatrix4fv( locmatrVisuBase, 1, GL_FALSE, matrVisu );

   matrModel.LoadIdentity();
   glUniformMatrix4fv( locmatrModelBase, 1, GL_FALSE, matrModel );

   // afficher les axes
   if ( etat.afficheAxes ) FenetreTP::afficherAxes();

   // dessiner la scène
   glUseProgram( prog );
   glUniformMatrix4fv( locmatrProj, 1, GL_FALSE, matrProj );
   glUniformMatrix4fv( locmatrVisu, 1, GL_FALSE, matrVisu );
   glUniformMatrix4fv( locmatrModel, 1, GL_FALSE, matrModel );
   glUniform4fv( locplanCoupe, 1, glm::value_ptr(etat.planCoupe) );
   glUniform1i( loccoulProfondeur, etat.coulProfondeur );

   // afficher le modèle et tenir compte du stencil et du plan de coupe
   
   glEnable( GL_STENCIL_TEST );
   glStencilFunc( GL_ALWAYS, 1, 1 ); 
   glStencilOp( GL_INCR, GL_INCR, GL_INCR );

   glEnable( GL_CLIP_PLANE0 );
   afficherModele();
   glDisable( GL_CLIP_PLANE0 );
   
   // Le stencil étant maintenant rempli de 1 (au premier bit) à la position des planètes, 
   // on trace le plan blanc. 
   glStencilFunc( GL_EQUAL, 1, 1 );
   glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
   afficherQuad( 1.0 );
   glDisable( GL_STENCIL_TEST );
   
   // Partie 2 : Sélection d'une planète selon sa couleur
   if ( etat.modeSelection )
   {
      glFinish();    
      GLint cloture[4]; glGetIntegerv( GL_VIEWPORT, cloture );
      GLint posX = etat.sourisPosPrec.x, posY = cloture[3]-etat.sourisPosPrec.y;
      
      glReadBuffer( GL_BACK );
      
      Soleil.couleur   = Soleil.couleurSel;
      Terre.couleur    = Terre.couleurSel;
      Lune.couleur     = Lune.couleurSel;
      Mars.couleur     = Mars.couleurSel;
      Phobos.couleur   = Phobos.couleurSel;
      Deimos.couleur   = Deimos.couleurSel;
      Jupiter.couleur  = Jupiter.couleurSel;
      Io.couleur       = Io.couleurSel;
      Europa.couleur   = Europa.couleurSel;
      Ganymede.couleur = Ganymede.couleurSel;
      Callisto.couleur = Callisto.couleurSel;

      GLfloat couleur[3];
      glReadPixels( posX, posY, 1, 1, GL_RGB, GL_FLOAT, couleur );
      std::cout << "r: " << couleur[0] << " g: " << couleur[1] << " b: "<< couleur[2] << std::endl;
      
      if( couleur[0] == Soleil.couleurSel[0] )
        Soleil.estSelectionne = !Soleil.estSelectionne;
      else if ( couleur[1] == Terre.couleurSel[1] ) 
        Terre.estSelectionne = !Terre.estSelectionne;
      else if ( couleur[2] == Lune.couleurSel[2] ) 
        Lune.estSelectionne = !Lune.estSelectionne;
      else if ( couleur[0] == Mars.couleurSel[0] ) 
        Mars.estSelectionne = !Mars.estSelectionne;
      else if ( couleur[1] == Phobos.couleurSel[1] ) 
        Phobos.estSelectionne = !Phobos.estSelectionne;
      else if ( couleur[2] == Deimos.couleurSel[2] ) 
        Deimos.estSelectionne = !Deimos.estSelectionne;
      else if ( couleur[0] == Jupiter.couleurSel[0] ) 
        Jupiter.estSelectionne = !Jupiter.estSelectionne;
      else if ( couleur[1] == Io.couleurSel[1] ) 
        Io.estSelectionne = !Io.estSelectionne;
      else if ( couleur[2] == Europa.couleurSel[2] ) 
        Europa.estSelectionne = !Europa.estSelectionne;
      else if ( couleur[0] == Ganymede.couleurSel[0] ) 
        Ganymede.estSelectionne = !Ganymede.estSelectionne; 
      else if ( couleur[1] == Callisto.couleurSel[1] ) 
        Callisto.estSelectionne = !Callisto.estSelectionne; 
       
      Soleil.couleur   = coulAffichee[0];
      Terre.couleur    = coulAffichee[1];
      Lune.couleur     = coulAffichee[2];
      Mars.couleur     = coulAffichee[3];
      Phobos.couleur   = coulAffichee[4];
      Deimos.couleur   = coulAffichee[5];
      Jupiter.couleur  = coulAffichee[6];
      Io.couleur       = coulAffichee[7];
      Europa.couleur   = coulAffichee[8];
      Ganymede.couleur = coulAffichee[9];
      Callisto.couleur = coulAffichee[10];
      
      
   } else {
      afficherQuad( 0.25 );
   }
}

void FenetreTP::redimensionner( GLsizei w, GLsizei h )
{
   // Partie 2 : clôtures multiples
   GLfloat h2 = 0.5*h;
   GLfloat v[] = { 0, 0, w, h2, 0, h2, w, h2 };
   glViewportArrayv(0, 2, v);
}

void FenetreTP::clavier( TP_touche touche )
{
   switch ( touche )
   {
   case TP_ECHAP:
   case TP_q: // Quitter l'application
      quit();
      break;

   case TP_x: // Activer/désactiver l'affichage des axes
      etat.afficheAxes = !etat.afficheAxes;
      std::cout << "// Affichage des axes ? " << ( etat.afficheAxes ? "OUI" : "NON" ) << std::endl;
      break;

   case TP_v: // Recharger les fichiers des nuanceurs et recréer le programme
      chargerNuanceurs();
      std::cout << "// Recharger nuanceurs" << std::endl;
      break;

   case TP_ESPACE: // Mettre en pause ou reprendre l'animation
      etat.enmouvement = !etat.enmouvement;
      break;

   case TP_g: // Permuter l'affichage en fil de fer ou plein
      etat.modePolygone = ( etat.modePolygone == GL_FILL ) ? GL_LINE : GL_FILL;
      glPolygonMode( GL_FRONT_AND_BACK, etat.modePolygone );
      break;

   case TP_m: // Choisir le modèle: 1-sphère, 2-cube, 3-théière (déjà implanté)
      if ( ++etat.modele > 3 ) etat.modele = 1;
      std::cout << " etat.modele=" << etat.modele << std::endl;
      break;

   case TP_p: // Atténuer ou non la couleur selon la profondeur
      etat.coulProfondeur = !etat.coulProfondeur;
      std::cout << " etat.coulProfondeur=" << etat.coulProfondeur << std::endl;
      break;

   case TP_HAUT: // Déplacer le plan de coupe vers le haut
      etat.planCoupe.w += 0.1;
      std::cout << " etat.planCoupe.w=" << etat.planCoupe.w << std::endl;
      break;

   case TP_BAS: // Déplacer le plan de coupe vers le bas
      etat.planCoupe.w -= 0.1;
      std::cout << " etat.planCoupe.w=" << etat.planCoupe.w << std::endl;
      break;

   case TP_CROCHETDROIT:
   case TP_DROITE: // Augmenter l'angle du plan de coupe
      etat.angleCoupe += 0.5;
      etat.planCoupe.x = sin(glm::radians(etat.angleCoupe));
      etat.planCoupe.z = cos(glm::radians(etat.angleCoupe));
      std::cout << " etat.angleCoupe=" << etat.angleCoupe << std::endl;
      break;
   case TP_CROCHETGAUCHE:
   case TP_GAUCHE: // Diminuer l'angle du plan de coupe
      etat.angleCoupe -= 0.5;
      etat.planCoupe.x = sin(glm::radians(etat.angleCoupe));
      etat.planCoupe.z = cos(glm::radians(etat.angleCoupe));
      std::cout << " etat.angleCoupe=" << etat.angleCoupe << std::endl;
      break;

   // case TP_c: // Augmenter le facteur de réchauffement
   //    etat.facteurRechauffement += 0.05; if ( etat.facteurRechauffement > 1.0 ) etat.facteurRechauffement = 1.0;
   //    std::cout << " etat.facteurRechauffement=" << etat.facteurRechauffement << " " << std::endl;
   //    break;
   // case TP_f: // Diminuer le facteur de réchauffement
   //    etat.facteurRechauffement -= 0.05; if ( etat.facteurRechauffement < 0.0 ) etat.facteurRechauffement = 0.0;
   //    std::cout << " etat.facteurRechauffement=" << etat.facteurRechauffement << " " << std::endl;
   //    break;

   case TP_PLUS: // Incrémenter la distance de la caméra
   case TP_EGAL:
      camera.dist--;
      std::cout << " camera.dist=" << camera.dist << std::endl;
      break;

   case TP_SOULIGNE:
   case TP_MOINS: // Décrémenter la distance de la caméra
      camera.dist++;
      std::cout << " camera.dist=" << camera.dist << std::endl;
      break;

   default:
      std::cout << " touche inconnue : " << (char) touche << std::endl;
      imprimerTouches();
      break;
   }
}

static bool pressed = false;
void FenetreTP::sourisClic( int button, int state, int x, int y )
{
   pressed = ( state == TP_PRESSE );
   if ( pressed )
   {
      switch ( button )
      {
      default:
      case TP_BOUTON_GAUCHE: // Modifier le point de vue
         etat.modeSelection = false;
         break;
      case TP_BOUTON_DROIT: // Sélectionner des objets
         etat.modeSelection = true;
         break;
      }
      etat.sourisPosPrec.x = x;
      etat.sourisPosPrec.y = y;
   }
   else
   {
      etat.modeSelection = false;
   }
}

void FenetreTP::sourisWheel( int x, int y ) // Déplacer le plan de coupe
{
   const int sens = +1;
   etat.planCoupe.w += 0.02 * sens * y;
   std::cout << " etat.planCoupe.w=" << etat.planCoupe.w << std::endl;
}

void FenetreTP::sourisMouvement( int x, int y )
{
   if ( pressed )
   {
      if ( !etat.modeSelection )
      {
         int dx = x - etat.sourisPosPrec.x;
         int dy = y - etat.sourisPosPrec.y;
         camera.theta -= dx / 3.0;
         camera.phi   -= dy / 3.0;
      }

      etat.sourisPosPrec.x = x;
      etat.sourisPosPrec.y = y;

      camera.verifierAngles();
   }
}

int main( int argc, char *argv[] )
{
   // créer une fenêtre
   FenetreTP fenetre( "INF2705 TP" );

   // allouer des ressources et définir le contexte OpenGL
   fenetre.initialiser();

   bool boucler = true;
   while ( boucler )
   {
      // mettre à jour la physique
      calculerPhysique( );

      // affichage
      fenetre.afficherScene();

      if( etat.modeSelection ) 
      {
         etat.modeSelection = pressed = false;
      }
      else
        fenetre.swap();
      
      // récupérer les événements et appeler la fonction de rappel
      boucler = fenetre.gererEvenement();
   }

   // détruire les ressources OpenGL allouées
   fenetre.conclure();

   return 0;
}
