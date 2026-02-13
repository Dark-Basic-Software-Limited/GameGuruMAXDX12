/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  https://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
///btSoftBody implementation by Nathanael Presson

#include "btSoftBodyInternals.h"
#include "BulletSoftBody/btSoftBodySolvers.h"
#include "btSoftBodyData.h"
#include "LinearMath/btSerializer.h"
#include "LinearMath/btImplicitQRSVD.h"
#include "LinearMath/btAlignedAllocator.h"
#include "BulletDynamics/Featherstone/btMultiBodyLinkCollider.h"
#include "BulletDynamics/Featherstone/btMultiBodyConstraint.h"
#include "BulletCollision/NarrowPhaseCollision/btGjkEpa2.h"
#include "BulletCollision/CollisionShapes/btTriangleShape.h"
#include <iostream>
//
static inline btDbvtNode* buildTreeBottomUp(btAlignedObjectArray<btDbvtNode*>& leafNodes, btAlignedObjectArray<btAlignedObjectArray<int> >& adj)
{
	int N = leafNodes.size();
	if (N == 0)
	{
		return NULL;
	}
	while (N > 1)
	{
		btAlignedObjectArray<bool> marked;
		btAlignedObjectArray<btDbvtNode*> newLeafNodes;
		btAlignedObjectArray<std::pair<int, int> > childIds;
		btAlignedObjectArray<btAlignedObjectArray<int> > newAdj;
		marked.resize(N);
		for (int i = 0; i < N; ++i)
			marked[i] = false;

		// pair adjacent nodes into new(parent) node
		for (int i = 0; i < N; ++i)
		{
			if (marked[i])
				continue;
			bool merged = false;
			for (int j = 0; j < adj[i].size(); ++j)
			{
				int n = adj[i][j];
				if (!marked[adj[i][j]])
				{
					btDbvtNode* node = new (btAlignedAlloc(sizeof(btDbvtNode), 16)) btDbvtNode();
					node->parent = NULL;
					node->childs[0] = leafNodes[i];
					node->childs[1] = leafNodes[n];
					leafNodes[i]->parent = node;
					leafNodes[n]->parent = node;
					newLeafNodes.push_back(node);
					childIds.push_back(std::make_pair(i, n));
					merged = true;
					marked[n] = true;
					break;
				}
			}
			if (!merged)
			{
				newLeafNodes.push_back(leafNodes[i]);
				childIds.push_back(std::make_pair(i, -1));
			}
			marked[i] = true;
		}
		// update adjacency matrix
		newAdj.resize(newLeafNodes.size());
		for (int i = 0; i < newLeafNodes.size(); ++i)
		{
			for (int j = i + 1; j < newLeafNodes.size(); ++j)
			{
				bool neighbor = false;
				const btAlignedObjectArray<int>& leftChildNeighbors = adj[childIds[i].first];
				for (int k = 0; k < leftChildNeighbors.size(); ++k)
				{
					if (leftChildNeighbors[k] == childIds[j].first || leftChildNeighbors[k] == childIds[j].second)
					{
						neighbor = true;
						break;
					}
				}
				if (!neighbor && childIds[i].second != -1)
				{
					const btAlignedObjectArray<int>& rightChildNeighbors = adj[childIds[i].second];
					for (int k = 0; k < rightChildNeighbors.size(); ++k)
					{
						if (rightChildNeighbors[k] == childIds[j].first || rightChildNeighbors[k] == childIds[j].second)
						{
							neighbor = true;
							break;
						}
					}
				}
				if (neighbor)
				{
					newAdj[i].push_back(j);
					newAdj[j].push_back(i);
				}
			}
		}
		leafNodes = newLeafNodes;
		//this assignment leaks memory, the assignment doesn't do a deep copy, for now a manual copy
		//adj = newAdj;
		adj.clear();
		adj.resize(newAdj.size());
		for (int i = 0; i < newAdj.size(); i++)
		{
			for (int j = 0; j < newAdj[i].size(); j++)
			{
				adj[i].push_back(newAdj[i][j]);
			}
		}
		N = leafNodes.size();
	}
	return leafNodes[0];
}

//
btSoftBody::btSoftBody(btSoftBodyWorldInfo* worldInfo, int node_count, const btVector3* x, const btScalar* m)
	: m_softBodySolver(0), m_worldInfo(worldInfo)
{
	/* Init		*/
	initDefaults();

	/* Default material	*/
	Material* pm = appendMaterial();
	pm->m_kLST = 1;
	pm->m_kAST = 1;
	pm->m_kVST = 1;
	pm->m_flags = fMaterial::Default;

	/* Nodes			*/
	const btScalar margin = getCollisionShape()->getMargin();
	m_nodes.resize(node_count);
	m_X.resize(node_count);
	for (int i = 0, ni = node_count; i < ni; ++i)
	{
		Node& n = m_nodes[i];
		ZeroInitialize(n);
		n.m_x = x ? *x++ : btVector3(0, 0, 0);
		n.m_q = n.m_x;
		n.m_im = m ? *m++ : 1;
		n.m_im = n.m_im > 0 ? 1 / n.m_im : 0;
		n.m_leaf = m_ndbvt.insert(btDbvtVolume::FromCR(n.m_x, margin), &n);
		n.m_material = pm;
		m_X[i] = n.m_x;
	}
	updateBounds();
	setCollisionQuadrature(3);
	m_fdbvnt = 0;
}

btSoftBody::btSoftBody(btSoftBodyWorldInfo* worldInfo)
	: m_worldInfo(worldInfo)
{
	initDefaults();
}

void btSoftBody::initDefaults()
{
	m_internalType = CO_SOFT_BODY;
	m_cfg.aeromodel = eAeroModel::V_Point;
	m_cfg.kVCF = 1;
	m_cfg.kDG = 0;
	m_cfg.kLF = 0;
	m_cfg.kDP = 0;
	m_cfg.kPR = 0;
	m_cfg.kVC = 0;
	m_cfg.kDF = (btScalar)0.2;
	m_cfg.kMT = 0;
	m_cfg.kCHR = (btScalar)1.0;
	m_cfg.kKHR = (btScalar)0.1;
	m_cfg.kSHR = (btScalar)1.0;
	m_cfg.kAHR = (btScalar)0.7;
	m_cfg.kSRHR_CL = (btScalar)0.1;
	m_cfg.kSKHR_CL = (btScalar)1;
	m_cfg.kSSHR_CL = (btScalar)0.5;
	m_cfg.kSR_SPLT_CL = (btScalar)0.5;
	m_cfg.kSK_SPLT_CL = (btScalar)0.5;
	m_cfg.kSS_SPLT_CL = (btScalar)0.5;
	m_cfg.maxvolume = (btScalar)1;
	m_cfg.timescale = 1;
	m_cfg.viterations = 0;
	m_cfg.piterations = 1;
	m_cfg.diterations = 0;
	m_cfg.citerations = 4;
	m_cfg.drag = 0;
	m_cfg.m_maxStress = 0;
	m_cfg.collisions = fCollision::Default;
	m_pose.m_bvolume = false;
	m_pose.m_bframe = false;
	m_pose.m_volume = 0;
	m_pose.m_com = btVector3(0, 0, 0);
	m_pose.m_rot.setIdentity();
	m_pose.m_scl.setIdentity();
	m_tag = 0;
	m_timeacc = 0;
	m_bUpdateRtCst = true;
	m_bounds[0] = btVector3(0, 0, 0);
	m_bounds[1] = btVector3(0, 0, 0);
	m_worldTransform.setIdentity();
	setSolver(eSolverPresets::Positions);

	/* Collision shape	*/
	///for now, create a collision shape internally
	m_collisionShape = new btSoftBodyCollisionShape(this);
	m_collisionShape->setMargin(0.25f);

	m_worldTransform.setIdentity();

	m_windVelocity = btVector3(0, 0, 0);
	m_restLengthScale = btScalar(1.0);
	m_dampingCoefficient = 1.0;
	m_sleepingThreshold = .04;
	m_useSelfCollision = false;
	m_collisionFlags = 0;
	m_softSoftCollision = false;
	m_maxSpeedSquared = 0;
	m_repulsionStiffness = 0.5;
	m_gravityFactor = 1;
	m_cacheBarycenter = false;
	m_fdbvnt = 0;
}

