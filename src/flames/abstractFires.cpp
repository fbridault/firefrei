#include "abstractFires.hpp"

#include "../scene/scene.hpp"

/**********************************************************************************************************************/
/************************************** IMPLEMENTATION DE LA CLASSE FLAMELIGHT ****************************************/
/**********************************************************************************************************************/

FlameLight::FlameLight(const Scene* const a_scene, uint index, const CShader& a_rSVShader, const char* const IESFilename) :
	m_orientationSPtheta(0.0f), m_scene(a_scene), m_rSVShader(a_rSVShader)
{
  switch(index){
  case 0 : m_light = GL_LIGHT0; break;
  case 1 : m_light = GL_LIGHT1; break;
  case 2 : m_light = GL_LIGHT2; break;
  case 3 : m_light = GL_LIGHT3; break;
  case 4 : m_light = GL_LIGHT4; break;
  case 5 : m_light = GL_LIGHT5; break;
  case 6 : m_light = GL_LIGHT6; break;
  case 7 : m_light = GL_LIGHT7; break;
  default : m_light = GL_LIGHT0; break;
  }

  m_lightPosition[3] = 1.0f;

  m_iesFile = new IES(IESFilename);
}

FlameLight::~FlameLight()
{
  delete m_iesFile;
}

void FlameLight::switchOff()
{
  glDisable(m_light);
}

void FlameLight::switchOn()
{
  float coef = 3.0f*m_intensity;
//   GLfloat val_diffuse[]={1,1,1,1.0};
  GLfloat val_diffuse[]={1.0f*coef,0.5f*coef,0.0f,1.0f};
  //GLfloat val_ambiant[]={0.05*coef,0.05*coef,0.05*coef,1.0};
  GLfloat val_null[]={0.0f,0.0f,0.0f,1.0f};
  GLfloat val_specular[]={.5f*coef,.5f*coef,.5f*coef,1.0f};

  /* Définition de l'intensité lumineuse de chaque flamme en fonction de la hauteur de celle-ci */
  glLightfv(m_light,GL_POSITION,m_lightPosition);
  glLightfv(m_light,GL_DIFFUSE,val_diffuse);
  glLightfv(m_light,GL_SPECULAR,val_specular);
  glLightfv(m_light,GL_AMBIENT,val_null);
  glLightf(m_light,GL_QUADRATIC_ATTENUATION,0.05f);
  glEnable(m_light);
}

void FlameLight::drawShadowVolume (GLfloat fatness[4], GLfloat extrudeDist[4])
{
  m_rSVShader.Enable();
  m_rSVShader.SetUniform4f("LightPos",m_lightPosition);
  m_rSVShader.SetUniform4f("Fatness",fatness);
  m_rSVShader.SetUniform4f("ShadowExtrudeDist",extrudeDist);

  m_scene->drawSceneWSV();
  m_rSVShader.Disable();
}

void FlameLight::computeGlowWeights(uint index, float sigma)
{
  int offset;
  m_glowDivide[index] = 0.0f;

  offset = FILTER_SIZE/2;
  for(int x=-offset ; x<=offset ; x++){
    m_glowWeights[index][x+offset] = expf(-(x*x)/(sigma*sigma));
    m_glowDivide[index] += m_glowWeights[index][x+offset];
    //    cerr << x << " " << x+offset << " " << m_glowWeights[index][x+offset] << endl;
  }
  //     cerr << m_divide[index] << endl;
  m_glowDivide[index] = 1/m_glowDivide[index];
}

/**********************************************************************************************************************/
/************************************** IMPLEMENTATION DE LA CLASSE FIRESOURCE ****************************************/
/**********************************************************************************************************************/

