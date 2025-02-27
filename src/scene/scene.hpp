#ifndef SCENE_H
#define SCENE_H

#define WICK_NAME_PREFIX "Wick"

class Scene;

#include "object.hpp"
#include "../flames/abstractFires.hpp"
#include "../flames/luminary.hpp"
#include "camera.hpp"
#include <vector>
#include <list>

class Material;
class FireSource;
class Object;

/**
 * Classe repr&eacute;sentant une sc&egrave;ne g&eacute;om&eacute;trique.
 * Une sc&egrave;ne comporte deux listes index�es des objets, une pour les objets projetant
 * des ombres et une pour les autres, une liste des mat�riaux associ�s � ces polygones et
 * une liste des sources lumineuses (inutilis�e pour l'instant).<br>
 * Cette classe encapsule �galement les fonctions d'import de fichiers OBJ.
 * @todo Certaines fonctions d'import semble �tre un peu d�cor�ll�es de la classe.
 * Je demande � voir si il ne faudrait pas les ressortir.
 *
 * @author	Flavien Bridault
 */
class Scene
{
public:
  /**
   * Constructeur par d�faut.
   * @param fileName Nom du fichier OBJ.
   * @param flames Tableau contenant les flammes.
   * @param nbFlames Nombre de flammes dans le tableau.
   */
  Scene(const char* const fileName, vector <Luminary *> *luminaries, vector <FireSource *> *flames);

  /** Destructeur par d&eacute;faut. */
  ~Scene();

  /** Lit un fichier OBJ pass&eacute; en param&egrave;tres et l'importe dans la sc&egrave;ne.
   * Si object est non nul, les objets seront ne seront pas ajout�s dans la liste des objets de la sc�ne,
   * mais dans la liste object. Ceci est par exemple utilis� pour les luminaires des flammes qui sont stock�s
   * dans FireSource et non dans Scene. Si objName est non nul, on cherchera � importer seulement les objets
   * commen�ant portant ce nom, sinon tous les objets contenu dans le fichier sont import�s.
   *
   * @param fileName nom du fichier OBJ &agrave; importer.
   * @param objectsList Optionnel, liste d'objets dans laquelle importer le fichier.
   * @param wicksList Optionnel, liste de m�ches dans laquelle importer le fichier.
   * @param prefix Pr�fixe servant de filtre pour les noms des objets.
   *
   * @return false si l'import a �chou�.
   */
  bool importOBJ(const char* fileName, list<Object*>* const objectsList=NULL, list<Wick*>* const wicksList=NULL,
		 const char* prefix=NULL);

  /** Lit un fichier OBJ pass&eacute; en param&egrave;tre et cherche le nom du fichier MTL.
   *
   * @param fileName Nom du fichier OBJ � lire.
   * @param mtlName Nom du fichier r�cup�r�, la cha�ne doit �tre allou�e au pr�alable.
   *
   * @return true si trouve un fichier MTL
   */
  bool getMTLFileNameFromOBJ(const char* fileName, char* mtlName);

  /** Lit un fichier MTL pass&eacute; en param&egrave;tres et importe les mat�riaux qu'il contient
   * dans la sc�ne.
   *
   * @param fileName nom du fichier OBJ &agrave; importer.
   */
  void importMTL(const char* fileName);

  /** Cr�e les VBOs - A n'appeler qu'une fois que tous les objets
   * ont �t� ajout� � la sc�ne, soit via le constructeur, soit via la m�thode
   * loadObject() qui permet d'ajouter des objets individuels.
   */
  void createVBOs(void);

  /** Cr�e les display lists - A n'appeler qu'une fois que tous les objets
   * ont �t� ajout� � la sc�ne, soit via le constructeur, soit via la m�thode
   * loadObject() qui permet d'ajouter des objets individuels.
   */
  void createDisplayLists(void);

  void sortTransparentObjects();

  /** Ajoute un objet dans la sc�ne.
   * @param newObj CPointeur vers l'objet &agrave; ajouter.
   * @param objectWSV True si l'objet projette des ombres, false sinon.
   */
  void addObject(Object* const newObj, bool objectWSV = false)
  {
    if(objectWSV)
      m_objectsArrayWSV.push_back(newObj);
    else
      m_objectsArray.push_back(newObj);
  };

  /** Ajoute un mat�riau dans la sc�ne.
   * @param material CPointeur sur le nouveau mat�riau � r�f�rencer.
   */
  void addMaterial(Material *material)
  { m_materialArray.push_back(material); };

  /** Ajoute une texture dans la sc�ne.
   * @param texture CPointeur sur la nouvelle texture � r�f�rencer.
   * @return Indice OpenGL de la texture.
   */
  GLuint addTexture(CBitmapTexture *texture)
  { m_texturesArray.push_back(texture); return m_texturesArray.size()-1; };

