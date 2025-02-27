#include "abstractFlames.hpp"

#include <wx/intl.h>

/**********************************************************************************************************************/
/************************************** IMPLEMENTATION DE LA CLASSE NURBSFLAME ****************************************/
/**********************************************************************************************************************/

#ifdef COUNT_NURBS_POLYGONS
uint g_count=0;
#endif

NurbsFlame::NurbsFlame(uint nbSkeletons, ushort nbFixedCPoints, const ITexture* const tex)
{
  m_nbSkeletons = nbSkeletons;
  m_nbFixedCPoints = nbFixedCPoints;

  m_uorder = 4;
  m_vorder = 4;

  /* Allocation des tableaux à la taille maximale pour les NURBS, */
  /* ceci afin d'éviter des réallocations trop nombreuses */
  m_ctrlCPoints =  new GLfloat[(NB_PARTICLES_MAX + m_nbFixedCPoints) * (m_nbSkeletons + m_uorder - 1) * 3];
  m_texCPoints =  new GLfloat[(NB_PARTICLES_MAX + m_nbFixedCPoints) * (m_nbSkeletons + m_uorder - 1) * 2];
  m_uknots = new GLfloat[m_uorder + m_nbSkeletons + m_uorder - 1];
  m_vknots = new GLfloat[m_vorder + NB_PARTICLES_MAX + m_nbFixedCPoints];
  m_texTmp = new GLfloat[(NB_PARTICLES_MAX + m_nbFixedCPoints)];

  m_uknotsCount = m_vknotsCount = 0;

  /* Initialisation des pointeurs de sauvegarde des tableaux */
  m_ctrlCPointsSave = m_ctrlCPoints;
  m_texCPointsSave = m_texCPoints;
  m_texTmpSave = m_texTmp;

  initNurbs(&m_nurbs);
  gluNurbsProperty(m_nurbs, GLU_SAMPLING_METHOD, GLU_DOMAIN_DISTANCE);
  gluNurbsProperty(m_nurbs, GLU_U_STEP, 4);
  gluNurbsProperty(m_nurbs, GLU_V_STEP, 4);

  m_shadingType=1;

  m_tex = tex;
}

NurbsFlame::NurbsFlame(const NurbsFlame* const source, uint nbSkeletons, ushort nbFixedCPoints, const ITexture* const tex)
{
  m_nbSkeletons = nbSkeletons;
  m_nbFixedCPoints = nbFixedCPoints;

  m_uorder = 4;
  m_vorder = 4;

  /* Allocation des tableaux à la taille maximale pour les NURBS, */
  /* ceci afin d'éviter des réallocations trop nombreuses */
  m_ctrlCPoints =  new GLfloat[(NB_PARTICLES_MAX + m_nbFixedCPoints) * (m_nbSkeletons + m_uorder - 1) * 3];
  m_texCPoints =  new GLfloat[(NB_PARTICLES_MAX + m_nbFixedCPoints) * (m_nbSkeletons + m_uorder - 1) * 2];
  m_uknots = new GLfloat[m_uorder + m_nbSkeletons + m_uorder - 1];
  m_vknots = new GLfloat[m_vorder + NB_PARTICLES_MAX + m_nbFixedCPoints];
  m_texTmp = new GLfloat[(NB_PARTICLES_MAX + m_nbFixedCPoints)];

  m_uknotsCount = m_vknotsCount = 0;

  /* Initialisation des pointeurs de sauvegarde des tableaux */
  m_ctrlCPointsSave = m_ctrlCPoints;
  m_texCPointsSave = m_texCPoints;
  m_texTmpSave = m_texTmp;

  initNurbs(&m_nurbs);
  gluNurbsProperty(m_nurbs, GLU_SAMPLING_METHOD, GLU_DOMAIN_DISTANCE);
  gluNurbsProperty(m_nurbs, GLU_U_STEP, 4);
  gluNurbsProperty(m_nurbs, GLU_V_STEP, 4);

  m_shadingType=1;

  m_tex = tex;
}

NurbsFlame::~NurbsFlame()
{
  gluDeleteNurbsRenderer(m_nurbs);

  delete[]m_ctrlCPoints;
  delete[]m_uknots;
  delete[]m_vknots;
  delete[]m_texCPoints;
  delete[]m_texTmp;
}