//
btSoftBody::~btSoftBody()
{
	//for now, delete the internal shape
	delete m_collisionShape;
	int i;

	releaseClusters();
	for (i = 0; i < m_materials.size(); ++i)
		btAlignedFree(m_materials[i]);
	for (i = 0; i < m_joints.size(); ++i)
		btAlignedFree(m_joints[i]);
	if (m_fdbvnt)
		delete m_fdbvnt;
}

//
bool btSoftBody::checkLink(int node0, int node1) const
{
	return (checkLink(&m_nodes[node0], &m_nodes[node1]));
}

//
bool btSoftBody::checkLink(const Node* node0, const Node* node1) const
{
	const Node* n[] = {node0, node1};
	for (int i = 0, ni = m_links.size(); i < ni; ++i)
	{
		const Link& l = m_links[i];
		if ((l.m_n[0] == n[0] && l.m_n[1] == n[1]) ||
			(l.m_n[0] == n[1] && l.m_n[1] == n[0]))
		{
			return (true);
		}
	}
	return (false);
}

//
bool btSoftBody::checkFace(int node0, int node1, int node2) const
{
	const Node* n[] = {&m_nodes[node0],
					   &m_nodes[node1],
					   &m_nodes[node2]};
	for (int i = 0, ni = m_faces.size(); i < ni; ++i)
	{
		const Face& f = m_faces[i];
		int c = 0;
		for (int j = 0; j < 3; ++j)
		{
			if ((f.m_n[j] == n[0]) ||
				(f.m_n[j] == n[1]) ||
				(f.m_n[j] == n[2]))
				c |= 1 << j;
			else
				break;
		}
		if (c == 7) return (true);
	}
	return (false);
}

//
btSoftBody::Material* btSoftBody::appendMaterial()
{
	Material* pm = new (btAlignedAlloc(sizeof(Material), 16)) Material();
	if (m_materials.size() > 0)
		*pm = *m_materials[0];
	else
		ZeroInitialize(*pm);
	m_materials.push_back(pm);
	return (pm);
}

//
void btSoftBody::appendNote(const char* text,
							const btVector3& o,
							const btVector4& c,
							Node* n0,
							Node* n1,
							Node* n2,
							Node* n3)
{
	Note n;
	ZeroInitialize(n);
	n.m_rank = 0;
	n.m_text = text;
	n.m_offset = o;
	n.m_coords[0] = c.x();
	n.m_coords[1] = c.y();
	n.m_coords[2] = c.z();
	n.m_coords[3] = c.w();
	n.m_nodes[0] = n0;
	n.m_rank += n0 ? 1 : 0;
	n.m_nodes[1] = n1;
	n.m_rank += n1 ? 1 : 0;
	n.m_nodes[2] = n2;
	n.m_rank += n2 ? 1 : 0;
	n.m_nodes[3] = n3;
	n.m_rank += n3 ? 1 : 0;
	m_notes.push_back(n);
}

//
void btSoftBody::appendNote(const char* text,
							const btVector3& o,
							Node* feature)
{
	appendNote(text, o, btVector4(1, 0, 0, 0), feature);
}

//
void btSoftBody::appendNote(const char* text,
							const btVector3& o,
							Link* feature)
{
	static const btScalar w = 1 / (btScalar)2;
	appendNote(text, o, btVector4(w, w, 0, 0), feature->m_n[0],
			   feature->m_n[1]);
}

//
void btSoftBody::appendNote(const char* text,
							const btVector3& o,
							Face* feature)
{
	static const btScalar w = 1 / (btScalar)3;
	appendNote(text, o, btVector4(w, w, w, 0), feature->m_n[0],
			   feature->m_n[1],
			   feature->m_n[2]);
}

//
void btSoftBody::appendNode(const btVector3& x, btScalar m)
{
	if (m_nodes.capacity() == m_nodes.size())
	{
		pointersToIndices();
		m_nodes.reserve(m_nodes.size() * 2 + 1);
		indicesToPointers();
	}
	const btScalar margin = getCollisionShape()->getMargin();
	m_nodes.push_back(Node());
	Node& n = m_nodes[m_nodes.size() - 1];
	ZeroInitialize(n);
	n.m_x = x;
	n.m_q = n.m_x;
	n.m_im = m > 0 ? 1 / m : 0;
	n.m_material = m_materials[0];
	n.m_leaf = m_ndbvt.insert(btDbvtVolume::FromCR(n.m_x, margin), &n);
}

//
void btSoftBody::appendLink(int model, Material* mat)
{
	Link l;
	if (model >= 0)
		l = m_links[model];
	else
	{
		ZeroInitialize(l);
		l.m_material = mat ? mat : m_materials[0];
	}
	m_links.push_back(l);
}

//
void btSoftBody::appendLink(int node0,
							int node1,
							Material* mat,
							bool bcheckexist)
{
	appendLink(&m_nodes[node0], &m_nodes[node1], mat, bcheckexist);
}

//
void btSoftBody::appendLink(Node* node0,
							Node* node1,
							Material* mat,
							bool bcheckexist)
{
	if ((!bcheckexist) || (!checkLink(node0, node1)))
	{
		appendLink(-1, mat);
		Link& l = m_links[m_links.size() - 1];
		l.m_n[0] = node0;
		l.m_n[1] = node1;
		l.m_rl = (l.m_n[0]->m_x - l.m_n[1]->m_x).length();
		m_bUpdateRtCst = true;
	}
}

//
void btSoftBody::appendFace(int model, Material* mat)
{
	Face f;
	if (model >= 0)
	{
		f = m_faces[model];
	}
	else
	{
		ZeroInitialize(f);
		f.m_material = mat ? mat : m_materials[0];
	}
	m_faces.push_back(f);
}

//
void btSoftBody::appendFace(int node0, int node1, int node2, Material* mat)
{
	if (node0 == node1)
		return;
	if (node1 == node2)
		return;
	if (node2 == node0)
		return;

	appendFace(-1, mat);
	Face& f = m_faces[m_faces.size() - 1];
	btAssert(node0 != node1);
	btAssert(node1 != node2);
	btAssert(node2 != node0);
	f.m_n[0] = &m_nodes[node0];
	f.m_n[1] = &m_nodes[node1];
	f.m_n[2] = &m_nodes[node2];
	f.m_ra = AreaOf(f.m_n[0]->m_x,
					f.m_n[1]->m_x,
					f.m_n[2]->m_x);
	m_bUpdateRtCst = true;
}

//
void btSoftBody::appendTetra(int model, Material* mat)
{
	Tetra t;
	if (model >= 0)
		t = m_tetras[model];
	else
	{
		ZeroInitialize(t);
		t.m_material = mat ? mat : m_materials[0];
	}
	m_tetras.push_back(t);
}

//
void btSoftBody::appendTetra(int node0,
							 int node1,
							 int node2,
							 int node3,
							 Material* mat)
{
	appendTetra(-1, mat);
	Tetra& t = m_tetras[m_tetras.size() - 1];
	t.m_n[0] = &m_nodes[node0];
	t.m_n[1] = &m_nodes[node1];
	t.m_n[2] = &m_nodes[node2];
	t.m_n[3] = &m_nodes[node3];
	t.m_rv = VolumeOf(t.m_n[0]->m_x, t.m_n[1]->m_x, t.m_n[2]->m_x, t.m_n[3]->m_x);
	m_bUpdateRtCst = true;
}

//

void btSoftBody::appendAnchor(int node, btRigidBody* body, bool disableCollisionBetweenLinkedBodies, btScalar influence)
{
	btVector3 local = body->getWorldTransform().inverse() * m_nodes[node].m_x;
	appendAnchor(node, body, local, disableCollisionBetweenLinkedBodies, influence);
}

//
void btSoftBody::appendAnchor(int node, btRigidBody* body, const btVector3& localPivot, bool disableCollisionBetweenLinkedBodies, btScalar influence)
{
	if (disableCollisionBetweenLinkedBodies)
	{
		if (m_collisionDisabledObjects.findLinearSearch(body) == m_collisionDisabledObjects.size())
		{
			m_collisionDisabledObjects.push_back(body);
		}
	}

	Anchor a;
	a.m_node = &m_nodes[node];
	a.m_body = body;
	a.m_local = localPivot;
	a.m_node->m_battach = 1;
	a.m_influence = influence;
	m_anchors.push_back(a);
}

