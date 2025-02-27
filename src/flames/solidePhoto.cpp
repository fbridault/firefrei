#include "solidePhoto.hpp"

PixelLightingRenderer::PixelLightingRenderer(	const Scene* const a_pScene,
																						const vector <FireSource *> *a_pvpFlames,
																						const string& a_strMacro) :
	m_oShader("pixelLighting.vp", "pixelLighting.fp", a_strMacro)
{
  m_scene = a_pScene;
  m_flames = a_pvpFlames;

  m_centers = new GLfloat[m_flames->size()*3];
  m_intensities = new GLfloat[m_flames->size()];
  m_lazimuth_lzenith = new GLfloat[m_flames->size()*2];
}

PixelLightingRenderer::PixelLightingRenderer(	const Scene* const a_pScene,
																						const vector <FireSource *> *a_pvpFlames,
																						const string& a_strFragmentProgram,
																						const string& a_strMacro) :
	m_oShader("pixelLighting.vp", a_strFragmentProgram, a_strMacro)
{
  m_scene = a_pScene;
  m_flames = a_pvpFlames;

  m_centers = new GLfloat[m_flames->size()*3];
  m_intensities = new GLfloat[m_flames->size()];
  m_lazimuth_lzenith = new GLfloat[m_flames->size()*2];
}

PixelLightingRenderer::~PixelLightingRenderer()
{
  delete [] m_centers;
  delete [] m_intensities;
  delete [] m_lazimuth_lzenith;
}

void PixelLightingRenderer::draw(bool color)
{
  uint k=0;
  /* R�cup�ration des propri�t�s des flammes */
  for (vector < FireSource* >::const_iterator flamesIterator = m_flames->begin ();
       flamesIterator != m_flames->end (); flamesIterator++, k++)
    {
      (*flamesIterator)->computeIntensityPositionAndDirection();

      m_intensities[k] = (*flamesIterator)->getIntensity();
      (*flamesIterator)->getCenterSP(m_centers[k*3], m_centers[k*3+1], m_centers[k*3+2]);
    }

  /* Affichage du solide modul� avec la couleur des objets  */
  m_oShader.Enable();
  m_oShader.SetUniform3fv("centreSP", m_centers, m_flames->size());
  m_oShader.SetUniform1fv("fluctuationIntensite", m_intensities, m_flames->size());
  m_oShader.SetUniform1i("isTextured",1);
  m_oShader.SetUniform3f("lumTr", 0.0f, 0.0f, 0.0f);
  m_oShader.SetUniform4f("scale", 1.0f, 1.0f, 1.0f, 1.0f);

  m_oShader.SetUniform1i("textureObjet",0);
  m_scene->drawSceneTEX();
  m_oShader.SetUniform1i("isTextured",0);
  m_scene->drawSceneWTEX(&m_oShader);
  m_oShader.Disable();
}

/***************************************************************************************************************/
PhotometricSolidsRenderer::PhotometricSolidsRenderer(	const Scene* const s,
																												const vector <FireSource *> *flames,
																												const string& a_strMacro) :
  PixelLightingRenderer(s, flames, "photoSolid.fp", a_strMacro),
  m_oSPOnlyShader("pixelLighting.vp", "photoSolidOnly.fp", a_strMacro)
{
  generateTexture();
}

PhotometricSolidsRenderer::~PhotometricSolidsRenderer()
{
  delete m_photometricSolidsTex;
}