FireSource::FireSource(const FlameConfig& flameConfig, Field3D* const s, uint nbFlames, Scene* const scene,
		       const wxString &texname, uint index, const CShader& a_rShader) :
  FlameLight(scene, index, a_rShader, flameConfig.IESFileName.ToAscii()),
  m_texture(string(texname.fn_str()), GL_REPEAT, GL_REPEAT),
  m_position(0.0f,0.0f,0.0f)
{
  m_solver = s;

  m_nbFlames=nbFlames;
  /* Si le tableau n'est pas initialisé par le constructeur d'une sous-classe, on le fait ici */
  if(m_nbFlames) m_flames = new RealFlame* [m_nbFlames];

  m_intensityCoef = 0.3f;
  m_visibility = true;
  m_dist=0;
  buildBoundingSphere();
  m_flickSave=-1;
  m_fluidsLODSave=15;
  m_nurbsLODSave=-1;

  computeGlowWeights(0,3.0f);
  computeGlowWeights(1,10.0f);
}

FireSource::~FireSource()
{
  for (uint i = 0; i < m_nbFlames; i++)
    delete m_flames[i];
  delete[]m_flames;
}

void FireSource::build()
{
  CPoint averagePos;
  CVector averageVec;

  if(!m_visibility) return;

  for (uint i = 0; i < m_nbFlames; i++){
    m_flames[i]->build();
    averagePos += m_flames[i]->getCenter ();
    averageVec += m_flames[i]->getMainDirection ();
  }
  averagePos *= m_solver->getScale();
  m_center = averagePos/m_nbFlames;
  averagePos = m_center + getPosition();
  setLightPosition(averagePos);

  m_direction = averageVec/m_nbFlames;
}

void FireSource::computeIntensityPositionAndDirection()
{
  //  float r,y;

  CVector o = getMainDirection();

  // l'intensité est calculée à partir du rapport de la longueur de la flamme (o)
  // et de la taille en y de la grille fois un coeff correcteur
  m_intensity=o.length()*(m_solver->getScale().y)*m_intensityCoef;

   //  m_intensity = log(m_intensity)/6.0+1;
//   m_intensity = sin(m_intensity * PI/2.0);
  /* Fonction de smoothing pour éviter d'avoir trop de fluctuation */
  m_intensity = sqrt(m_intensity);

  // l'axe de rotation est dans le plan x0z perpendiculaire aux coordonnées
  // de o projeté perpendiculairement dans ce plan
//   m_axeRotation.set(-o.z,0.0,o.x);

//   // l'angle de rotation theta est la coordonnée sphérique correspondante
//   y=o.y;
//   r = (float)o.length();
//   if(r - fabs(y) < EPSILON)
//     m_orientationSPtheta = 0.0;
//   else
//     m_orientationSPtheta=acos(y / r)*180.0/M_PI;
}

void FireSource::buildBoundingSphere ()
{
  CPoint p;
  float t;
  p = (m_solver->getScale() * m_solver->getDim());
  t = p.max();

  m_boundingSphere.radius = sqrtf(3.0f)/2.0f*t;
  /* Augmentation de 10% du rayon pour prévenir l'apparition des flammes */
//   m_boundingSphere.radius *= 1.1;
  m_boundingSphere.centre = getPosition() + p/2.0f;
  //  m_boundingSphere.radius = ((getMainDirection()-getCenter()).scaleBy(m_solver->getScale())).length()+.1;
  //  m_boundingSphere.centre = getCenterSP();
}

void FireSource::drawImpostor() const
{
  if(m_visibility)
    {
//       CPoint position(getPosition());
//       CPoint scale(m_solver->getScale());
//       glPushMatrix();
//       glTranslatef (position.x, position.y, position.z);
//       glScalef (scale.x, scale.y, scale.z);
//       CUGraphicsFn::SolidBox(CPoint(),m_solver->getDim());
//       glPopMatrix();
      GLfloat modelview[16];

      CPoint pos(getPosition());
      float size=m_solver->getScale().x*1.5f, halfSize=m_solver->getScale().x*.5f;
      CPoint a,b,c,d,zero;
      CVector right,up,offset;

      glGetFloatv (GL_MODELVIEW_MATRIX, modelview);

      offset = CVector(modelview[2], modelview[6], modelview[10])*m_solver->getDim().z*m_solver->getScale().z/2.0f;

      right.x = modelview[0];
      right.y = modelview[4];
      right.z = modelview[8];

      up.x = modelview[1];
      up.y = modelview[5];
      up.z = modelview[9];

      a = pos - right * (size * 0.5f);
      b = pos + right * size * 0.5f;
      c = pos + right * size * 0.5f + up * size;
      d = pos - right * size * 0.5f + up * size;

      glPushMatrix();
      glColor3f(1.0f,1.0f,1.0f);
      glTranslatef(offset.x,offset.y,offset.z);
      glBegin(GL_QUADS);
      glVertex3f(a.x+halfSize, a.y, a.z+halfSize);
      glVertex3f(b.x+halfSize, b.y, b.z+halfSize);
      glVertex3f(c.x+halfSize, c.y, c.z+halfSize);
      glVertex3f(d.x+halfSize, d.y, d.z+halfSize);
      glEnd();
      glPopMatrix();
    }
}