//
void btSoftBody::appendDeformableAnchor(int node, btRigidBody* body)
{
	DeformableNodeRigidAnchor c;
	btSoftBody::Node& n = m_nodes[node];
	const btScalar ima = n.m_im;
	const btScalar imb = body->getInvMass();
	btVector3 nrm;
	const btCollisionShape* shp = body->getCollisionShape();
	const btTransform& wtr = body->getWorldTransform();
	btScalar dst =
		m_worldInfo->m_sparsesdf.Evaluate(
			wtr.invXform(m_nodes[node].m_x),
			shp,
			nrm,
			0);

	c.m_cti.m_colObj = body;
	c.m_cti.m_normal = wtr.getBasis() * nrm;
	c.m_cti.m_offset = dst;
	c.m_node = &m_nodes[node];
	const btScalar fc = m_cfg.kDF * body->getFriction();
	c.m_c2 = ima;
	c.m_c3 = fc;
	c.m_c4 = body->isStaticOrKinematicObject() ? m_cfg.kKHR : m_cfg.kCHR;
	static const btMatrix3x3 iwiStatic(0, 0, 0, 0, 0, 0, 0, 0, 0);
	const btMatrix3x3& iwi = body->getInvInertiaTensorWorld();
	const btVector3 ra = n.m_x - wtr.getOrigin();

	c.m_c0 = ImpulseMatrix(1, ima, imb, iwi, ra);
	c.m_c1 = ra;
	c.m_local = body->getWorldTransform().inverse() * m_nodes[node].m_x;
	c.m_node->m_battach = 1;
	m_deformableAnchors.push_back(c);
}

void btSoftBody::removeAnchor(int node)
{
	const btSoftBody::Node& n = m_nodes[node];
	for (int i = 0; i < m_deformableAnchors.size();)
	{
		const DeformableNodeRigidAnchor& c = m_deformableAnchors[i];
		if (c.m_node == &n)
		{
			m_deformableAnchors.removeAtIndex(i);
		}
		else
		{
			i++;
		}
	}
}

//
void btSoftBody::appendDeformableAnchor(int node, btMultiBodyLinkCollider* link)
{
	DeformableNodeRigidAnchor c;
	btSoftBody::Node& n = m_nodes[node];
	const btScalar ima = n.m_im;
	btVector3 nrm;
	const btCollisionShape* shp = link->getCollisionShape();
	const btTransform& wtr = link->getWorldTransform();
	btScalar dst =
		m_worldInfo->m_sparsesdf.Evaluate(
			wtr.invXform(m_nodes[node].m_x),
			shp,
			nrm,
			0);
	c.m_cti.m_colObj = link;
	c.m_cti.m_normal = wtr.getBasis() * nrm;
	c.m_cti.m_offset = dst;
	c.m_node = &m_nodes[node];
	const btScalar fc = m_cfg.kDF * link->getFriction();
	c.m_c2 = ima;
	c.m_c3 = fc;
	c.m_c4 = link->isStaticOrKinematicObject() ? m_cfg.kKHR : m_cfg.kCHR;
	btVector3 normal = c.m_cti.m_normal;
	btVector3 t1 = generateUnitOrthogonalVector(normal);
	btVector3 t2 = btCross(normal, t1);
	btMultiBodyJacobianData jacobianData_normal, jacobianData_t1, jacobianData_t2;
	findJacobian(link, jacobianData_normal, c.m_node->m_x, normal);
	findJacobian(link, jacobianData_t1, c.m_node->m_x, t1);
	findJacobian(link, jacobianData_t2, c.m_node->m_x, t2);

	btScalar* J_n = &jacobianData_normal.m_jacobians[0];
	btScalar* J_t1 = &jacobianData_t1.m_jacobians[0];
	btScalar* J_t2 = &jacobianData_t2.m_jacobians[0];

	btScalar* u_n = &jacobianData_normal.m_deltaVelocitiesUnitImpulse[0];
	btScalar* u_t1 = &jacobianData_t1.m_deltaVelocitiesUnitImpulse[0];
	btScalar* u_t2 = &jacobianData_t2.m_deltaVelocitiesUnitImpulse[0];

	btMatrix3x3 rot(normal.getX(), normal.getY(), normal.getZ(),
					t1.getX(), t1.getY(), t1.getZ(),
					t2.getX(), t2.getY(), t2.getZ());  // world frame to local frame
	const int ndof = link->m_multiBody->getNumDofs() + 6;
	btMatrix3x3 local_impulse_matrix = (Diagonal(n.m_im) + OuterProduct(J_n, J_t1, J_t2, u_n, u_t1, u_t2, ndof)).inverse();
	c.m_c0 = rot.transpose() * local_impulse_matrix * rot;
	c.jacobianData_normal = jacobianData_normal;
	c.jacobianData_t1 = jacobianData_t1;
	c.jacobianData_t2 = jacobianData_t2;
	c.t1 = t1;
	c.t2 = t2;
	const btVector3 ra = n.m_x - wtr.getOrigin();
	c.m_c1 = ra;
	c.m_local = link->getWorldTransform().inverse() * m_nodes[node].m_x;
	c.m_node->m_battach = 1;
	m_deformableAnchors.push_back(c);
}
//
void btSoftBody::appendLinearJoint(const LJoint::Specs& specs, Cluster* body0, Body body1)
{
	LJoint* pj = new (btAlignedAlloc(sizeof(LJoint), 16)) LJoint();
	pj->m_bodies[0] = body0;
	pj->m_bodies[1] = body1;
	pj->m_refs[0] = pj->m_bodies[0].xform().inverse() * specs.position;
	pj->m_refs[1] = pj->m_bodies[1].xform().inverse() * specs.position;
	pj->m_cfm = specs.cfm;
	pj->m_erp = specs.erp;
	pj->m_split = specs.split;
	m_joints.push_back(pj);
}

//
void btSoftBody::appendLinearJoint(const LJoint::Specs& specs, Body body)
{
	appendLinearJoint(specs, m_clusters[0], body);
}

//
void btSoftBody::appendLinearJoint(const LJoint::Specs& specs, btSoftBody* body)
{
	appendLinearJoint(specs, m_clusters[0], body->m_clusters[0]);
}

//
void btSoftBody::appendAngularJoint(const AJoint::Specs& specs, Cluster* body0, Body body1)
{
	AJoint* pj = new (btAlignedAlloc(sizeof(AJoint), 16)) AJoint();
	pj->m_bodies[0] = body0;
	pj->m_bodies[1] = body1;
	pj->m_refs[0] = pj->m_bodies[0].xform().inverse().getBasis() * specs.axis;
	pj->m_refs[1] = pj->m_bodies[1].xform().inverse().getBasis() * specs.axis;
	pj->m_cfm = specs.cfm;
	pj->m_erp = specs.erp;
	pj->m_split = specs.split;
	pj->m_icontrol = specs.icontrol;
	m_joints.push_back(pj);
}

//
void btSoftBody::appendAngularJoint(const AJoint::Specs& specs, Body body)
{
	appendAngularJoint(specs, m_clusters[0], body);
}

//
void btSoftBody::appendAngularJoint(const AJoint::Specs& specs, btSoftBody* body)
{
	appendAngularJoint(specs, m_clusters[0], body->m_clusters[0]);
}

//
void btSoftBody::addForce(const btVector3& force)
{
	for (int i = 0, ni = m_nodes.size(); i < ni; ++i) addForce(force, i);
}

//
void btSoftBody::addForce(const btVector3& force, int node)
{
	Node& n = m_nodes[node];
	if (n.m_im > 0)
	{
		n.m_f += force;
	}
}

