#include "object.hpp"

#include "scene.hpp"
#include "material.hpp"

extern uint g_objectCount;

/**********************************************************************************************************************/
/********************************************** DEFINITION DE LA CLASSE OBJECT ****************************************/
/**********************************************************************************************************************/

Object::Object(const Scene* const scene)
{
	assert(scene != NULL);

	m_scene = scene;
	m_attributes = 0;
	glGenBuffers(1, &m_bufferID);
	m_lookupTable = NULL;
}

Object::~Object ()
{
	m_vertexArray.clear();

	for (list <Mesh* >::iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		delete (*meshesListIterator);
	m_meshesList.clear ();
	glDeleteBuffers(1, &m_bufferID);
	delete [] m_lookupTable;
}

void Object::buildBoundingBox ()
{
	CPoint ptMax(-FLT_MAX, -FLT_MAX, -FLT_MAX), ptMin(FLT_MAX, FLT_MAX, FLT_MAX);
	/* Cr�ation de la bounding box */

	for (vector < Vertex >::iterator vertexIterator = m_vertexArray.begin ();
	     vertexIterator != m_vertexArray.end (); vertexIterator++)
	{
		/* Calcul du max */
		if ( vertexIterator->x > ptMax.x)
			ptMax.x = vertexIterator->x;
		if ( vertexIterator->y > ptMax.y)
			ptMax.y = vertexIterator->y;
		if ( vertexIterator->z > ptMax.z)
			ptMax.z = vertexIterator->z;
		/* Calcul du min */
		if ( vertexIterator->x < ptMin.x)
			ptMin.x = vertexIterator->x;
		if ( vertexIterator->y < ptMin.y)
			ptMin.y = vertexIterator->y;
		if ( vertexIterator->z < ptMin.z)
			ptMin.z = vertexIterator->z;
	}
	m_max = ptMax;
	m_min = ptMin;
}

void Object::buildBoundingSpheres ()
{
	/* On r�alloue la table car le tableau peut contenir des nouveaux points
	   depuis l'appel � setUVsAndNormals() */
	allocLookupTable();
	for (list <Mesh* >::iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		(*meshesListIterator)->buildBoundingSphere();
}

void Object::drawBoundingSpheres () const
{
	m_scene->getMaterial(0)->apply();
	for (list <Mesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		(*meshesListIterator)->drawBoundingSphere();
}

void Object::computeVisibility(const Camera &view)
{
	for (list <Mesh* >::iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		(*meshesListIterator)->computeVisibility(view);
}

uint Object::getPolygonsCount () const
{
	uint count=0;
	for (list <Mesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		count += (*meshesListIterator)->getPolygonsCount();
	return count;
}

void Object::buildVBO()
{
	/* D�termination du type de donn�es d�crites � partir des maillages */
	for (list <Mesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		if ( (*meshesListIterator)->getAttributes() > m_attributes)
			m_attributes = (*meshesListIterator)->getAttributes();

	glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
	glBufferData(GL_ARRAY_BUFFER, m_vertexArray.size()*sizeof(Vertex), &m_vertexArray[0], GL_STATIC_DRAW);

	for (list <Mesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		(*meshesListIterator)->buildVBO();
	glBindBuffer(GL_ARRAY_BUFFER, 0 );
}

void Object::draw (char drawCode, bool tex, bool boundingSpheres) const
{
	/* On initialise le dernier mat�riau au premier de la liste, le mat�riau par d�faut */
	uint lastMaterialIndex=0;

	if (boundingSpheres){
		m_scene->getMaterial(0)->apply();
		for (list <Mesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
		     meshesListIterator != m_meshesList.end ();
		     meshesListIterator++)
			(*meshesListIterator)->drawBoundingSphere();
	}else{
		if (drawCode == AMBIENT)
			/* Dessiner avec le mat�riau par d�faut (pour tester les zones d'ombres par exemple) */
			m_scene->getMaterial(0)->apply();

		/* Parcours de la liste des meshes */
		for (list <Mesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
		     meshesListIterator != m_meshesList.end ();
		     meshesListIterator++)
			(*meshesListIterator)->draw(drawCode, tex, lastMaterialIndex);

		/* On d�sactive l'unit� de texture le cas �ch�ant */
		if (m_scene->getMaterial(lastMaterialIndex)->hasDiffuseTexture() && tex){
			glDisable(GL_TEXTURE_2D);
		}
	}
}

void Object::translate(const CVector& direction)
{
	for (vector < Vertex >::iterator vertexIterator = m_vertexArray.begin ();
	     vertexIterator != m_vertexArray.end (); vertexIterator++)
	{
		vertexIterator->x += direction.x;
		vertexIterator->y += direction.y;
		vertexIterator->z += direction.z;
	}
	/* On d�place aussi la bo�te englobante */
	m_max += direction;
	m_min += direction;
}


/**********************************************************************************************************************/
/********************************************** DEFINITION DE LA CLASSE MESH ******************************************/
/**********************************************************************************************************************/

Mesh::Mesh (const Scene* const scene, uint materialIndex, Object* parent)
{
	assert(scene  != NULL);
	assert(parent != NULL);

	m_scene = scene;
	m_attributes = 0;
	m_materialIndex = materialIndex;
	m_parent = parent;
	m_visibility = true;
	glGenBuffers(1, &m_bufferID);
}

Mesh::~Mesh ()
{
	m_indexArray.clear();
	glDeleteBuffers(1, &m_bufferID);
}

void Mesh::buildVBO() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexArray.size()*sizeof(GLuint), &m_indexArray[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0 );
}

void Mesh::buildBoundingSphere ()
{
	float dist;
	Vertex v;
	uint n=0;

	/* On initialise la table */
	m_parent->initLookupTable();

	for (uint i = 0; i < m_indexArray.size(); i++)
	{
		/* Il ne faut prendre un m�me point qu'une seule fois en compte. */
		if ( !m_parent->findRefInLookupTable( m_indexArray[i] ) )
		{
			v = m_parent->getVertex(m_indexArray[i]);
			m_parent->addRefInLookupTable( m_indexArray[i], i );
			m_boundingSphere.centre = (m_boundingSphere.centre*n + CPoint(v.x, v.y, v.z)) / (float)(n+1);
			n++;
		}
	}

	for (vector < GLuint >::iterator indexIterator = m_indexArray.begin ();
	     indexIterator != m_indexArray.end ();  indexIterator++)
	{
		CPoint p;
		v=m_parent->getVertex(*indexIterator);
		p=CPoint(v.x, v.y, v.z);
		dist=p.squaredDistanceFrom(m_boundingSphere.centre);
		if ( dist > m_boundingSphere.radius)
			m_boundingSphere.radius = dist;
	}
	m_boundingSphere.radius = sqrt(m_boundingSphere.radius);
	//cerr << "sphere de centre " << m_centre << " et de rayon " << m_radius << endl;
}

void Mesh::drawBoundingSphere() const
{
	m_boundingSphere.draw();
}

void Mesh::computeVisibility(const Camera &view)
{
	m_visibility = m_boundingSphere.isVisible(view);
//  if(m_visibility) g_objectCount++;
	return;
}

void Mesh::draw (char drawCode, bool tex, uint &lastMaterialIndex) const
{
	if (!m_visibility)
		return;

	if (drawCode == TEXTURED){
		/* Ne dessiner que si il y a une texture */
		if (!m_scene->getMaterial(m_materialIndex)->hasDiffuseTexture())
			return;
	}else
		if (drawCode == FLAT){
			/* Ne dessiner que si il n'y a pas de texture */
			if (m_scene->getMaterial(m_materialIndex)->hasDiffuseTexture())
				return;
		}

	if (drawCode != AMBIENT)
		if ( m_materialIndex != lastMaterialIndex){
			if (m_scene->getMaterial(m_materialIndex)->hasDiffuseTexture() && tex){
				/* Inutile de r�activer l'unit� de texture si le mat�riau pr�c�dent en avait une */
				if (!m_scene->getMaterial(lastMaterialIndex)->hasDiffuseTexture()){
					glActiveTextureARB(GL_TEXTURE0_ARB);
					glEnable(GL_TEXTURE_2D);
					glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_MODULATE);
				}
				m_scene->getMaterial(m_materialIndex)->getDiffuseTexture()->bind();
			}else
				if (m_scene->getMaterial(lastMaterialIndex)->hasDiffuseTexture() && tex){
					/* Pas de texture pour le mat�riau courant, on d�sactive l'unit� de
					 * texture car le mat�riau pr�c�dent en avait une */
					glDisable(GL_TEXTURE_2D);
				}
			m_scene->getMaterial(m_materialIndex)->apply();
		}

	/* Affichage du VBO */
	m_parent->bindVBO();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferID);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glDrawElements(GL_TRIANGLES, m_indexArray.size(), GL_UNSIGNED_INT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindBuffer(GL_ARRAY_BUFFER, 0 );
	lastMaterialIndex = m_materialIndex;
}

const bool Mesh::isTransparent () const
{
	if (m_scene->getMaterial(m_materialIndex)->hasDiffuseTexture())
		if ( m_scene->getMaterial(m_materialIndex)->isTransparent())
			return true;
	return false;
}

void Mesh::setUVsAndNormals(const vector < CVector > &normalsVector, const vector < GLuint > &normalsIndexVector,
                            const vector < CPoint >  &texCoordsVector, const vector < GLuint > &texCoordsIndexVector)
{
	Vertex v;
	CVector normal, texCoord;
	uint dup=0,nondup=0;

	//   cerr << " Over " << m_indexArray.size() << " vertices, ";
	if (!m_attributes)
		for (uint i = 0; i < m_indexArray.size(); i++)
			m_parent->setVertex( m_indexArray[i], 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	else
		if (m_attributes < 3)
			for (uint i = 0; i < m_indexArray.size(); i++)
			{
				normal = normalsVector[normalsIndexVector[i]];
				if ( m_parent->findRefInLookupTable( m_indexArray[i] ) )
					/* Le point courant a d�j� �t� r�f�renc� auparavant dans le tableau d'indices */
				{
					v = m_parent->getVertex(m_indexArray[i]);
					if ( ((float)normal.x) == v.nx && ((float)normal.y) == v.ny && ((float)normal.z) == v.nz ){
						nondup++;
						/* La normale du point courant est identique � la pr�c�dente r�f�rence, il n'y a donc rien � faire */
						continue;
					}else {
						m_parent->addVertex( v );
						/* Le nouveau point est plac� en dernier, on r�cup�re son index et on le stocke */
						m_indexArray[i] = m_parent->getVertexArraySize()-1;
						dup++;
					}
				}
				else
					m_parent->addRefInLookupTable( m_indexArray[i], i);
				/* On affecte les coordonn�es de texture et de normale au point courant */
				m_parent->setVertex( m_indexArray[i], 0.0f, 0.0f, normal.x, normal.y, normal.z);
			}
		else
			for (uint i = 0; i < m_indexArray.size(); i++)
			{
				normal = normalsVector[normalsIndexVector[i]];
				texCoord = texCoordsVector[texCoordsIndexVector[i]];

				if ( m_parent->findRefInLookupTable( m_indexArray[i] ) )
					/* Le point courant a d�j� �t� r�f�renc� auparavant dans le tableau d'indices */
				{
					v = m_parent->getVertex(m_indexArray[i]);
					if ( ((float)texCoord.x) == v.u && ((float)texCoord.y) == v.v &&
					     ((float)normal.x) == v.nx && ((float)normal.y) == v.ny && ((float)normal.z) == v.nz ){
						nondup++;
						/* La normale et les coordonn�es de texture du point courant sont identiques � la pr�c�dente r�f�rence, il n'y a donc rien � faire */
						continue;
					}else{
						m_parent->addVertex( v );
						/* Le nouveau point est plac� en dernier, on r�cup�re son index et on le stocke */
						m_indexArray[i] = m_parent->getVertexArraySize()-1;
						dup++;
					}
				}
				else
					m_parent->addRefInLookupTable( m_indexArray[i], i);
				/* On affecte les coordonn�es de texture et de normale au point courant */
				m_parent->setVertex( m_indexArray[i], texCoord.x, texCoord.y, normal.x, normal.y, normal.z);
			}
	//   cerr << dup << " have been duplicated, " << nondup << " untouched" << endl;
}