void FireSource::computeVisibility(const Camera &view, bool forceSpheresBuild)
{
  bool vis_save=m_visibility;
  int fluidsLOD, nurbsLOD;
  float glow1LOD, glow2LOD;
  float coverage;

  if(forceSpheresBuild)
    buildBoundingSphere();

  m_dist=m_boundingSphere.visibleDistance(view);
  m_visibility = (m_dist);
  if(m_visibility){
    if(!vis_save){
      cerr << "solver " << m_light - GL_LIGHT0 << " launched" << endl;
      m_solver->setRunningState(true);
    }

    coverage = m_boundingSphere.getPixelCoverage(view);

    /* Fonction obtenue par régression linéaire avec les données
     * y = [.60 .25 .05 .025 .015 .01 .001] et x = [15 13 11 9 7 5 3]
     */
    fluidsLOD = (int)nearbyint(2.0870203*log(coverage*2399.4418));

    if(coverage > .75f) nurbsLOD = 5;
    else if(coverage > .2f) nurbsLOD = 4;
    else if(coverage > .1f) nurbsLOD = 3;
    else if(coverage > .003f) nurbsLOD = 2;
    else if(coverage > .0015f) nurbsLOD = 1;
    else nurbsLOD = 0;

    glow1LOD = 0.3134922*log(coverage*11115.586);
    glow2LOD = 1.5543804*log(coverage*840.01981);

    /* Fonction obtenue par régression linéaire avec les données
     */
    //     nurbsLOD = (int)nearbyint(1.116488*log(coverage*68.271493));
    //cout << "coverage " << coverage << " " << fluidsLOD << " " << nurbsLOD << " " << glow1LOD << " " << glow2LOD << endl;

    computeGlowWeights(0,glow1LOD);
    computeGlowWeights(1,glow2LOD);

    // Changement de niveau pour les fluides
    if(fluidsLOD < m_fluidsLODSave)
      {
	do
	  {
	    /* On passe en FakeField */
	    if( fluidsLOD == 5 ){
	      m_solver->switchToFakeField();
	    }
	    m_fluidsLODSave-=1;
	  }
	while(fluidsLOD < m_fluidsLODSave);
      }
    else
      if(fluidsLOD > m_fluidsLODSave)
	{
	  do
	    {
	      /* On repasse en solveur */
	      if( fluidsLOD == 6 )
		{
		  m_solver->switchToRealSolver();
		}
	      m_fluidsLODSave+=1;
	    }
	  while(fluidsLOD > m_fluidsLODSave);
	}

    // Changement de niveau pour les NURBS
    if(nurbsLOD != m_nurbsLODSave)
      {
	setLOD(nurbsLOD);
	m_nurbsLODSave=nurbsLOD;
      }
  }else
    if(vis_save){
      cerr << "solver " << m_light - GL_LIGHT0 << " stopped" << endl;
      m_solver->setRunningState(false);
    }
}

bool FireSource::operator<(const FireSource& other) const{
  return (m_dist < other.m_dist);
}

/**********************************************************************************************************************/
/******************************** IMPLEMENTATION DE LA CLASSE DETACHABLEFIRESOURCE ************************************/
/**********************************************************************************************************************/