void NurbsFlame::initNurbs(GLUnurbsObj** nurbs)
{
  *nurbs = gluNewNurbsRenderer();
  gluNurbsProperty(*nurbs, GLU_DISPLAY_MODE, GLU_FILL);
  gluNurbsProperty(*nurbs, GLU_NURBS_MODE,GLU_NURBS_TESSELLATOR);
  /* Important : ne fait pas la facettisation si la NURBS n'est pas dans le volume de vision */
  gluNurbsProperty(*nurbs, GLU_CULLING, GL_TRUE);
  gluNurbsCallback(*nurbs, GLU_NURBS_ERROR, (void(*)())nurbsError);

  gluNurbsCallback(*nurbs, GLU_NURBS_BEGIN_DATA, (void(*)())NurbsBegin);
  gluNurbsCallback(*nurbs, GLU_NURBS_VERTEX, (void(*)())NurbsVertex);
  gluNurbsCallback(*nurbs, GLU_NURBS_NORMAL, (void(*)())NurbsNormal);
  gluNurbsCallback(*nurbs, GLU_NURBS_TEXTURE_COORD, (void(*)())NurbsTexCoord);
  gluNurbsCallback(*nurbs, GLU_NURBS_END_DATA, (void(*)())NurbsEnd);

  gluNurbsCallbackData(*nurbs,(GLvoid *)&m_shadingType);
}

void NurbsFlame::drawNurbs () const
{
  if(m_uknotsCount && m_vknotsCount){
//     unsigned long long int start;
//     start = rdtsc();
    gluBeginSurface (m_nurbs);
    gluNurbsSurface (m_nurbs, m_uknotsCount, m_uknots, m_vknotsCount, m_vknots, m_vsize * 2,
		     2, m_texCPoints, m_uorder, m_vorder, GL_MAP2_TEXTURE_COORD_2);
    gluNurbsSurface (m_nurbs, m_uknotsCount, m_uknots, m_vknotsCount, m_vknots, m_vsize * 3,
		     3, m_ctrlCPoints, m_uorder, m_vorder, GL_MAP2_VERTEX_3);
    gluEndSurface (m_nurbs);
//     cerr << (rdtsc() - start)/CPU_FREQ << endl;
  }else
     cerr << "Not enough knots to draw flame : " << m_uknotsCount << " x " << m_vknotsCount << endl;
}

void NurbsFlame::drawLineFlame () const
{
  if (! (m_shadingType & 1) )
    {
      glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
      drawNurbs ();
    }
  else
    {
      glActiveTextureARB(GL_TEXTURE0_ARB);
      glEnable (GL_TEXTURE_2D);
      glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      m_tex->bind();

      drawNurbs ();

      glDisable (GL_TEXTURE_2D);
    }
}

/**********************************************************************************************************************/
/************************************** IMPLEMENTATION DE LA CLASSE FIXEDFLAME ****************************************/
/**********************************************************************************************************************/

FixedFlame::FixedFlame(uint nbSkeletons, ushort nbFixedCPoints, const ITexture* const tex) :
  NurbsFlame(nbSkeletons, nbFixedCPoints, tex)
{
}

FixedFlame::~FixedFlame()
{
}