void btSoftBody::addAeroForceToNode(const btVector3& windVelocity, int nodeIndex)
{
	btAssert(nodeIndex >= 0 && nodeIndex < m_nodes.size());

	const btScalar dt = m_sst.sdt;
	const btScalar kLF = m_cfg.kLF;
	const btScalar kDG = m_cfg.kDG;
	//const btScalar kPR = m_cfg.kPR;
	//const btScalar kVC = m_cfg.kVC;
	const bool as_lift = kLF > 0;
	const bool as_drag = kDG > 0;
	const bool as_aero = as_lift || as_drag;
	const bool as_vaero = as_aero && (m_cfg.aeromodel < btSoftBody::eAeroModel::F_TwoSided);

	Node& n = m_nodes[nodeIndex];

	if (n.m_im > 0)
	{
		btSoftBody::sMedium medium;

		EvaluateMedium(m_worldInfo, n.m_x, medium);
		medium.m_velocity = windVelocity;
		medium.m_density = m_worldInfo->air_density;

		/* Aerodynamics			*/
		if (as_vaero)
		{
			const btVector3 rel_v = n.m_v - medium.m_velocity;
			const btScalar rel_v_len = rel_v.length();
			const btScalar rel_v2 = rel_v.length2();

			if (rel_v2 > SIMD_EPSILON)
			{
				const btVector3 rel_v_nrm = rel_v.normalized();
				btVector3 nrm = n.m_n;

				if (m_cfg.aeromodel == btSoftBody::eAeroModel::V_TwoSidedLiftDrag)
				{
					nrm *= (btScalar)((btDot(nrm, rel_v) < 0) ? -1 : +1);
					btVector3 fDrag(0, 0, 0);
					btVector3 fLift(0, 0, 0);

					btScalar n_dot_v = nrm.dot(rel_v_nrm);
					btScalar tri_area = 0.5f * n.m_area;

					fDrag = 0.5f * kDG * medium.m_density * rel_v2 * tri_area * n_dot_v * (-rel_v_nrm);

					// Check angle of attack
					// cos(10Âº) = 0.98480
					if (0 < n_dot_v && n_dot_v < 0.98480f)
						fLift = 0.5f * kLF * medium.m_density * rel_v_len * tri_area * btSqrt(1.0f - n_dot_v * n_dot_v) * (nrm.cross(rel_v_nrm).cross(rel_v_nrm));

					// Check if the velocity change resulted by aero drag force exceeds the current velocity of the node.
					btVector3 del_v_by_fDrag = fDrag * n.m_im * m_sst.sdt;
					btScalar del_v_by_fDrag_len2 = del_v_by_fDrag.length2();
					btScalar v_len2 = n.m_v.length2();

					if (del_v_by_fDrag_len2 >= v_len2 && del_v_by_fDrag_len2 > 0)
					{
						btScalar del_v_by_fDrag_len = del_v_by_fDrag.length();
						btScalar v_len = n.m_v.length();
						fDrag *= btScalar(0.8) * (v_len / del_v_by_fDrag_len);
					}

					n.m_f += fDrag;
					n.m_f += fLift;
				}
				else if (m_cfg.aeromodel == btSoftBody::eAeroModel::V_Point || m_cfg.aeromodel == btSoftBody::eAeroModel::V_OneSided || m_cfg.aeromodel == btSoftBody::eAeroModel::V_TwoSided)
				{
					if (m_cfg.aeromodel == btSoftBody::eAeroModel::V_TwoSided)
						nrm *= (btScalar)((btDot(nrm, rel_v) < 0) ? -1 : +1);

					const btScalar dvn = btDot(rel_v, nrm);
					/* Compute forces	*/
					if (dvn > 0)
					{
						btVector3 force(0, 0, 0);
						const btScalar c0 = n.m_area * dvn * rel_v2 / 2;
						const btScalar c1 = c0 * medium.m_density;
						force += nrm * (-c1 * kLF);
						force += rel_v.normalized() * (-c1 * kDG);
						ApplyClampedForce(n, force, dt);
					}
				}
			}
		}
	}
}

void btSoftBody::addAeroForceToFace(const btVector3& windVelocity, int faceIndex)
{
	const btScalar dt = m_sst.sdt;
	const btScalar kLF = m_cfg.kLF;
	const btScalar kDG = m_cfg.kDG;
	//	const btScalar kPR = m_cfg.kPR;
	//	const btScalar kVC = m_cfg.kVC;
	const bool as_lift = kLF > 0;
	const bool as_drag = kDG > 0;
	const bool as_aero = as_lift || as_drag;
	const bool as_faero = as_aero && (m_cfg.aeromodel >= btSoftBody::eAeroModel::F_TwoSided);

	if (as_faero)
	{
		btSoftBody::Face& f = m_faces[faceIndex];

		btSoftBody::sMedium medium;

		const btVector3 v = (f.m_n[0]->m_v + f.m_n[1]->m_v + f.m_n[2]->m_v) / 3;
		const btVector3 x = (f.m_n[0]->m_x + f.m_n[1]->m_x + f.m_n[2]->m_x) / 3;
		EvaluateMedium(m_worldInfo, x, medium);
		medium.m_velocity = windVelocity;
		medium.m_density = m_worldInfo->air_density;
		const btVector3 rel_v = v - medium.m_velocity;
		const btScalar rel_v_len = rel_v.length();
		const btScalar rel_v2 = rel_v.length2();

		if (rel_v2 > SIMD_EPSILON)
		{
			const btVector3 rel_v_nrm = rel_v.normalized();
			btVector3 nrm = f.m_normal;

			if (m_cfg.aeromodel == btSoftBody::eAeroModel::F_TwoSidedLiftDrag)
			{
				nrm *= (btScalar)((btDot(nrm, rel_v) < 0) ? -1 : +1);

				btVector3 fDrag(0, 0, 0);
				btVector3 fLift(0, 0, 0);

				btScalar n_dot_v = nrm.dot(rel_v_nrm);
				btScalar tri_area = 0.5f * f.m_ra;

				fDrag = 0.5f * kDG * medium.m_density * rel_v2 * tri_area * n_dot_v * (-rel_v_nrm);

				// Check angle of attack
				// cos(10Âº) = 0.98480
				if (0 < n_dot_v && n_dot_v < 0.98480f)
					fLift = 0.5f * kLF * medium.m_density * rel_v_len * tri_area * btSqrt(1.0f - n_dot_v * n_dot_v) * (nrm.cross(rel_v_nrm).cross(rel_v_nrm));

				fDrag /= 3;
				fLift /= 3;

				for (int j = 0; j < 3; ++j)
				{
					if (f.m_n[j]->m_im > 0)
					{
						// Check if the velocity change resulted by aero drag force exceeds the current velocity of the node.
						btVector3 del_v_by_fDrag = fDrag * f.m_n[j]->m_im * m_sst.sdt;
						btScalar del_v_by_fDrag_len2 = del_v_by_fDrag.length2();
						btScalar v_len2 = f.m_n[j]->m_v.length2();

						if (del_v_by_fDrag_len2 >= v_len2 && del_v_by_fDrag_len2 > 0)
						{
							btScalar del_v_by_fDrag_len = del_v_by_fDrag.length();
							btScalar v_len = f.m_n[j]->m_v.length();
							fDrag *= btScalar(0.8) * (v_len / del_v_by_fDrag_len);
						}

						f.m_n[j]->m_f += fDrag;
						f.m_n[j]->m_f += fLift;
					}
				}
			}
			else if (m_cfg.aeromodel == btSoftBody::eAeroModel::F_OneSided || m_cfg.aeromodel == btSoftBody::eAeroModel::F_TwoSided)
			{
				if (m_cfg.aeromodel == btSoftBody::eAeroModel::F_TwoSided)
					nrm *= (btScalar)((btDot(nrm, rel_v) < 0) ? -1 : +1);

				const btScalar dvn = btDot(rel_v, nrm);
				/* Compute forces	*/
				if (dvn > 0)
				{
					btVector3 force(0, 0, 0);
					const btScalar c0 = f.m_ra * dvn * rel_v2;
					const btScalar c1 = c0 * medium.m_density;
					force += nrm * (-c1 * kLF);
					force += rel_v.normalized() * (-c1 * kDG);
					force /= 3;
					for (int j = 0; j < 3; ++j) ApplyClampedForce(*f.m_n[j], force, dt);
				}
			}
		}
	}
}

//
void btSoftBody::addVelocity(const btVector3& velocity)
{
	for (int i = 0, ni = m_nodes.size(); i < ni; ++i) addVelocity(velocity, i);
}

/* Set velocity for the entire body										*/
void btSoftBody::setVelocity(const btVector3& velocity)
{
	for (int i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		Node& n = m_nodes[i];
		if (n.m_im > 0)
		{
			n.m_v = velocity;
			n.m_vn = velocity;
		}
	}
}