DetachableFireSource::DetachableFireSource(const FlameConfig& flameConfig, Field3D* const s, uint nbFlames,
					   Scene* const scene,
					   const wxString &texname, uint index, const CShader& a_rShader) :
  FireSource (flameConfig, s, nbFlames, scene, texname, index, a_rShader)
{
  computeGlowWeights(0,1.8f);
  computeGlowWeights(1,2.2f);
}

DetachableFireSource::~DetachableFireSource()
{
  for (list < DetachedFlame* >::iterator flamesIterator = m_detachedFlamesList.begin ();
       flamesIterator != m_detachedFlamesList.end();  flamesIterator++)
    delete (*flamesIterator);
  m_detachedFlamesList.clear ();
}

void DetachableFireSource::drawFlame(bool display, bool displayParticle, u_char boundingVolume) const
{
#ifdef COUNT_NURBS_POLYGONS
  g_count=0;
#endif
  switch(boundingVolume){
  case 1 : drawBoundingSphere(); break;
  case 2 : drawImpostor(); break;
  default :
    if(m_visibility)
      {
	CPoint pt(getPosition());
	CPoint scale(m_solver->getScale());
	glPushMatrix();
	glTranslatef (pt.x, pt.y, pt.z);
	glScalef (scale.x, scale.y, scale.z);
	for (uint i = 0; i < m_nbFlames; i++)
	  m_flames[i]->drawFlame(display, displayParticle);
	for (list < DetachedFlame* >::const_iterator flamesIterator = m_detachedFlamesList.begin ();
	     flamesIterator != m_detachedFlamesList.end();  flamesIterator++)
	  (*flamesIterator)->drawFlame(display, displayParticle);
	glPopMatrix();
      }
    break;
  }
#ifdef COUNT_NURBS_POLYGONS
  cerr << g_count << endl;
#endif
}

void DetachableFireSource::build()
{
  CPoint averagePos, tmp;
  CVector averageVec;
  DetachedFlame* flame;

  if(!m_visibility) return;

  for (uint i = 0; i < m_nbFlames; i++){
    m_flames[i]->breakCheck();
    m_flames[i]->build();
    averagePos += m_flames[i]->getCenter ();
    averageVec += m_flames[i]->getMainDirection ();
  }
  averagePos *= m_solver->getScale();
  m_center = averagePos/m_nbFlames;
  averagePos = m_center + getPosition();
  setLightPosition(averagePos);

  m_direction = averageVec/m_nbFlames;

  /* Destruction des flammes détachées en fin de vie */
  list < DetachedFlame* >::iterator flamesIterator = m_detachedFlamesList.begin ();
  while( flamesIterator != m_detachedFlamesList.end()){
    if(!(*flamesIterator)->build()){
      flame = *flamesIterator;
      flamesIterator = m_detachedFlamesList.erase(flamesIterator);
      delete flame;
    }else
      flamesIterator++;
  }

  CPoint ptMax(-FLT_MAX, -FLT_MAX, -FLT_MAX), ptMin(FLT_MAX, FLT_MAX, FLT_MAX);
  CPoint pt;
  CPoint p;
  float t,k;
  p = (m_solver->getScale() * m_solver->getDim())/2.0f;
  t = p.max();
  k = t*t;

  /* Calcul de la bounding sphere pour les flammes détachées */
//   if( m_detachedFlamesList.size () )
//     for (list < DetachedFlame* >::const_iterator flamesIterator = m_detachedFlamesList.begin ();
// 	 flamesIterator != m_detachedFlamesList.end();  flamesIterator++){
//       pt = (*flamesIterator)->getTop();
//       if(pt.x > ptMax.x)
// 	ptMax.x = pt.x;
//       if(pt.y > ptMax.y)
// 	ptMax.y = pt.y;
//       if(pt.z > ptMax.z)
// 	ptMax.z = pt.z;
//       pt = (*flamesIterator)->getBottom();
//       if(pt.x < ptMin.x)
//       ptMin.x = pt.x;
//       if(pt.y < ptMin.y)
// 	ptMin.y = pt.y;
//       if(pt.z < ptMin.z)
// 	ptMin.z = pt.z;
//       ptMin *= m_solver->getScale();
//       ptMax *= m_solver->getScale();
//       m_boundingSphere.radius = (sqrt(k+k) + ptMax.distance(ptMin));
//       m_boundingSphere.centre = m_solver->getPosition() + (p + (ptMax + ptMin)/2.0f)/2.0f;
//     }else{
    m_boundingSphere.radius = sqrt(k+k);
    m_boundingSphere.centre = getPosition() + p;
//   }
    /* Calcul de la bounding box pour affichage */
    buildBoundingBox ();
}