  /** Donne l'indice d'un mat�riau dans la liste des mat�riaux de la sc�ne.
   * @param name Nom du mat�riau.
   * @return Index du mat�riau.
   */
  int getMaterialIndexByName(const char *name);

  /** Donne l'indice d'une texture dans la liste des textures de la sc�ne.
   * @param name Nom du fichier de la texture.
   * @return Index de la texture, -1 si elle n'existe pas
   */
  int searchTextureIndexByName(const char *name);

  /** Ajoute une source lumineuse � la sc�ne.
   * @param newSource pointeur vers la source lumineuse � ajouter.
   */
//  void addSource(Source* const newSource)
//  { m_lightSourcesArray.push_back(newSource); };

  /** Lecture du nombre de points contenus dans la sc�ne.
   * @return Nombre de points.
   */
  int getVertexCount();

  /** Lecture du nombre de polygones contenus dans la sc�ne.
   * @return Nombre de polygones.
   */
  int getPolygonsCount();

  /** Lecture du nombre d'objets contenus dans la sc�ne.
   * @return Nombre d'objets.
   */
  int getObjectsCount() const
  { return m_objectsArray.size() + m_objectsArrayWSV.size(); };

  /** Lecture du nombre de sources lumineuses contenus dans la sc�ne.
   * @return Nombre de sources lumineuses.
   */
//  int getSourcesCount() const
//  { return m_lightSourcesArray.size(); };

  /** Lecture d'un polygone sp�cifique contenu dans la sc�ne.
   * @param index Indice du polygone � obtenir.
   * @return Un pointeur vers le polygone recherch�.
   */
  Object* getObject(const int index) const
  { return (m_objectsArray[index]); };

  /** Lecture d'une source lumineuse sp�cifique contenue dans la sc�ne.
   * @param index Indice de la source � obtenir.
   * @return Un pointeur vers la source recherch�e.
   */
//  Source* getSource(const int index) const
//  { return (m_lightSourcesArray[index]); };

  /** Lecture d'un mat�riau sp�cifique contenue dans la sc�ne.
   * @param index Indice du mat�riau � obtenir.
   * @return Un pointeur vers le mat�riau recherch�.
   */
  Material* getMaterial(const int index) const
  { return (m_materialArray[index]); };

  /** Lecture d'une texture sp�cifique contenue dans la sc�ne.
   * @param index Indice de la texture � obtenir.
   * @return Un pointeur vers la texture recherch�e.
   */
  ITexture* getTexture(const int index) const
  { return (m_texturesArray[index]); };

  /** Change l'affichage des sph�res englobantes. */
  void setBoundingSphereMode(bool mode) { m_boundingSpheresMode = mode; };

  void computeVisibility(const Camera &view);

  /** Dessin des objets textur�s */
  void drawSceneTEX(void) const
  {
    for (vector<Object*>::const_iterator objectsArrayIterator = m_objectsArray.begin();
	 objectsArrayIterator != m_objectsArray.end();
	 objectsArrayIterator++)
      (*objectsArrayIterator)->draw(TEXTURED,true, m_boundingSpheresMode);
    for (vector<Object*>::const_iterator objectsArrayIteratorWSV = m_objectsArrayWSV.begin();
	 objectsArrayIteratorWSV != m_objectsArrayWSV.end();
	 objectsArrayIteratorWSV++)
      (*objectsArrayIteratorWSV)->draw(TEXTURED,true, m_boundingSpheresMode);
  };

  /** Dessin des objets non textur�s */
  void drawSceneWTEX(CShader *a_pShader=NULL) const
  {
    for (vector<Object*>::const_iterator objectsArrayIterator = m_objectsArray.begin();
	 objectsArrayIterator != m_objectsArray.end();
	 objectsArrayIterator++)
      (*objectsArrayIterator)->draw(FLAT,false, m_boundingSpheresMode);
    for (vector<Object*>::const_iterator objectsArrayIteratorWSV = m_objectsArrayWSV.begin();
	 objectsArrayIteratorWSV != m_objectsArrayWSV.end();
	 objectsArrayIteratorWSV++)
      (*objectsArrayIteratorWSV)->draw(FLAT,false, m_boundingSpheresMode);
    if(a_pShader != NULL)
      for (vector < Luminary* >::const_iterator luminariesIterator = m_luminaries->begin ();
	   luminariesIterator != m_luminaries->end (); luminariesIterator++){
	CPoint pos=(*luminariesIterator)->getPosition();
	CPoint sc=(*luminariesIterator)->getScale();
	a_pShader->SetUniform3f("lumTr", pos.x,pos.y,pos.z);
	a_pShader->SetUniform4f("scale", sc.x,sc.y,sc.z, 1.0f);
	(*luminariesIterator)->draw();
      }
    else
      for (vector < Luminary* >::const_iterator luminariesIterator = m_luminaries->begin ();
	   luminariesIterator != m_luminaries->end (); luminariesIterator++)
	(*luminariesIterator)->draw();
  };