//
void btSoftBody::addVelocity(const btVector3& velocity, int node)
{
	Node& n = m_nodes[node];
	if (n.m_im > 0)
	{
		n.m_v += velocity;
	}
}

//
void btSoftBody::setMass(int node, btScalar mass)
{
	m_nodes[node].m_im = mass > 0 ? 1 / mass : 0;
	m_bUpdateRtCst = true;
}

//
btScalar btSoftBody::getMass(int node) const
{
	return (m_nodes[node].m_im > 0 ? 1 / m_nodes[node].m_im : 0);
}

//
btScalar btSoftBody::getTotalMass() const
{
	btScalar mass = 0;
	for (int i = 0; i < m_nodes.size(); ++i)
	{
		mass += getMass(i);
	}
	return (mass);
}

//
void btSoftBody::setTotalMass(btScalar mass, bool fromfaces)
{
	int i;

	if (fromfaces)
	{
		for (i = 0; i < m_nodes.size(); ++i)
		{
			m_nodes[i].m_im = 0;
		}
		for (i = 0; i < m_faces.size(); ++i)
		{
			const Face& f = m_faces[i];
			const btScalar twicearea = AreaOf(f.m_n[0]->m_x,
											  f.m_n[1]->m_x,
											  f.m_n[2]->m_x);
			for (int j = 0; j < 3; ++j)
			{
				f.m_n[j]->m_im += twicearea;
			}
		}
		for (i = 0; i < m_nodes.size(); ++i)
		{
			m_nodes[i].m_im = 1 / m_nodes[i].m_im;
		}
	}
	const btScalar tm = getTotalMass();
	const btScalar itm = 1 / tm;
	for (i = 0; i < m_nodes.size(); ++i)
	{
		m_nodes[i].m_im /= itm * mass;
	}
	m_bUpdateRtCst = true;
}

//
void btSoftBody::setTotalDensity(btScalar density)
{
	setTotalMass(getVolume() * density, true);
}

//
void btSoftBody::setVolumeMass(btScalar mass)
{
	btAlignedObjectArray<btScalar> ranks;
	ranks.resize(m_nodes.size(), 0);
	int i;

	for (i = 0; i < m_nodes.size(); ++i)
	{
		m_nodes[i].m_im = 0;
	}
	for (i = 0; i < m_tetras.size(); ++i)
	{
		const Tetra& t = m_tetras[i];
		for (int j = 0; j < 4; ++j)
		{
			t.m_n[j]->m_im += btFabs(t.m_rv);
			ranks[int(t.m_n[j] - &m_nodes[0])] += 1;
		}
	}
	for (i = 0; i < m_nodes.size(); ++i)
	{
		if (m_nodes[i].m_im > 0)
		{
			m_nodes[i].m_im = ranks[i] / m_nodes[i].m_im;
		}
	}
	setTotalMass(mass, false);
}

//
void btSoftBody::setVolumeDensity(btScalar density)
{
	btScalar volume = 0;
	for (int i = 0; i < m_tetras.size(); ++i)
	{
		const Tetra& t = m_tetras[i];
		for (int j = 0; j < 4; ++j)
		{
			volume += btFabs(t.m_rv);
		}
	}
	setVolumeMass(volume * density / 6);
}

//
btVector3 btSoftBody::getLinearVelocity()
{
	btVector3 total_momentum = btVector3(0, 0, 0);
	for (int i = 0; i < m_nodes.size(); ++i)
	{
		btScalar mass = m_nodes[i].m_im == 0 ? 0 : 1.0 / m_nodes[i].m_im;
		total_momentum += mass * m_nodes[i].m_v;
	}
	btScalar total_mass = getTotalMass();
	return total_mass == 0 ? total_momentum : total_momentum / total_mass;
}

//
void btSoftBody::setLinearVelocity(const btVector3& linVel)
{
	btVector3 old_vel = getLinearVelocity();
	btVector3 diff = linVel - old_vel;
	for (int i = 0; i < m_nodes.size(); ++i)
		m_nodes[i].m_v += diff;
}

//
void btSoftBody::setAngularVelocity(const btVector3& angVel)
{
	btVector3 old_vel = getLinearVelocity();
	btVector3 com = getCenterOfMass();
	for (int i = 0; i < m_nodes.size(); ++i)
	{
		m_nodes[i].m_v = angVel.cross(m_nodes[i].m_x - com) + old_vel;
	}
}

//
btTransform btSoftBody::getRigidTransform()
{
	btVector3 t = getCenterOfMass();
	btMatrix3x3 S;
	S.setZero();
	// Get rotation that minimizes L2 difference: \sum_i || RX_i + t - x_i ||
	// It's important to make sure that S has the correct signs.
	// SVD is only unique up to the ordering of singular values.
	// SVD will manipulate U and V to ensure the ordering of singular values. If all three singular
	// vaues are negative, SVD will permute colums of U to make two of them positive.
	for (int i = 0; i < m_nodes.size(); ++i)
	{
		S -= OuterProduct(m_X[i], t - m_nodes[i].m_x);
	}
	btVector3 sigma;
	btMatrix3x3 U, V;
	singularValueDecomposition(S, U, sigma, V);
	btMatrix3x3 R = V * U.transpose();
	btTransform trs;
	trs.setIdentity();
	trs.setOrigin(t);
	trs.setBasis(R);
	return trs;
}

//
void btSoftBody::transformTo(const btTransform& trs)
{
	// get the current best rigid fit
	btTransform current_transform = getRigidTransform();
	// apply transform in material space
	btTransform new_transform = trs * current_transform.inverse();
	transform(new_transform);
}

//
void btSoftBody::transform(const btTransform& trs)
{
	const btScalar margin = getCollisionShape()->getMargin();
	ATTRIBUTE_ALIGNED16(btDbvtVolume)
	vol;

	for (int i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		Node& n = m_nodes[i];
		n.m_x = trs * n.m_x;
		n.m_q = trs * n.m_q;
		n.m_n = trs.getBasis() * n.m_n;
		vol = btDbvtVolume::FromCR(n.m_x, margin);

		m_ndbvt.update(n.m_leaf, vol);
	}
	updateNormals();
	updateBounds();
	updateConstants();
}

//
void btSoftBody::translate(const btVector3& trs)
{
	btTransform t;
	t.setIdentity();
	t.setOrigin(trs);
	transform(t);
}

//
void btSoftBody::rotate(const btQuaternion& rot)
{
	btTransform t;
	t.setIdentity();
	t.setRotation(rot);
	transform(t);
}

//
void btSoftBody::scale(const btVector3& scl)
{
	const btScalar margin = getCollisionShape()->getMargin();
	ATTRIBUTE_ALIGNED16(btDbvtVolume)
	vol;

	for (int i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		Node& n = m_nodes[i];
		n.m_x *= scl;
		n.m_q *= scl;
		vol = btDbvtVolume::FromCR(n.m_x, margin);
		m_ndbvt.update(n.m_leaf, vol);
	}
	updateNormals();
	updateBounds();
	updateConstants();
	initializeDmInverse();
}

//
btScalar btSoftBody::getRestLengthScale()
{
	return m_restLengthScale;
}

//
void btSoftBody::setRestLengthScale(btScalar restLengthScale)
{
	for (int i = 0, ni = m_links.size(); i < ni; ++i)
	{
		Link& l = m_links[i];
		l.m_rl = l.m_rl / m_restLengthScale * restLengthScale;
		l.m_c1 = l.m_rl * l.m_rl;
	}
	m_restLengthScale = restLengthScale;

	if (getActivationState() == ISLAND_SLEEPING)
		activate();
}