void FixedFlame::drawCPointFlame () const
{
  if (! (m_shadingType & 1) )
    {
      glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
      drawNurbs ();
    }
  else
    {
      float angle, angle2, angle4;

      GLfloat m[16];

      glGetFloatv (GL_MODELVIEW_MATRIX, m);

//       CVector Transform=CVector(m[0]*(m[12]+.5f) + m[1]*(m[13]) + m[2]*(m[14]-.5f),
// 			      m[4]*(m[12]+.5f) + m[5]*(m[13]) + m[6]*(m[14]-.5f),
// 			      m[8]*(m[12]+.5f) + m[9]*(m[13]) + m[10]*(m[14]-.5f));

//       CVector direction(-Transform.x, 0.0f, -Transform.z);
//       direction.normalize ();
//       //cout << direction << endl;

//       CVector lookAt(0,0,1);
//       CVector upAux = lookAt^direction;
//       angle=acos(lookAt*direction);
//       if(upAux.y < 0) angle = -angle;
//             cout << angle << " " << angle/PI << endl;

      /* Position de la bougie = translation dans la matrice courante */
      CVector bougiepos(m[12], m[13], m[14]);

      /* Vecteur Z de l'axe de regard de bougie dans le repère du monde = axe initial * Matrice de rotation */
      /* Attention, ne pas prendre la translation en plus !!!! */
      CVector worldLookAt(m[8], m[9], m[10]);
      /* Vecteur X local de la bougie */
      CVector worldLookX(m[0], m[1], m[2]);

      CVector direction(-bougiepos.x, 0.0f, -bougiepos.z);

      direction.normalize ();
      /* Apparemment, pas besoin de le normaliser, on laisse pour le moment */
      worldLookAt.normalize ();
      worldLookX.normalize ();

      angle = -acos (direction * worldLookAt);
      angle2 = acos (direction * worldLookX);

      angle4 = (angle2 < PI / 2.0f) ? -angle : angle;

      glActiveTextureARB(GL_TEXTURE0_ARB);
      glEnable (GL_TEXTURE_2D);
      /****************************************************************************************/
      /* Affichage de la flamme */

      glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      m_tex->bind();

      glMatrixMode (GL_TEXTURE);
      glPushMatrix ();
      glLoadIdentity ();

      glTranslatef (angle4 / PI, 0.0f, 0.0f);

      drawNurbs();

      glPopMatrix();

      glMatrixMode (GL_MODELVIEW);

      glDisable (GL_TEXTURE_2D);
    }
}

/**********************************************************************************************************************/
/*************************************** IMPLEMENTATION DE LA CLASSE REALFLAME ****************************************/
/**********************************************************************************************************************/

RealFlame::RealFlame(uint nbSkeletons, ushort nbFixedCPoints,
		     const ITexture* const tex, Field3D* const s) :
  FixedFlame (nbSkeletons, nbFixedCPoints, tex)
{
  m_distances = new float[NB_PARTICLES_MAX - 1 + m_nbFixedCPoints];
  m_maxDistancesIndexes = new int[NB_PARTICLES_MAX - 1 + m_nbFixedCPoints];

  m_periSkeletons = new PeriSkeleton* [m_nbSkeletons];
  for (uint i = 0; i < m_nbSkeletons; i++)
    m_periSkeletons[i]=NULL;

  m_solver = s;
  m_lodSkelChanged = false;
  m_lodSkel = FULL_SKELETON;
  m_flat = false;
}

void RealFlame::computeVTexCoords()
{
  float vinc, vtmp;
  uint i;

  vinc = 1.0f / (float)(m_vsize-1);
  vtmp = 0.0;
  for (i = 0; i < m_vsize; i++)
    {
      m_texTmp[i] = vtmp;
      vtmp += vinc;
    }
}