// void DetachableFireSource::drawImpostor() const
// {
//   if(m_visibility)
//     {
//       GLfloat modelview[16];

//       CPoint pos(m_solver->getPosition());
//       float size=m_solver->getScale().x*1.1;
//       CPoint a,b,c,d,zero;
//       CVector right,up,offset;//(0.0f,0.0f,0.5f);

//       glGetFloatv (GL_MODELVIEW_MATRIX, modelview);
//       offset = CVector(modelview[2], modelview[6], modelview[10])*.5f*size;

//       right.x = modelview[0];
//       right.y = modelview[4];
//       right.z = modelview[8];

//       up.x = modelview[1];
//       up.y = modelview[5];
//       up.z = modelview[9];

//       a = pos - right * (size * 0.5f);
//       b = pos + right * size * 0.5f;
//       c = pos + right * size * 0.5f + up * size;
//       d = pos - right * size * 0.5f + up * size;

//       glPushMatrix();
//       glColor3f(1.0f,1.0f,1.0f);
//       glTranslatef(offset.x,offset.y,offset.z);
//       glBegin(GL_QUADS);
//       glVertex3f(a.x+0.5f, a.y, a.z+0.5f);
//       glVertex3f(b.x+0.5f, b.y, b.z+0.5f);
//       glVertex3f(c.x+0.5f, c.y, c.z+0.5f);
//       glVertex3f(d.x+0.5f, d.y, d.z+0.5f);
//       glEnd();
//       glPopMatrix();
//     }
// }

void DetachableFireSource::setSmoothShading (bool state)
{
  FireSource::setSmoothShading(state);
  for (list < DetachedFlame* >::const_iterator flamesIterator = m_detachedFlamesList.begin ();
       flamesIterator != m_detachedFlamesList.end();  flamesIterator++)
    (*flamesIterator)->setSmoothShading(state);
}

void DetachableFireSource::computeIntensityPositionAndDirection()
{
  CVector o = getMainDirection();

  // l'intensité est calculée à partir du rapport de la longueur de la flamme (o)
  // et de la taille en y de la grille fois un coeff correcteur
  m_intensity=o.length()*(m_solver->getScale().y)*m_intensityCoef;

  /* Fonction de smoothing pour éviter d'avoir trop de fluctuation */
  m_intensity = sqrt(m_intensity)*2.0f;

}