void PhotometricSolidsRenderer::generateTexture(void)
{
  uint tmp, sav, p;
  uint sizex, sizey;
  GLfloat *tex3DValues, *ptrTex;

  m_tex2DSize[0] = m_tex2DSize[1] = 0;
  /* Calcul de la taille maximale */
  for (vector < FireSource* >::const_iterator flamesIterator = m_flames->begin ();
       flamesIterator != m_flames->end (); flamesIterator++){
    if((*flamesIterator)->getIESAzimuthSize() > m_tex2DSize[0])
      m_tex2DSize[0] = (*flamesIterator)->getIESAzimuthSize();
    if((*flamesIterator)->getIESZenithSize() > m_tex2DSize[1])
      m_tex2DSize[1] = (*flamesIterator)->getIESZenithSize();
  }

  /* On prend la puissance de deux sup�rieure */
  sav = tmp = m_tex2DSize[0];
  p = 0;
  while(tmp > 0){
    tmp=tmp>>1; p++;
  }
 m_tex2DSize[0] = (uint)pow(2,p-1);
  if(sav != m_tex2DSize[0])
    m_tex2DSize[0] = m_tex2DSize[0] << 1;

  sav = tmp = m_tex2DSize[1];
  p = 0;
  while(tmp > 0){
    tmp=tmp>>1; p++;
  }
  m_tex2DSize[1] = (uint)pow(2,p-1);
  if(sav != m_tex2DSize[1])
    m_tex2DSize[1] = m_tex2DSize[1] << 1;

  cout << "Texture 3D size : " << m_tex2DSize[0] << " " << m_tex2DSize[1] << endl;

  tex3DValues = new GLfloat[m_tex2DSize[0]*m_tex2DSize[1]*m_flames->size()];
  ptrTex = tex3DValues;

  for (vector < FireSource* >::const_iterator flamesIterator = m_flames->begin ();
       flamesIterator != m_flames->end (); flamesIterator++){
    GLfloat *values = (*flamesIterator)->getIntensities();

    sizex = (*flamesIterator)->getIESAzimuthSize();
    sizey = (*flamesIterator)->getIESZenithSize();

    /* On cr�e la texture 3D en remplissant de 0 l� o� on n'a pas de valeur */
    for(uint j=0; j < sizey; j++){

      for(uint i=0; i < sizex; i++)
	*ptrTex++ = *values++;
      for(uint i=sizex; i < m_tex2DSize[0]; i++)
	*ptrTex++ = 0;

    }

    for(uint j=sizey; j < m_tex2DSize[1]; j++)
      for(uint i=0; i < m_tex2DSize[0]; i++)
	*ptrTex++ = 0;
  }

//    ptrTex = tex3DValues;
//    for(uint k=0; k < m_flames->size(); k++){
//      for(uint j=0; j < m_tex2DSize[1]; j++){
//        for(uint i=0; i < m_tex2DSize[0]; i++){
//  	cerr << *ptrTex++ << " ";
//        }
//        cerr << endl;
//      }
//      cerr << "Fin 2D" << endl;
//    }

  m_photometricSolidsTex = new CTexture3D((GLsizei)m_tex2DSize[0], (GLsizei)m_tex2DSize[1], (GLsizei)m_flames->size(), tex3DValues);

  uint k=0;
  for (vector < FireSource* >::const_iterator flamesIterator = m_flames->begin ();
       flamesIterator != m_flames->end (); flamesIterator++, k++){
    /* On prend l'inverse pour �viter une division dans le shader et on divise par la taille */
    /* pour avoir sur l'intervalle [O:1] dans l'espace de la texture */
    /* Le calcul final d'un uv dans le shader est le suivant : */
    /* phi / m_lazimuth_lzenith[i].x / m_tex2Dsize[0] */
    /* Ce qui est simplifi� gr�ce au traitement effectu� ici en : */
    /* phi * m_lazimuth_lzenith[i].x */
    m_lazimuth_lzenith[k*2] = (1/(*flamesIterator)->getLazimutTEX())/m_tex2DSize[0];
    m_lazimuth_lzenith[k*2+1] = (1/(*flamesIterator)->getLzenithTEX())/m_tex2DSize[1];
//     cerr << m_lazimuth_lzenith[k*2] << " " << m_lazimuth_lzenith[k*2+1] << endl;
  }

  delete [] tex3DValues;
}

void PhotometricSolidsRenderer::draw(bool color)
{
  uint k=0;
  /* R�cup�ration des propri�t�s des flammes */
  for (vector < FireSource* >::const_iterator flamesIterator = m_flames->begin ();
       flamesIterator != m_flames->end (); flamesIterator++, k++)
    {
      (*flamesIterator)->computeIntensityPositionAndDirection();

      m_intensities[k] = (*flamesIterator)->getIntensity();
      (*flamesIterator)->getCenterSP(m_centers[k*3], m_centers[k*3+1], m_centers[k*3+2]);
    }

  /* Affichage du solide seul */
  if(!color){
    m_oSPOnlyShader.Enable();
    m_oSPOnlyShader.SetUniform3fv("centreSP", m_centers, m_flames->size());
    m_oSPOnlyShader.SetUniform1fv("fluctuationIntensite", m_intensities, m_flames->size());
    m_oSPOnlyShader.SetUniform2fv("angles",m_lazimuth_lzenith, m_flames->size());
    m_oSPOnlyShader.SetUniform1f("incr", m_flames->size() > 1 ? 1/(m_flames->size()-1) : 0.0f);
    m_oSPOnlyShader.SetUniform3f("lumTr", 0.0f, 0.0f, 0.0f);
    m_oSPOnlyShader.SetUniform3f("scale", 1.0f, 1.0f, 1.0f);

    glActiveTexture(GL_TEXTURE0);
    m_photometricSolidsTex->bind();
    m_oSPOnlyShader.SetUniform1i("textureSP",0);
    /* Dessin en enlevant toutes les textures si n�cessaire */
    m_scene->drawSceneWT(&m_oSPOnlyShader);
    m_oSPOnlyShader.Disable();
  }else{
    /* Affichage du solide modul� avec la couleur des objets  */
    m_oShader.Enable();
    m_oShader.SetUniform3fv("centreSP", m_centers, m_flames->size());
    m_oShader.SetUniform1fv("fluctuationIntensite", m_intensities, m_flames->size());
    m_oShader.SetUniform2fv("angles", m_lazimuth_lzenith, m_flames->size());
    m_oShader.SetUniform1f("incr", m_flames->size() > 1 ? 1/(m_flames->size()-1) : 0.0f);
    m_oShader.SetUniform1i("isTextured",1);
    m_oShader.SetUniform3f("lumTr", 0.0f, 0.0f, 0.0f);
    m_oShader.SetUniform4f("scale", 1.0f, 1.0f, 1.0f, 1.0f);

    glActiveTexture(GL_TEXTURE1);
    m_photometricSolidsTex->bind();
    m_oShader.SetUniform1i("textureSP",1);
    m_oShader.SetUniform1i("textureObjet",0);
    m_scene->drawSceneTEX();
    m_oShader.SetUniform1i("isTextured",0);
    m_scene->drawSceneWTEX(&m_oShader);
    m_oShader.Disable();
  }
}