  /** Dessin de tous les objets de la sc�ne en enlevant les textures si n�cessaire */
  void drawSceneWT(CShader *a_pShader=NULL) const
  {
    for (vector < Object * >::const_iterator objectsArrayIteratorWSV = m_objectsArrayWSV.begin ();
	 objectsArrayIteratorWSV != m_objectsArrayWSV.end ();
	 objectsArrayIteratorWSV++)
      (*objectsArrayIteratorWSV)->draw (AMBIENT,false, m_boundingSpheresMode);
    for (vector < Object * >::const_iterator objectsArrayIterator = m_objectsArray.begin ();
	 objectsArrayIterator != m_objectsArray.end ();
	 objectsArrayIterator++)
      (*objectsArrayIterator)->draw (AMBIENT,false, m_boundingSpheresMode);
    if(a_pShader != NULL)
      for (vector < Luminary* >::const_iterator luminariesIterator = m_luminaries->begin ();
	   luminariesIterator != m_luminaries->end (); luminariesIterator++){
	CPoint pos=(*luminariesIterator)->getPosition();
	CPoint sc=(*luminariesIterator)->getScale();
	a_pShader->SetUniform3f("lumTr", pos.x,pos.y,pos.z);
	a_pShader->SetUniform4f("scale", sc.x,sc.y,sc.z, 1.0f);
	(*luminariesIterator)->draw();
      }
    else
      for (vector < Luminary* >::const_iterator luminariesIterator = m_luminaries->begin ();
	   luminariesIterator != m_luminaries->end (); luminariesIterator++)
	(*luminariesIterator)->draw();
  };

  /** Dessin de tous les objets de la sc�ne */
  void drawScene () const
  {
    for (vector<Object*>::const_iterator objectsArrayIterator = m_objectsArray.begin();
	 objectsArrayIterator != m_objectsArray.end();
	 objectsArrayIterator++)
      (*objectsArrayIterator)->draw(ALL,true, m_boundingSpheresMode);
    for (vector < Object * >::const_iterator objectsArrayIteratorWSV = m_objectsArrayWSV.begin ();
	 objectsArrayIteratorWSV != m_objectsArrayWSV.end ();
	 objectsArrayIteratorWSV++)
      (*objectsArrayIteratorWSV)->draw (ALL,true, m_boundingSpheresMode);
    for (vector < Luminary* >::const_iterator luminariesIterator = m_luminaries->begin ();
	 luminariesIterator != m_luminaries->end (); luminariesIterator++)
      (*luminariesIterator)->draw();
  };

  /** Dessin de tous les objets qui projettent des ombres */
  void drawSceneWSV (void) const
  {
    for (vector < Object * >::const_iterator objectsArrayIteratorWSV = m_objectsArrayWSV.begin ();
	 objectsArrayIteratorWSV != m_objectsArrayWSV.end ();
	 objectsArrayIteratorWSV++)
      (*objectsArrayIteratorWSV)->draw (AMBIENT,false, m_boundingSpheresMode);
    for (vector < Luminary* >::const_iterator luminariesIterator = m_luminaries->begin ();
	 luminariesIterator != m_luminaries->end (); luminariesIterator++)
      (*luminariesIterator)->draw();
  };

  void computeBoundingBox(CPoint& max, CPoint& min);
private:
  vector<Object*> m_objectsArray; /** Liste des objets de la scene ne projetant pas d'ombres. */
  vector<Object*> m_objectsArrayWSV; /** Liste des objets de la scene projetant des ombres. */
//  vector<Source*> m_lightSourcesArray; /** Liste des sources de lumi�re. */
  vector<Material*> m_materialArray; /** Liste des mat�riaux.*/
  vector<CBitmapTexture*> m_texturesArray; /** Liste des textures.*/

  /** Tableaux de flammes. Il s'agit d'un pointeur sur le tableau de flamme cr�� dans la classe GlFlameCanvas.
   * Celles-ci sont n�cessaires pour recalculer la visibilit� des flammes dans computeVisibility().
   */
  vector <FireSource *> *m_flames;
  /** Tableaux de luminaires. Il s'agit d'un pointeur sur le tableau de luminaire cr�� dans la classe GlFlameCanvas.
   * Ceux-ci sont n�cessaires pour dessiner les luminaires lors du dessin de la sc�ne.
   */
  vector <Luminary *> *m_luminaries;

  /** Cha�ne de caract�re contenant le chemin courant. Elle est utilis�e dans les fonctions d'import pour
   * parcourir les diff�rents r�pertoires (scenes, textures, ...).
   */
  char m_currentDir[255];

  /** Fonctions retournant le chemin absolu d'un fichier par rapport au chemin courant.
   * @param fileName Nom du fichier � traiter.
   */
  void getSceneAbsolutePath(const char* const fileName);

  /** Mode d'affichage */
  uint m_boundingSpheresMode;
};//Scene

#endif