void DetachableFireSource::computeVisibility(const Camera &view, bool forceSpheresBuild)
{
  bool vis_save=m_visibility;
  int nurbsLOD, fluidsLOD;
  float glow1LOD, glow2LOD;
  float coverage;

  if(forceSpheresBuild)
    buildBoundingSphere();

  m_dist=m_boundingSphere.visibleDistance(view);
  m_visibility = (m_dist);

  if(m_visibility){
    if(!vis_save){
      cerr << "solver " << m_light - GL_LIGHT0 << " launched" << endl;
      m_solver->setRunningState(true);
    }

    coverage = m_boundingSphere.getPixelCoverage(view);

    /* Fonction obtenue par régression linéaire avec les données
     * y = [.60 .25 .05 .025 .015 .01 .001] et x = [15 13 11 9 7 5 3]
     */
    fluidsLOD = (int)nearbyint(2.0870203*log(coverage*2399.4418));

    if(fluidsLOD < 5) fluidsLOD=5;
    else if(fluidsLOD > 15) fluidsLOD=15;

    if(coverage > .9f) nurbsLOD = 5;
    else if(coverage > .5f) nurbsLOD = 4;
    else if(coverage > .2f) nurbsLOD = 3;
    else if(coverage > .05f) nurbsLOD = 2;
    else if(coverage > .01f) nurbsLOD = 1;
    else nurbsLOD = 0;

    glow1LOD = 0.3134922*log(coverage*11115.586);
    glow2LOD = 1.5543804*log(coverage*840.01981);
    //     nurbsLOD = (int)nearbyint(1.0731832*log(coverage*54.470523));
    //cout << "coverage " << coverage << " " << fluidsLOD << " " << nurbsLOD << endl;

    computeGlowWeights(0,glow1LOD);
    computeGlowWeights(1,glow2LOD);

    if(fluidsLOD < m_fluidsLODSave)
      {
	int diff = (m_fluidsLODSave - fluidsLOD)/2;
	while(diff > 0 )
	  {
	    /* On passe en FakeField */
	    if( fluidsLOD == 5 && m_fluidsLODSave==7){
	      if(getPerturbateMode() != FLICKERING_NOISE){
		m_flickSave = getPerturbateMode();
		setPerturbateMode(FLICKERING_NOISE);
	      }
	      m_solver->switchToFakeField();
	    }else
	      m_solver->decreaseRes();
	    diff--; m_fluidsLODSave-=2;
	  }
      }
    else
      if(fluidsLOD > m_fluidsLODSave)
	{
	  int diff = (fluidsLOD - (m_fluidsLODSave-1))/2;

	  while(diff > 0 )
	    {
	      /* On passe en FakeField */
	      if( fluidsLOD == 6 && (m_fluidsLODSave-1)==4){
		if(m_flickSave > -1){
		  setPerturbateMode(m_flickSave);
		  m_flickSave = -1;
		}
		m_solver->switchToRealSolver();
	      }else
		m_solver->increaseRes();
	      diff--; m_fluidsLODSave+=2;
	    }
	}

    // Changement de niveau pour les NURBS
    if(nurbsLOD != m_nurbsLODSave)
      {
	setLOD(nurbsLOD);
	m_nurbsLODSave=nurbsLOD;
      }
  }else
    if(vis_save){
      cerr << "solver " << m_light - GL_LIGHT0 << " stopped" << endl;
      m_solver->setRunningState(false);
    }
}

void DetachableFireSource::buildBoundingBox ()
{
  CPoint ptMax(-FLT_MAX, -FLT_MAX, -FLT_MAX), ptMin(FLT_MAX, FLT_MAX, FLT_MAX);
  CPoint pt;
  CPoint pos(getPosition());

  if( m_detachedFlamesList.size () )
    for (list < DetachedFlame* >::const_iterator flamesIterator = m_detachedFlamesList.begin ();
	 flamesIterator != m_detachedFlamesList.end();  flamesIterator++){
      pt = (*flamesIterator)->getTop();
      if(pt.x > ptMax.x)
	ptMax.x = pt.x;
      if(pt.y > ptMax.y)
	ptMax.y = pt.y;
      if(pt.z > ptMax.z)
	ptMax.z = pt.z;
      pt = (*flamesIterator)->getBottom();
      if(pt.x < ptMin.x)
	ptMin.x = pt.x;
      if(pt.y < ptMin.y)
	ptMin.y = pt.y;
      if(pt.z < ptMin.z)
	ptMin.z = pt.z;
    }
  pt = m_solver->getDim();
  if(pt.x > ptMax.x)
    ptMax.x = pt.x;
  if(pt.y > ptMax.y)
    ptMax.y = pt.y;
  if(pt.z > ptMax.z)
    ptMax.z = pt.z;
  if(0.0f < ptMin.x)
    ptMin.x = 0.0f;
  if(0.0f < ptMin.y)
    ptMin.y = 0.0f;
  if(0.0f < ptMin.z)
    ptMin.z = 0.0f;

  m_BBmin = ptMin * m_solver->getScale();
  m_BBmax = ptMax * m_solver->getScale();
}
