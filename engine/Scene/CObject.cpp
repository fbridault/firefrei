#include "CObject.hpp"

#include "CScene.hpp"
#include "CMaterial.hpp"
#include "../Utility/CRefTable.hpp"

#include <values.h>

extern uint g_objectCount;


/**************************************************************************************************/
/**														 DEFINITION DE LA CLASSE OBJECT 											 	    			*/
/**************************************************************************************************/

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
CObject::CObject(CScene* const scene) : CSceneItem(CPoint(0,0,0))
{
	assert ( scene != NULL);

	m_scene      = scene;
	m_attributes = 0;

	glGenBuffers(1, &m_bufferID);

	m_glName = CScene::glNameCounter++;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
CObject::~CObject ()
{
	m_vertexArray.clear();

	for (vector <CMesh* >::iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		delete (*meshesListIterator);
	m_meshesList.clear ();
	glDeleteBuffers(1, &m_bufferID);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
void CObject::buildBoundingBox ()
{
	CPoint ptMax(-FLT_MAX, -FLT_MAX, -FLT_MAX), ptMin(FLT_MAX, FLT_MAX, FLT_MAX);
	/* Création de la bounding box */

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

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
void CObject::scale (float scaleFactor, const CPoint& offset)
{
	for (vector < Vertex >::iterator vertexIterator = m_vertexArray.begin ();
	     vertexIterator != m_vertexArray.end (); vertexIterator++)
	{
		vertexIterator->x = (vertexIterator->x + offset.x) * scaleFactor;
		vertexIterator->y = (vertexIterator->y + offset.y) * scaleFactor;
		vertexIterator->z = (vertexIterator->z + offset.z) * scaleFactor;
	}
	m_max = (m_max + offset) * scaleFactor;
	m_min = (m_min + offset) * scaleFactor;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
void CObject::buildBoundingSpheres ()
{
	CRefTable refTable(getVertexArraySize());

	for (vector <CMesh* >::iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		(*meshesListIterator)->buildBoundingSphere(refTable);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
void CObject::drawBoundingSpheres ()
{
	m_scene->getMaterial(0)->apply();
	for (vector <CMesh* >::iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		(*meshesListIterator)->drawBoundingSphere();
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
void CObject::computeVisibility(const CCamera &view)
{
	for (vector <CMesh* >::iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		(*meshesListIterator)->computeVisibility(view);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
uint CObject::getPolygonsCount () const
{
	uint count=0;
	for (vector <CMesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		count += (*meshesListIterator)->getPolygonsCount();
	return count;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
void CObject::buildVBO()
{
	/* Détermination du type de données décrites à partir des maillages */
	for (vector <CMesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		if ( (*meshesListIterator)->getAttributes() > m_attributes)
			m_attributes = (*meshesListIterator)->getAttributes();

	glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
	glBufferData(GL_ARRAY_BUFFER, m_vertexArray.size()*sizeof(Vertex), &m_vertexArray[0], GL_STATIC_DRAW);

	for (vector <CMesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		(*meshesListIterator)->buildVBO();
	glBindBuffer(GL_ARRAY_BUFFER, 0 );
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
void CObject::draw (char drawCode, bool tex, bool boundingSpheres) const
{
	/* On initialise le dernier matériau au premier de la liste, le matériau par défaut */
	uint lastMaterialIndex=0;

	if (boundingSpheres){
		m_scene->getMaterial(0)->apply();
		for (vector <CMesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
		     meshesListIterator != m_meshesList.end ();
		     meshesListIterator++)
			(*meshesListIterator)->drawBoundingSphere();
	}else{
		if (drawCode == AMBIENT)
			/* Dessiner avec le matériau par défaut (pour tester les zones d'ombres par exemple) */
			m_scene->getMaterial(0)->apply();

		glPushMatrix();
		glTranslatef(m_position.x, m_position.y, m_position.z);
		/* Parcours de la liste des meshes */
		for (vector <CMesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
		     meshesListIterator != m_meshesList.end ();
		     meshesListIterator++)
			(*meshesListIterator)->draw(drawCode, tex, lastMaterialIndex);
		glPopMatrix();

		if (drawCode != AMBIENT)
			/* On désactive l'unité de texture le cas échéant */
			if (m_scene->getMaterial(lastMaterialIndex)->hasDiffuseTexture() && tex){
				glDisable(GL_TEXTURE_2D);
			}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
void CObject::drawForSelection () const
{
	m_scene->getMaterial(0)->apply();

	glPushMatrix();
	glTranslatef(m_position.x, m_position.y, m_position.z);
	glPushName(m_glName);
	/* Parcours de la liste des meshes */
	for (vector <CMesh* >::const_iterator meshesListIterator = m_meshesList.begin ();
	     meshesListIterator != m_meshesList.end ();
	     meshesListIterator++)
		(*meshesListIterator)->drawForSelection();
	glPopName();
	glPopMatrix();
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
void CObject::translate(const CVector& direction)
{
	for (vector < Vertex >::iterator vertexIterator = m_vertexArray.begin ();
	     vertexIterator != m_vertexArray.end (); vertexIterator++)
	{
		vertexIterator->x += direction.x;
		vertexIterator->y += direction.y;
		vertexIterator->z += direction.z;
	}
	/* On déplace aussi la boîte englobante */
	m_max += direction;
	m_min += direction;
}

