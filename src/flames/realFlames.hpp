#ifndef REALFLAMES_HPP
#define REALFLAMES_HPP

class LineFlame;
class CPointFlame;
class DetachedFlame;
class DetachableFireSource;

#include "abstractFlames.hpp"

#include <list>

/**********************************************************************************************************************/
/**************************************** DEFINITION DE LA CLASSE LINEFLAME **************************************/
/**********************************************************************************************************************/

/** La classe LineFlame impl�mente une flamme qui provient d'une m�che "lin�aire".<br>
 * L'objet Wick appartient � la classe LineFlame, il est donc pr�cis� dans le constructeur
 * de Wick que l'objet doit �tre import� dans la sc�ne dans l'�tat "detached", de sorte que
 * le constructeur de la sc�ne ne cherche pas � le r�f�rencer ni � le d�truire.
 *
 * @author	Flavien Bridault
 */
class LineFlame : public RealFlame
{
public:
  /** Constructeur.
   * @param flameConfig CPointeur vers la configuration de la flamme.
   * @param tex CPointeur vers la texture � utiliser.
   * @param s CPointeur vers le solveur.
   * @param wickFileName Cha�ne de caract�re contenant le nom du fichier contenant la m�che.
   * @param parentFire CPointeur sur le feu auquel appartient la flamme.
   * @param detachedFlamesWidth Largeur des flammes d�tach�es.
   * @param wickName Cha�ne de caract�re contenant le nom de la m�che dans le fichier OBJ.
   */
  LineFlame (const FlameConfig& flameConfig, const ITexture* const tex, Field3D* const s,
	     Wick *wickObject, float width, float detachedFlamesWidth, DetachableFireSource *parentFire=NULL);
  virtual ~LineFlame();

  virtual void drawFlame(bool display, bool displayParticle) const{
    if(displayParticle) drawParticles();
    if(display) drawLineFlame();
  };

  virtual CVector getMainDirection() const
  {
    CVector direction;
    for(uint i = 0; i < m_nbLeadSkeletons; i++){
      direction += *(m_leadSkeletons[i]->getParticle(0));
    }
    direction = direction / m_nbLeadSkeletons;

    return direction;
  }

  virtual bool build()
  {
    if(m_flat)
      {
	m_nbFixedCPoints = 1;
	m_shadingType = m_shadingType | 2;
	return buildFlat();
      }
    else
      {
	m_nbFixedCPoints = 3;
	m_shadingType = m_shadingType & 1;
	return buildNormal();
      }
  }

  virtual CPoint getCenter () const { return m_center; };

  CPoint getTop() const { return m_top; };
  CPoint getBottom() const { return m_bottom; };

  virtual void setSamplingTolerance(u_char value){
    NurbsFlame::setSamplingTolerance(value);
    m_samplingMethod = value;
  };

  void computeVTexCoords();
  void breakCheck();

  /** M�thode permettant de g�n�rer des �tincelles dans le feu.
   * @todo Cette m�thode n'est pas encore termin�e.
   */
//   virtual void generateAndDrawSparks();
private:
  virtual bool buildNormal()
  {
    if(RealFlame::build()){
      computeCenterAndExtremities();
      return true;
    }
    return false;
  }

  virtual bool buildFlat ();

  virtual void computeCenterAndExtremities()
  {
    CPoint averagePos;

    /* Calcul des extr�mit�s haute et basse */
    m_top.resetToNull();
    m_bottom.resetToNull();
    for (vector < LeadSkeleton * >::iterator skeletonsIterator = m_leadSkeletons.begin ();
	 skeletonsIterator != m_leadSkeletons.end (); skeletonsIterator++)
      {
	m_top += *((*skeletonsIterator)->getParticle(0));
	m_bottom += *((*skeletonsIterator)->getRoot());
      }
    m_top = m_top / (float)m_leadSkeletons.size();
    m_bottom = m_bottom / (float)m_leadSkeletons.size();

    /* Calcul du centre */
    for (uint i = 1; i < m_nbLeadSkeletons-1 ; i++)
      averagePos += *m_leadSkeletons[i]->getMiddleParticle ();

    m_center = averagePos / (m_nbLeadSkeletons-2);
  }

  /** Liste des particules utilis�es pour afficher des �tincelles */
  list<Particle *> m_sparksList;

  /** CPointeur sur le feu auquel appartient la flamme */
  DetachableFireSource *m_parentFire;

  /** Largeur des flammes d�tach�es */
  float m_detachedFlamesWidth;

  CPoint m_top, m_bottom, m_center;

  u_char m_samplingMethod;

  GLfloat m_vTexInit;
};

/**********************************************************************************************************************/
/*************************************** DEFINITION DE LA CLASSE POINTFLAME **************************************/
/**********************************************************************************************************************/

/** La classe CPointFlame impl�mente une flamme qui provient d'une m�che verticale droite.<br>
 * Elle g�n�re ses squelettes � partir du maillage de la m�che dont le nom est fourni au
 * constructeur.
 *
 * @author	Flavien Bridault
 */