//
void btSoftBody::setPose(bool bvolume, bool bframe)
{
	m_pose.m_bvolume = bvolume;
	m_pose.m_bframe = bframe;
	int i, ni;

	/* Weights		*/
	const btScalar omass = getTotalMass();
	const btScalar kmass = omass * m_nodes.size() * 1000;
	btScalar tmass = omass;
	m_pose.m_wgh.resize(m_nodes.size());
	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		if (m_nodes[i].m_im <= 0) tmass += kmass;
	}
	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		Node& n = m_nodes[i];
		m_pose.m_wgh[i] = n.m_im > 0 ? 1 / (m_nodes[i].m_im * tmass) : kmass / tmass;
	}
	/* Pos		*/
	const btVector3 com = evaluateCom();
	m_pose.m_pos.resize(m_nodes.size());
	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		m_pose.m_pos[i] = m_nodes[i].m_x - com;
	}
	m_pose.m_volume = bvolume ? getVolume() : 0;
	m_pose.m_com = com;
	m_pose.m_rot.setIdentity();
	m_pose.m_scl.setIdentity();
	/* Aqq		*/
	m_pose.m_aqq[0] =
		m_pose.m_aqq[1] =
			m_pose.m_aqq[2] = btVector3(0, 0, 0);
	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		const btVector3& q = m_pose.m_pos[i];
		const btVector3 mq = m_pose.m_wgh[i] * q;
		m_pose.m_aqq[0] += mq.x() * q;
		m_pose.m_aqq[1] += mq.y() * q;
		m_pose.m_aqq[2] += mq.z() * q;
	}
	m_pose.m_aqq = m_pose.m_aqq.inverse();

	updateConstants();
}

void btSoftBody::resetLinkRestLengths()
{
	for (int i = 0, ni = m_links.size(); i < ni; ++i)
	{
		Link& l = m_links[i];
		l.m_rl = (l.m_n[0]->m_x - l.m_n[1]->m_x).length();
		l.m_c1 = l.m_rl * l.m_rl;
	}
}

//
btScalar btSoftBody::getVolume() const
{
	btScalar vol = 0;
	if (m_nodes.size() > 0)
	{
		int i, ni;

		const btVector3 org = m_nodes[0].m_x;
		for (i = 0, ni = m_faces.size(); i < ni; ++i)
		{
			const Face& f = m_faces[i];
			vol += btDot(f.m_n[0]->m_x - org, btCross(f.m_n[1]->m_x - org, f.m_n[2]->m_x - org));
		}
		vol /= (btScalar)6;
	}
	return (vol);
}

//
int btSoftBody::clusterCount() const
{
	return (m_clusters.size());
}

//
btVector3 btSoftBody::clusterCom(const Cluster* cluster)
{
	btVector3 com(0, 0, 0);
	for (int i = 0, ni = cluster->m_nodes.size(); i < ni; ++i)
	{
		com += cluster->m_nodes[i]->m_x * cluster->m_masses[i];
	}
	return (com * cluster->m_imass);
}

//
btVector3 btSoftBody::clusterCom(int cluster) const
{
	return (clusterCom(m_clusters[cluster]));
}

//
btVector3 btSoftBody::clusterVelocity(const Cluster* cluster, const btVector3& rpos)
{
	return (cluster->m_lv + btCross(cluster->m_av, rpos));
}

//
void btSoftBody::clusterVImpulse(Cluster* cluster, const btVector3& rpos, const btVector3& impulse)
{
	const btVector3 li = cluster->m_imass * impulse;
	const btVector3 ai = cluster->m_invwi * btCross(rpos, impulse);
	cluster->m_vimpulses[0] += li;
	cluster->m_lv += li;
	cluster->m_vimpulses[1] += ai;
	cluster->m_av += ai;
	cluster->m_nvimpulses++;
}

//
void btSoftBody::clusterDImpulse(Cluster* cluster, const btVector3& rpos, const btVector3& impulse)
{
	const btVector3 li = cluster->m_imass * impulse;
	const btVector3 ai = cluster->m_invwi * btCross(rpos, impulse);
	cluster->m_dimpulses[0] += li;
	cluster->m_dimpulses[1] += ai;
	cluster->m_ndimpulses++;
}

//
void btSoftBody::clusterImpulse(Cluster* cluster, const btVector3& rpos, const Impulse& impulse)
{
	if (impulse.m_asVelocity) clusterVImpulse(cluster, rpos, impulse.m_velocity);
	if (impulse.m_asDrift) clusterDImpulse(cluster, rpos, impulse.m_drift);
}

//
void btSoftBody::clusterVAImpulse(Cluster* cluster, const btVector3& impulse)
{
	const btVector3 ai = cluster->m_invwi * impulse;
	cluster->m_vimpulses[1] += ai;
	cluster->m_av += ai;
	cluster->m_nvimpulses++;
}

//
void btSoftBody::clusterDAImpulse(Cluster* cluster, const btVector3& impulse)
{
	const btVector3 ai = cluster->m_invwi * impulse;
	cluster->m_dimpulses[1] += ai;
	cluster->m_ndimpulses++;
}

//
void btSoftBody::clusterAImpulse(Cluster* cluster, const Impulse& impulse)
{
	if (impulse.m_asVelocity) clusterVAImpulse(cluster, impulse.m_velocity);
	if (impulse.m_asDrift) clusterDAImpulse(cluster, impulse.m_drift);
}

//
void btSoftBody::clusterDCImpulse(Cluster* cluster, const btVector3& impulse)
{
	cluster->m_dimpulses[0] += impulse * cluster->m_imass;
	cluster->m_ndimpulses++;
}

struct NodeLinks
{
	btAlignedObjectArray<int> m_links;
};

//
int btSoftBody::generateBendingConstraints(int distance, Material* mat)
{
	int i, j;

	if (distance > 1)
	{
		/* Build graph	*/
		const int n = m_nodes.size();
		const unsigned inf = (~(unsigned)0) >> 1;
		unsigned* adj = new unsigned[n * n];

#define IDX(_x_, _y_) ((_y_)*n + (_x_))
		for (j = 0; j < n; ++j)
		{
			for (i = 0; i < n; ++i)
			{
				if (i != j)
				{
					adj[IDX(i, j)] = adj[IDX(j, i)] = inf;
				}
				else
				{
					adj[IDX(i, j)] = adj[IDX(j, i)] = 0;
				}
			}
		}
		for (i = 0; i < m_links.size(); ++i)
		{
			const int ia = (int)(m_links[i].m_n[0] - &m_nodes[0]);
			const int ib = (int)(m_links[i].m_n[1] - &m_nodes[0]);
			adj[IDX(ia, ib)] = 1;
			adj[IDX(ib, ia)] = 1;
		}

		//special optimized case for distance == 2
		if (distance == 2)
		{
			btAlignedObjectArray<NodeLinks> nodeLinks;

			/* Build node links */
			nodeLinks.resize(m_nodes.size());

			for (i = 0; i < m_links.size(); ++i)
			{
				const int ia = (int)(m_links[i].m_n[0] - &m_nodes[0]);
				const int ib = (int)(m_links[i].m_n[1] - &m_nodes[0]);
				if (nodeLinks[ia].m_links.findLinearSearch(ib) == nodeLinks[ia].m_links.size())
					nodeLinks[ia].m_links.push_back(ib);

				if (nodeLinks[ib].m_links.findLinearSearch(ia) == nodeLinks[ib].m_links.size())
					nodeLinks[ib].m_links.push_back(ia);
			}
			for (int ii = 0; ii < nodeLinks.size(); ii++)
			{
				int i = ii;

				for (int jj = 0; jj < nodeLinks[ii].m_links.size(); jj++)
				{
					int k = nodeLinks[ii].m_links[jj];
					for (int kk = 0; kk < nodeLinks[k].m_links.size(); kk++)
					{
						int j = nodeLinks[k].m_links[kk];
						if (i != j)
						{
							const unsigned sum = adj[IDX(i, k)] + adj[IDX(k, j)];
							btAssert(sum == 2);
							if (adj[IDX(i, j)] > sum)
							{
								adj[IDX(i, j)] = adj[IDX(j, i)] = sum;
							}
						}
					}
				}
			}
		}
		else
		{
			///generic Floyd's algorithm
			for (int k = 0; k < n; ++k)
			{
				for (j = 0; j < n; ++j)
				{
					for (i = j + 1; i < n; ++i)
					{
						const unsigned sum = adj[IDX(i, k)] + adj[IDX(k, j)];
						if (adj[IDX(i, j)] > sum)
						{
							adj[IDX(i, j)] = adj[IDX(j, i)] = sum;
						}
					}
				}
			}
		}

		/* Build links	*/
		int nlinks = 0;
		for (j = 0; j < n; ++j)
		{
			for (i = j + 1; i < n; ++i)
			{
				if (adj[IDX(i, j)] == (unsigned)distance)
				{
					appendLink(i, j, mat);
					m_links[m_links.size() - 1].m_bbending = 1;
					++nlinks;
				}
			}
		}
		delete[] adj;
		return (nlinks);
	}
	return (0);
}