bool RealFlame::build ()
{
  uint i, j, l;
  float utex;
  float dist_max;
  m_maxParticles = 0;
  m_count = 0;
  utex = 0.0f;

  /* Si un changement de niveau de détail a été demandé, l'effectuer maintenant */
  if(m_lodSkelChanged) changeSkeletonsLOD();

  /* Déplacement des squelettes guides */
  for (i = 0; i < m_nbLeadSkeletons; i++)
    m_leadSkeletons[i]->move ();

  /* Déplacement des squelettes périphériques et détermination du maximum de particules par squelette */
  for (i = 0; i < m_nbSkeletons; i++)
    {
      m_periSkeletons[i]->move ();
      if (m_periSkeletons[i]->getSize () > m_maxParticles)
	m_maxParticles = m_periSkeletons[i]->getSize ();
    }

  m_vsize = m_maxParticles + m_nbFixedCPoints;

  /* On connaît la taille maximale d'un squelette, on peut maintenant déterminer les coordonnées de texture en v */
  computeVTexCoords();

  /* Direction des u */
  for (i = 0; i < m_nbSkeletons; i++)
    {
      /* Problème pour la direction des v, le nombre de particules par squelettes n'est pas garanti */
      /* La solution retenue va ajouter des points de contrôles là où les points de contrôles sont le plus éloignés */
      if (m_periSkeletons[i]->getSize () < m_maxParticles)
	{
	  // Nombre de points de contrôle supplémentaires
	  uint nb_pts_supp = m_maxParticles - m_periSkeletons[i]->getSize ();
	  CPoint pt;
	  /* On calcule les distances entre les particules successives */
	  /* On prend également en compte l'origine du squelette ET les extrémités du guide */
	  /* On laisse les distances au carré pour des raisons évidentes de coût de calcul */
	  m_distances[0] =
	    m_periSkeletons[i]->getLeadSkeleton ()->getParticle (0)->squaredDistanceFrom (*m_periSkeletons[i]->getParticle (0));

	  for (j = 0; j < m_periSkeletons[i]->getSize () - 1; j++)
	    m_distances[j + 1] =
	      m_periSkeletons[i]->getParticle (j)->squaredDistanceFrom(*m_periSkeletons[i]->getParticle (j + 1));

	  m_distances[m_periSkeletons[i]->getSize ()] =
	    m_periSkeletons[i]->getLastParticle ()->squaredDistanceFrom (*m_periSkeletons[i]->getRoot ());
	  m_distances[m_periSkeletons[i]->getSize () + 1] =
	    m_periSkeletons[i]->getRoot()->squaredDistanceFrom(*m_periSkeletons[i]->getLeadSkeleton ()->getRoot ());

	  /* On cherche les indices des distances max */
	  /* On n'effectue pas un tri complet car on a seulement besoin de connaître les premiers */
	  for (l = 0; l < nb_pts_supp; l++)
	    {
	      dist_max = -FLT_MAX;
	      for (j = 0; j < m_periSkeletons[i]->getSize () + 2; j++)
		{
		  if (m_distances[j] > dist_max)
		    {
		      m_maxDistancesIndexes[l] = j;
		      dist_max = m_distances[j];
		    }
		}
	      /* Il n'y a plus de place */
	      if (dist_max == -FLT_MAX)
		m_maxDistancesIndexes[l] = -1;
	      else
		/* On met à la distance la plus grande pour ne plus la prendre en compte lors de la recherche suivante */
		m_distances[m_maxDistancesIndexes[l]] = 0;
	    }
	  /* Les particules les plus écartées sont maintenant connues, on peut passer à l'affichage */
	  /* Remplissage des points de contrôle */
	  setCtrlCPoint (m_periSkeletons[i]->getLeadSkeleton ()->getParticle (0), utex);

	  for (l = 0; l < nb_pts_supp; l++)
	    if (m_maxDistancesIndexes[l] == 0)
	      {
		pt = CPoint::pointBetween(m_periSkeletons[i]->getLeadSkeleton ()->getParticle (0), m_periSkeletons[i]->getParticle (0));
		setCtrlCPoint (&pt, utex);
	      }

	  for (j = 0; j < m_periSkeletons[i]->getSize () - 1; j++)
	    {
	      setCtrlCPoint (m_periSkeletons[i]->getParticle (j), utex);
	      /* On regarde s'il ne faut pas ajouter un point */
	      for (l = 0; l < nb_pts_supp; l++)
		{
		  if (m_maxDistancesIndexes[l] == (int)j + 1)
		    {
		      /* On peut référencer j+1 puisque normalement, m_maxDistancesIndexes[l] != j si j == m_periSkeletons[i]->getSize()-1 */
		      pt = CPoint::pointBetween(m_periSkeletons[i]->getParticle (j), m_periSkeletons[i]->getParticle (j + 1));
		      setCtrlCPoint (&pt, utex);
		    }
		}
	    }

	  setCtrlCPoint (m_periSkeletons[i]->getLastParticle (), utex);

	  for (l = 0; l < nb_pts_supp; l++)
	    if (m_maxDistancesIndexes[l] == (int)m_periSkeletons[i]->getSize ())
	      {
		pt = CPoint::pointBetween(m_periSkeletons[i]->getRoot (), m_periSkeletons[i]-> getLastParticle ());
		setCtrlCPoint (&pt, utex);
	      }

	  setCtrlCPoint (m_periSkeletons[i]->getRoot (), utex);

	  bool prec = false;

	  for (l = 0; l < nb_pts_supp; l++)
	    if (m_maxDistancesIndexes[l] == (int)m_periSkeletons[i]->getSize () + 1)
	      {
		pt = CPoint::pointBetween(m_periSkeletons[i]->getRoot (),
					 m_periSkeletons[i]->getLeadSkeleton()->getRoot ());
		setCtrlCPoint (&pt, utex);
		prec = true;
	      }

	  /* CPoints supplémentaires au cas où il n'y a "plus de place" ailleurs entre les particules */
	  for (l = 0; l < nb_pts_supp; l++)
	    if (m_maxDistancesIndexes[l] == -1)
	      {
		if (!prec)
		  {
		    pt = *m_periSkeletons[i]-> getRoot ();
		  }
		pt = CPoint::pointBetween (&pt, m_periSkeletons[i]->getLeadSkeleton()->getRoot ());
		setCtrlCPoint (&pt, utex);
		prec = true;
	      }
	  setCtrlCPoint (m_periSkeletons[i]->getLeadSkeleton ()->getRoot (), utex);
	}
      else
	{
	  /* Cas sans problème */
	  /* Remplissage des points de contrôle */
	  setCtrlCPoint (m_periSkeletons[i]->getLeadSkeleton ()->getParticle (0), utex);
	  for (j = 0; j < m_periSkeletons[i]->getSize (); j++)
	    {
	      setCtrlCPoint (m_periSkeletons[i]->getParticle (j), utex);
	    }
	  setCtrlCPoint (m_periSkeletons[i]->getRoot (), utex);
	  setCtrlCPoint (m_periSkeletons[i]->getLeadSkeleton ()->getRoot (), utex);
	}
      m_texTmp = m_texTmpSave;
      utex += m_utexInc;
    }

  /* On recopie les (m_uorder-1) squelettes pour fermer la NURBS */
  GLfloat *startCtrlCPoints = m_ctrlCPointsSave;
  for (i = 0; i < (m_uorder-1)*m_vsize*3; i++)
    *m_ctrlCPoints++ = *startCtrlCPoints++;
  m_ctrlCPoints = m_ctrlCPointsSave;

  /* Il faut également recopier les coordonnées de texture mais attention le u change ! */
  for (i = 0; i < (uint)m_uorder-1; i++){
    for (j = 0; j < m_vsize; j++){
      *m_texCPoints++ = utex;
      *m_texCPoints++ = *m_texTmp++;
    }
    m_texTmp = m_texTmpSave;
    utex += m_utexInc;
  }
  m_texCPoints = m_texCPointsSave;

  /* Affichage en NURBS */
  m_uknotsCount = m_nbSkeletons + m_uorder + m_uorder - 1;
  m_vknotsCount = m_vsize + m_vorder;

  /* Vecteur nodal en u strictement croissant pour fermer la NURBS ex: 0 1 2 3 4 5 */
  for (i = 0; i < m_uknotsCount; i++)
    m_uknots[i] = (float)i;

  /* Vecteur nodal en v uniforme ex: 0 0 0 0 1 2 3 4 5 6 6 6 6 */
  for (j = 0; j < m_vorder; j++)
    m_vknots[j] = 0.0f;

  for (j = m_vorder; j < m_vsize; j++)
    m_vknots[j] = m_vknots[j-1]+1;
  /* Adoucit la jointure entre le haut des squelettes périphériques et le */
  /* haut du squelette guide ex: 0 0 0 0 1.9 2 3 4 5 6 6 6 6 */
  m_vknots[m_vorder] += .9f;

  m_vknots[m_vknotsCount-m_vorder] =  m_vknots[m_vknotsCount-m_vorder-1]+1;
  for (j = m_vsize+1; j < m_vknotsCount; j++)
    m_vknots[j] = m_vknots[j-1];

//    for (j = 0; j < m_vknotsCount; j++)
//      cerr << m_vknots[j] << " ";
//    cerr << endl;

  if(m_vsize*m_nbSkeletons != m_count)
     cerr << "error " << m_vsize*m_nbSkeletons << " " << m_count << endl;

  return true;
}

RealFlame::~RealFlame()
{
  for (uint i = 0; i < m_nbSkeletons; i++)
    delete m_periSkeletons[i];
  delete[]m_periSkeletons;

  for (vector < LeadSkeleton * >::iterator skeletonsIterator = m_leadSkeletons.begin ();
       skeletonsIterator != m_leadSkeletons.end (); skeletonsIterator++)
    delete (*skeletonsIterator);
  m_leadSkeletons.clear ();

  delete[]m_distances;
  delete[]m_maxDistancesIndexes;
}