class CPointFlame : public RealFlame
{
public:
  /** Constructeur.
   * @param flameConfig CPointeur sur le configuration de la flamme.
   * @param tex CPointeur vers la texture � utiliser.
   * @param s CPointeur vers le solveur.
   * @param rayon Valeur du rayon du cercle form� par les racines des squelettes.
   * @param wick Optionnel, objet repr�sentant la m�che. Si NULL, un cylindre simple est utilis�.
   */
  CPointFlame ( const FlameConfig& flameConfig, const ITexture* const tex, Field3D* const s,
	       float rayon, Wick *wick);

  /** Destructeur*/
  virtual ~CPointFlame();

  virtual void drawFlame(bool display, bool displayParticle) const
  {
    if(displayParticle) drawParticles();
    if(display) drawCPointFlame();
  };

  virtual CVector getMainDirection() const {
    return(*(m_leadSkeletons[0]->getParticle(0)));
  };
  virtual CPoint getCenter () const {
    return (*m_leadSkeletons[0]->getMiddleParticle ());
  };

  CPoint getTop() const { return *(m_leadSkeletons[0]->getParticle(0)); };
  CPoint getBottom() const { return *(m_leadSkeletons[0]->getRoot()); };

  /** Fonction testant si les squelettes doivent se briser. Si c'est le cas, elle effectue la division.
   * Dans le cas d'une CPointFlame, cette m�thode ne fait rien du tout pour l'instant.
   */
  void breakCheck() {};

  virtual void addForces (int fdf, float innerForce, char perturbate){
    for (vector < LeadSkeleton * >::iterator skeletonsIterator = m_leadSkeletons.begin ();
	 skeletonsIterator != m_leadSkeletons.end (); skeletonsIterator++)
      (*skeletonsIterator)->addForces (fdf, innerForce, perturbate);
    for (uint i = 0; i < m_nbSkeletons; i++)
      m_periSkeletons[i]->addForces ();
  }
  void getLightPositions (GLfloat lightPositions[8][4], uint& nbLights);
};


/**********************************************************************************************************************/
/********************************************* DEFINITION DE LA CLASSE DETACHEDFLAME **********************************/
/**********************************************************************************************************************/

/** La classe DetachedFlame d�fini une flamme d�tach�e, autrement dit une flamme �ph�m�re qui n'est pas attach�e � un
 * quelconque support. Pour l'instant seule une classe h�rit�e de RealFlame peut produire une flamme d�tach�e.
 *
 * @author	Flavien Bridault
 */
class DetachedFlame : public NurbsFlame
{
public:
  /** Constructeur de flamme d�tach�e. Il est appel�e lors de la division d'une flamme dans la m�thode breackCheck().
   * @param source Flamme qui a g�n�r�e la flamme d�tach�e.
   * @param nbLeadSkeletons Nombre de squelettes guides.
   * @param leadSkeletons Tableaux contenant les squelettes guides.
   * @param nbSkeletons Nombre de squelettes p�riph�riques.
   * @param periSkeletons Tableaux contenant les squelettes p�riph�riques.
   * @param tex CPointeur vers la texture � utiliser.
   */
  DetachedFlame(const RealFlame* const source, uint nbLeadSkeletons, FreeLeadSkeleton **leadSkeletons,
		uint nbSkeletons, FreePeriSkeleton **periSkeletons, const ITexture* const tex,
		bool smoothShading, u_char samplingMethod);

  /** Destructeur*/
  virtual ~DetachedFlame ();

  virtual bool build();
  void computeVTexCoords();

  virtual void drawFlame(bool display, bool displayParticle) const {
    if(displayParticle) drawParticles();
    if(display) drawLineFlame();
  };

  void drawParticles() const
  {
    uint i;
    /* D�placement et d�termination du maximum */
    for (i = 0; i < m_nbSkeletons; i++)
      m_periSkeletons[i]->draw();
    for (i = 0; i < m_nbLeadSkeletons; i++)
      m_leadSkeletons[i]->draw();
  };

  virtual CPoint getTop() const
  {
    CPoint averagePos;
    for (uint i = 0; i < m_nbLeadSkeletons; i++)
      averagePos += *m_leadSkeletons[i]->getParticle(0);
    averagePos = averagePos / m_nbLeadSkeletons;
    return averagePos;
  }
  virtual CPoint getBottom() const
  {
    CPoint averagePos;
    for (uint i = 0; i < m_nbLeadSkeletons; i++)
      averagePos += *m_leadSkeletons[i]->getLastParticle();
    averagePos = averagePos / m_nbLeadSkeletons;
    return averagePos;
  }

protected:
  /** CPointeur vers les squelettes guide. */
  FreeLeadSkeleton **m_leadSkeletons;
  /** Nombres de squelettes guides. */
  uint m_nbLeadSkeletons;
  /** Tableau contenant les pointeurs vers les squelettes p�riph�riques. */
  FreePeriSkeleton **m_periSkeletons;

  /** Tableau temporaire contenant les distances entre les particules d'un squelette. */
  float *m_distances;

  /** Tableau temporaire utilis� pour classer les indices des distances entre points de contr�le
   * lors de l'ajout de points de contr�le suppl�mentaires dans la NURBS.  Allou� une seule fois
   * en d�but de programme � la taille maximale pour des raisons �videntes d'optimisation du temps
   * d'ex�cution.
   */
  int *m_maxDistancesIndexes;
};

#endif