//
void btSoftBody::randomizeConstraints()
{
	unsigned long seed = 243703;
#define NEXTRAND (seed = (1664525L * seed + 1013904223L) & 0xffffffff)
	int i, ni;

	for (i = 0, ni = m_links.size(); i < ni; ++i)
	{
		btSwap(m_links[i], m_links[NEXTRAND % ni]);
	}
	for (i = 0, ni = m_faces.size(); i < ni; ++i)
	{
		btSwap(m_faces[i], m_faces[NEXTRAND % ni]);
	}
#undef NEXTRAND
}

//
void btSoftBody::releaseCluster(int index)
{
	Cluster* c = m_clusters[index];
	if (c->m_leaf) m_cdbvt.remove(c->m_leaf);
	c->~Cluster();
	btAlignedFree(c);
	m_clusters.remove(c);
}

//
void btSoftBody::releaseClusters()
{
	while (m_clusters.size() > 0) releaseCluster(0);
}

//
int btSoftBody::generateClusters(int k, int maxiterations)
{
	int i;
	releaseClusters();
	m_clusters.resize(btMin(k, m_nodes.size()));
	for (i = 0; i < m_clusters.size(); ++i)
	{
		m_clusters[i] = new (btAlignedAlloc(sizeof(Cluster), 16)) Cluster();
		m_clusters[i]->m_collide = true;
	}
	k = m_clusters.size();
	if (k > 0)
	{
		/* Initialize		*/
		btAlignedObjectArray<btVector3> centers;
		btVector3 cog(0, 0, 0);
		int i;
		for (i = 0; i < m_nodes.size(); ++i)
		{
			cog += m_nodes[i].m_x;
			m_clusters[(i * 29873) % m_clusters.size()]->m_nodes.push_back(&m_nodes[i]);
		}
		cog /= (btScalar)m_nodes.size();
		centers.resize(k, cog);
		/* Iterate			*/
		const btScalar slope = 16;
		bool changed;
		int iterations = 0;
		do
		{
			const btScalar w = 2 - btMin<btScalar>(1, iterations / slope);
			changed = false;
			iterations++;
			int i;

			for (i = 0; i < k; ++i)
			{
				btVector3 c(0, 0, 0);
				for (int j = 0; j < m_clusters[i]->m_nodes.size(); ++j)
				{
					c += m_clusters[i]->m_nodes[j]->m_x;
				}
				if (m_clusters[i]->m_nodes.size())
				{
					c /= (btScalar)m_clusters[i]->m_nodes.size();
					c = centers[i] + (c - centers[i]) * w;
					changed |= ((c - centers[i]).length2() > SIMD_EPSILON);
					centers[i] = c;
					m_clusters[i]->m_nodes.resize(0);
				}
			}
			for (i = 0; i < m_nodes.size(); ++i)
			{
				const btVector3 nx = m_nodes[i].m_x;
				int kbest = 0;
				btScalar kdist = ClusterMetric(centers[0], nx);
				for (int j = 1; j < k; ++j)
				{
					const btScalar d = ClusterMetric(centers[j], nx);
					if (d < kdist)
					{
						kbest = j;
						kdist = d;
					}
				}
				m_clusters[kbest]->m_nodes.push_back(&m_nodes[i]);
			}
		} while (changed && (iterations < maxiterations));
		/* Merge		*/
		btAlignedObjectArray<int> cids;
		cids.resize(m_nodes.size(), -1);
		for (i = 0; i < m_clusters.size(); ++i)
		{
			for (int j = 0; j < m_clusters[i]->m_nodes.size(); ++j)
			{
				cids[int(m_clusters[i]->m_nodes[j] - &m_nodes[0])] = i;
			}
		}
		for (i = 0; i < m_faces.size(); ++i)
		{
			const int idx[] = {int(m_faces[i].m_n[0] - &m_nodes[0]),
							   int(m_faces[i].m_n[1] - &m_nodes[0]),
							   int(m_faces[i].m_n[2] - &m_nodes[0])};
			for (int j = 0; j < 3; ++j)
			{
				const int cid = cids[idx[j]];
				for (int q = 1; q < 3; ++q)
				{
					const int kid = idx[(j + q) % 3];
					if (cids[kid] != cid)
					{
						if (m_clusters[cid]->m_nodes.findLinearSearch(&m_nodes[kid]) == m_clusters[cid]->m_nodes.size())
						{
							m_clusters[cid]->m_nodes.push_back(&m_nodes[kid]);
						}
					}
				}
			}
		}
		/* Master		*/
		if (m_clusters.size() > 1)
		{
			Cluster* pmaster = new (btAlignedAlloc(sizeof(Cluster), 16)) Cluster();
			pmaster->m_collide = false;
			pmaster->m_nodes.reserve(m_nodes.size());
			for (int i = 0; i < m_nodes.size(); ++i) pmaster->m_nodes.push_back(&m_nodes[i]);
			m_clusters.push_back(pmaster);
			btSwap(m_clusters[0], m_clusters[m_clusters.size() - 1]);
		}
		/* Terminate	*/
		for (i = 0; i < m_clusters.size(); ++i)
		{
			if (m_clusters[i]->m_nodes.size() == 0)
			{
				releaseCluster(i--);
			}
		}
	}
	else
	{
		//create a cluster for each tetrahedron (if tetrahedra exist) or each face
		if (m_tetras.size())
		{
			m_clusters.resize(m_tetras.size());
			for (i = 0; i < m_clusters.size(); ++i)
			{
				m_clusters[i] = new (btAlignedAlloc(sizeof(Cluster), 16)) Cluster();
				m_clusters[i]->m_collide = true;
			}
			for (i = 0; i < m_tetras.size(); i++)
			{
				for (int j = 0; j < 4; j++)
				{
					m_clusters[i]->m_nodes.push_back(m_tetras[i].m_n[j]);
				}
			}
		}
		else
		{
			m_clusters.resize(m_faces.size());
			for (i = 0; i < m_clusters.size(); ++i)
			{
				m_clusters[i] = new (btAlignedAlloc(sizeof(Cluster), 16)) Cluster();
				m_clusters[i]->m_collide = true;
			}

			for (i = 0; i < m_faces.size(); ++i)
			{
				for (int j = 0; j < 3; ++j)
				{
					m_clusters[i]->m_nodes.push_back(m_faces[i].m_n[j]);
				}
			}
		}
	}

	if (m_clusters.size())
	{
		initializeClusters();
		updateClusters();

		//for self-collision
		m_clusterConnectivity.resize(m_clusters.size() * m_clusters.size());
		{
			for (int c0 = 0; c0 < m_clusters.size(); c0++)
			{
				m_clusters[c0]->m_clusterIndex = c0;
				for (int c1 = 0; c1 < m_clusters.size(); c1++)
				{
					bool connected = false;
					Cluster* cla = m_clusters[c0];
					Cluster* clb = m_clusters[c1];
					for (int i = 0; !connected && i < cla->m_nodes.size(); i++)
					{
						for (int j = 0; j < clb->m_nodes.size(); j++)
						{
							if (cla->m_nodes[i] == clb->m_nodes[j])
							{
								connected = true;
								break;
							}
						}
					}
					m_clusterConnectivity[c0 + c1 * m_clusters.size()] = connected;
				}
			}
		}
	}

	return (m_clusters.size());
}

//
void btSoftBody::refine(ImplicitFn* ifn, btScalar accurary, bool cut)
{
	const Node* nbase = &m_nodes[0];
	int ncount = m_nodes.size();
	btSymMatrix<int> edges(ncount, -2);
	int newnodes = 0;
	int i, j, k, ni;

	/* Filter out		*/
	for (i = 0; i < m_links.size(); ++i)
	{
		Link& l = m_links[i];
		if (l.m_bbending)
		{
			if (!SameSign(ifn->Eval(l.m_n[0]->m_x), ifn->Eval(l.m_n[1]->m_x)))
			{
				btSwap(m_links[i], m_links[m_links.size() - 1]);
				m_links.pop_back();
				--i;
			}
		}
	}
	/* Fill edges		*/
	for (i = 0; i < m_links.size(); ++i)
	{
		Link& l = m_links[i];
		edges(int(l.m_n[0] - nbase), int(l.m_n[1] - nbase)) = -1;
	}
	for (i = 0; i < m_faces.size(); ++i)
	{
		Face& f = m_faces[i];
		edges(int(f.m_n[0] - nbase), int(f.m_n[1] - nbase)) = -1;
		edges(int(f.m_n[1] - nbase), int(f.m_n[2] - nbase)) = -1;
		edges(int(f.m_n[2] - nbase), int(f.m_n[0] - nbase)) = -1;
	}
	/* Intersect		*/
	for (i = 0; i < ncount; ++i)
	{
		for (j = i + 1; j < ncount; ++j)
		{
			if (edges(i, j) == -1)
			{
				Node& a = m_nodes[i];
				Node& b = m_nodes[j];
				const btScalar t = ImplicitSolve(ifn, a.m_x, b.m_x, accurary);
				if (t > 0)
				{
					const btVector3 x = Lerp(a.m_x, b.m_x, t);
					const btVector3 v = Lerp(a.m_v, b.m_v, t);
					btScalar m = 0;
					if (a.m_im > 0)
					{
						if (b.m_im > 0)
						{
							const btScalar ma = 1 / a.m_im;
							const btScalar mb = 1 / b.m_im;
							const btScalar mc = Lerp(ma, mb, t);
							const btScalar f = (ma + mb) / (ma + mb + mc);
							a.m_im = 1 / (ma * f);
							b.m_im = 1 / (mb * f);
							m = mc * f;
						}
						else
						{
							a.m_im /= 0.5f;
							m = 1 / a.m_im;
						}
					}
					else
					{
						if (b.m_im > 0)
						{
							b.m_im /= 0.5f;
							m = 1 / b.m_im;
						}
						else
							m = 0;
					}
					appendNode(x, m);
					edges(i, j) = m_nodes.size() - 1;
					m_nodes[edges(i, j)].m_v = v;
					++newnodes;
				}
			}
		}
	}
	nbase = &m_nodes[0];
	/* Refine links		*/
	for (i = 0, ni = m_links.size(); i < ni; ++i)
	{
		Link& feat = m_links[i];
		const int idx[] = {int(feat.m_n[0] - nbase),
						   int(feat.m_n[1] - nbase)};
		if ((idx[0] < ncount) && (idx[1] < ncount))
		{
			const int ni = edges(idx[0], idx[1]);
			if (ni > 0)
			{
				appendLink(i);
				Link* pft[] = {&m_links[i],
							   &m_links[m_links.size() - 1]};
				pft[0]->m_n[0] = &m_nodes[idx[0]];
				pft[0]->m_n[1] = &m_nodes[ni];
				pft[1]->m_n[0] = &m_nodes[ni];
				pft[1]->m_n[1] = &m_nodes[idx[1]];
			}
		}
	}
	/* Refine faces		*/
	for (i = 0; i < m_faces.size(); ++i)
	{
		const Face& feat = m_faces[i];
		const int idx[] = {int(feat.m_n[0] - nbase),
						   int(feat.m_n[1] - nbase),
						   int(feat.m_n[2] - nbase)};
		for (j = 2, k = 0; k < 3; j = k++)
		{
			if ((idx[j] < ncount) && (idx[k] < ncount))
			{
				const int ni = edges(idx[j], idx[k]);
				if (ni > 0)
				{
					appendFace(i);
					const int l = (k + 1) % 3;
					Face* pft[] = {&m_faces[i],
								   &m_faces[m_faces.size() - 1]};
					pft[0]->m_n[0] = &m_nodes[idx[l]];
					pft[0]->m_n[1] = &m_nodes[idx[j]];
					pft[0]->m_n[2] = &m_nodes[ni];
					pft[1]->m_n[0] = &m_nodes[ni];
					pft[1]->m_n[1] = &m_nodes[idx[k]];
					pft[1]->m_n[2] = &m_nodes[idx[l]];
					appendLink(ni, idx[l], pft[0]->m_material);
					--i;
					break;
				}
			}
		}
	}
	/* Cut				*/
	if (cut)
	{
		btAlignedObjectArray<int> cnodes;
		const int pcount = ncount;
		int i;
		ncount = m_nodes.size();
		cnodes.resize(ncount, 0);
		/* Nodes		*/
		for (i = 0; i < ncount; ++i)
		{
			const btVector3 x = m_nodes[i].m_x;
			if ((i >= pcount) || (btFabs(ifn->Eval(x)) < accurary))
			{
				const btVector3 v = m_nodes[i].m_v;
				btScalar m = getMass(i);
				if (m > 0)
				{
					m *= 0.5f;
					m_nodes[i].m_im /= 0.5f;
				}
				appendNode(x, m);
				cnodes[i] = m_nodes.size() - 1;
				m_nodes[cnodes[i]].m_v = v;
			}
		}
		nbase = &m_nodes[0];
		/* Links		*/
		for (i = 0, ni = m_links.size(); i < ni; ++i)
		{
			const int id[] = {int(m_links[i].m_n[0] - nbase),
							  int(m_links[i].m_n[1] - nbase)};
			int todetach = 0;
			if (cnodes[id[0]] && cnodes[id[1]])
			{
				appendLink(i);
				todetach = m_links.size() - 1;
			}
			else
			{
				if (((ifn->Eval(m_nodes[id[0]].m_x) < accurary) &&
					 (ifn->Eval(m_nodes[id[1]].m_x) < accurary)))
					todetach = i;
			}
			if (todetach)
			{
				Link& l = m_links[todetach];
				for (int j = 0; j < 2; ++j)
				{
					int cn = cnodes[int(l.m_n[j] - nbase)];
					if (cn) l.m_n[j] = &m_nodes[cn];
				}
			}
		}
		/* Faces		*/
		for (i = 0, ni = m_faces.size(); i < ni; ++i)
		{
			Node** n = m_faces[i].m_n;
			if ((ifn->Eval(n[0]->m_x) < accurary) &&
				(ifn->Eval(n[1]->m_x) < accurary) &&
				(ifn->Eval(n[2]->m_x) < accurary))
			{
				for (int j = 0; j < 3; ++j)
				{
					int cn = cnodes[int(n[j] - nbase)];
					if (cn) n[j] = &m_nodes[cn];
				}
			}
		}
		/* Clean orphans	*/
		int nnodes = m_nodes.size();
		btAlignedObjectArray<int> ranks;
		btAlignedObjectArray<int> todelete;
		ranks.resize(nnodes, 0);
		for (i = 0, ni = m_links.size(); i < ni; ++i)
		{
			for (int j = 0; j < 2; ++j) ranks[int(m_links[i].m_n[j] - nbase)]++;
		}
		for (i = 0, ni = m_faces.size(); i < ni; ++i)
		{
			for (int j = 0; j < 3; ++j) ranks[int(m_faces[i].m_n[j] - nbase)]++;
		}
		for (i = 0; i < m_links.size(); ++i)
		{
			const int id[] = {int(m_links[i].m_n[0] - nbase),
							  int(m_links[i].m_n[1] - nbase)};
			const bool sg[] = {ranks[id[0]] == 1,
							   ranks[id[1]] == 1};
			if (sg[0] || sg[1])
			{
				--ranks[id[0]];
				--ranks[id[1]];
				btSwap(m_links[i], m_links[m_links.size() - 1]);
				m_links.pop_back();
				--i;
			}
		}
#if 0	
		for(i=nnodes-1;i>=0;--i)
		{
			if(!ranks[i]) todelete.push_back(i);
		}	
		if(todelete.size())
		{		
			btAlignedObjectArray<int>&	map=ranks;
			for(int i=0;i<nnodes;++i) map[i]=i;
			PointersToIndices(this);
			for(int i=0,ni=todelete.size();i<ni;++i)
			{
				int		j=todelete[i];
				int&	a=map[j];
				int&	b=map[--nnodes];
				m_ndbvt.remove(m_nodes[a].m_leaf);m_nodes[a].m_leaf=0;
				btSwap(m_nodes[a],m_nodes[b]);
				j=a;a=b;b=j;			
			}
			IndicesToPointers(this,&map[0]);
			m_nodes.resize(nnodes);
		}
#endif
	}
	m_bUpdateRtCst = true;
}

//
bool btSoftBody::cutLink(const Node* node0, const Node* node1, btScalar position)
{
	return (cutLink(int(node0 - &m_nodes[0]), int(node1 - &m_nodes[0]), position));
}

