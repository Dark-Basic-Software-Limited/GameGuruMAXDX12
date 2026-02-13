//
void btSoftBody::PSolve_SContacts(btSoftBody* psb, btScalar, btScalar ti)
{
	BT_PROFILE("PSolve_SContacts");

	for (int i = 0, ni = psb->m_scontacts.size(); i < ni; ++i)
	{
		const SContact& c = psb->m_scontacts[i];
		const btVector3& nr = c.m_normal;
		Node& n = *c.m_node;
		Face& f = *c.m_face;
		const btVector3 p = BaryEval(f.m_n[0]->m_x,
									 f.m_n[1]->m_x,
									 f.m_n[2]->m_x,
									 c.m_weights);
		const btVector3 q = BaryEval(f.m_n[0]->m_q,
									 f.m_n[1]->m_q,
									 f.m_n[2]->m_q,
									 c.m_weights);
		const btVector3 vr = (n.m_x - n.m_q) - (p - q);
		btVector3 corr(0, 0, 0);
		btScalar dot = btDot(vr, nr);
		if (dot < 0)
		{
			const btScalar j = c.m_margin - (btDot(nr, n.m_x) - btDot(nr, p));
			corr += c.m_normal * j;
		}
		corr -= ProjectOnPlane(vr, nr) * c.m_friction;
		n.m_x += corr * c.m_cfm[0];
		f.m_n[0]->m_x -= corr * (c.m_cfm[1] * c.m_weights.x());
		f.m_n[1]->m_x -= corr * (c.m_cfm[1] * c.m_weights.y());
		f.m_n[2]->m_x -= corr * (c.m_cfm[1] * c.m_weights.z());
	}
}

//
void btSoftBody::PSolve_Links(btSoftBody* psb, btScalar kst, btScalar ti)
{
	BT_PROFILE("PSolve_Links");
	for (int i = 0, ni = psb->m_links.size(); i < ni; ++i)
	{
		Link& l = psb->m_links[i];
		if (l.m_c0 > 0)
		{
			Node& a = *l.m_n[0];
			Node& b = *l.m_n[1];
			const btVector3 del = b.m_x - a.m_x;
			const btScalar len = del.length2();
			if (l.m_c1 + len > SIMD_EPSILON)
			{
				const btScalar k = ((l.m_c1 - len) / (l.m_c0 * (l.m_c1 + len))) * kst;
				a.m_x -= del * (k * a.m_im);
				b.m_x += del * (k * b.m_im);
			}
		}
	}
}

//
void btSoftBody::VSolve_Links(btSoftBody* psb, btScalar kst)
{
	BT_PROFILE("VSolve_Links");
	for (int i = 0, ni = psb->m_links.size(); i < ni; ++i)
	{
		Link& l = psb->m_links[i];
		Node** n = l.m_n;
		const btScalar j = -btDot(l.m_c3, n[0]->m_v - n[1]->m_v) * l.m_c2 * kst;
		n[0]->m_v += l.m_c3 * (j * n[0]->m_im);
		n[1]->m_v -= l.m_c3 * (j * n[1]->m_im);
	}
}

//
btSoftBody::psolver_t btSoftBody::getSolver(ePSolver::_ solver)
{
	switch (solver)
	{
		case ePSolver::Anchors:
			return (&btSoftBody::PSolve_Anchors);
		case ePSolver::Linear:
			return (&btSoftBody::PSolve_Links);
		case ePSolver::RContacts:
			return (&btSoftBody::PSolve_RContacts);
		case ePSolver::SContacts:
			return (&btSoftBody::PSolve_SContacts);
		default:
		{
		}
	}
	return (0);
}

//
btSoftBody::vsolver_t btSoftBody::getSolver(eVSolver::_ solver)
{
	switch (solver)
	{
		case eVSolver::Linear:
			return (&btSoftBody::VSolve_Links);
		default:
		{
		}
	}
	return (0);
}

void btSoftBody::setSelfCollision(bool useSelfCollision)
{
	m_useSelfCollision = useSelfCollision;
}

bool btSoftBody::useSelfCollision()
{
	return m_useSelfCollision;
}

//
void btSoftBody::defaultCollisionHandler(const btCollisionObjectWrapper* pcoWrap)
{
	switch (m_cfg.collisions & fCollision::RVSmask)
	{
		case fCollision::SDF_RS:
		{
			btSoftColliders::CollideSDF_RS docollide;
			btRigidBody* prb1 = (btRigidBody*)btRigidBody::upcast(pcoWrap->getCollisionObject());
			btTransform wtr = pcoWrap->getWorldTransform();

			const btTransform ctr = pcoWrap->getWorldTransform();
			const btScalar timemargin = (wtr.getOrigin() - ctr.getOrigin()).length();
			const btScalar basemargin = getCollisionShape()->getMargin();
			btVector3 mins;
			btVector3 maxs;
			ATTRIBUTE_ALIGNED16(btDbvtVolume)
			volume;
			pcoWrap->getCollisionShape()->getAabb(pcoWrap->getWorldTransform(),
												  mins,
												  maxs);
			volume = btDbvtVolume::FromMM(mins, maxs);
			volume.Expand(btVector3(basemargin, basemargin, basemargin));
			docollide.psb = this;
			docollide.m_colObj1Wrap = pcoWrap;
			docollide.m_rigidBody = prb1;

			docollide.dynmargin = basemargin + timemargin;
			docollide.stamargin = basemargin;
			m_ndbvt.collideTV(m_ndbvt.m_root, volume, docollide);
		}
		break;
		case fCollision::CL_RS:
		{
			btSoftColliders::CollideCL_RS collider;
			collider.ProcessColObj(this, pcoWrap);
		}
		break;
		case fCollision::SDF_RD:
		{
			btRigidBody* prb1 = (btRigidBody*)btRigidBody::upcast(pcoWrap->getCollisionObject());
			if (this->isActive())
			{
				const btTransform wtr = pcoWrap->getWorldTransform();
				const btScalar timemargin = 0;
				const btScalar basemargin = getCollisionShape()->getMargin();
				btVector3 mins;
				btVector3 maxs;
				ATTRIBUTE_ALIGNED16(btDbvtVolume)
				volume;
				pcoWrap->getCollisionShape()->getAabb(wtr,
													  mins,
													  maxs);
				volume = btDbvtVolume::FromMM(mins, maxs);
				volume.Expand(btVector3(basemargin, basemargin, basemargin));
				if (m_cfg.collisions & fCollision::SDF_RDN)
				{
					btSoftColliders::CollideSDF_RD docollideNode;
					docollideNode.psb = this;
					docollideNode.m_colObj1Wrap = pcoWrap;
					docollideNode.m_rigidBody = prb1;
					docollideNode.dynmargin = basemargin + timemargin;
					docollideNode.stamargin = basemargin;
					m_ndbvt.collideTV(m_ndbvt.m_root, volume, docollideNode);
				}

				if (((pcoWrap->getCollisionObject()->getInternalType() == CO_RIGID_BODY) && (m_cfg.collisions & fCollision::SDF_RDF)) || ((pcoWrap->getCollisionObject()->getInternalType() == CO_FEATHERSTONE_LINK) && (m_cfg.collisions & fCollision::SDF_MDF)))
				{
					btSoftColliders::CollideSDF_RDF docollideFace;
					docollideFace.psb = this;
					docollideFace.m_colObj1Wrap = pcoWrap;
					docollideFace.m_rigidBody = prb1;
					docollideFace.dynmargin = basemargin + timemargin;
					docollideFace.stamargin = basemargin;
					m_fdbvt.collideTV(m_fdbvt.m_root, volume, docollideFace);
				}
			}
		}
		break;
	}
}

//
void btSoftBody::defaultCollisionHandler(btSoftBody* psb)
{
	BT_PROFILE("Deformable Collision");
	const int cf = m_cfg.collisions & psb->m_cfg.collisions;
	switch (cf & fCollision::SVSmask)
	{
		case fCollision::CL_SS:
		{
			//support self-collision if CL_SELF flag set
			if (this != psb || psb->m_cfg.collisions & fCollision::CL_SELF)
			{
				btSoftColliders::CollideCL_SS docollide;
				docollide.ProcessSoftSoft(this, psb);
			}
		}
		break;
		case fCollision::VF_SS:
		{
			//only self-collision for Cluster, not Vertex-Face yet
			if (this != psb)
			{
				btSoftColliders::CollideVF_SS docollide;
				/* common					*/
				docollide.mrg = getCollisionShape()->getMargin() +
								psb->getCollisionShape()->getMargin();
				/* psb0 nodes vs psb1 faces	*/
				docollide.psb[0] = this;
				docollide.psb[1] = psb;
				docollide.psb[0]->m_ndbvt.collideTT(docollide.psb[0]->m_ndbvt.m_root,
													docollide.psb[1]->m_fdbvt.m_root,
													docollide);
				/* psb1 nodes vs psb0 faces	*/
				docollide.psb[0] = psb;
				docollide.psb[1] = this;
				docollide.psb[0]->m_ndbvt.collideTT(docollide.psb[0]->m_ndbvt.m_root,
													docollide.psb[1]->m_fdbvt.m_root,
													docollide);
			}
		}
		break;
		case fCollision::VF_DD:
		{
			if (!psb->m_softSoftCollision)
				return;
			if (psb->isActive() || this->isActive())
			{
				if (this != psb)
				{
					btSoftColliders::CollideVF_DD docollide;
					/* common                    */
					docollide.mrg = getCollisionShape()->getMargin() +
									psb->getCollisionShape()->getMargin();
					/* psb0 nodes vs psb1 faces    */
					if (psb->m_tetras.size() > 0)
						docollide.useFaceNormal = true;
					else
						docollide.useFaceNormal = false;
					docollide.psb[0] = this;
					docollide.psb[1] = psb;
					docollide.psb[0]->m_ndbvt.collideTT(docollide.psb[0]->m_ndbvt.m_root,
														docollide.psb[1]->m_fdbvt.m_root,
														docollide);

					/* psb1 nodes vs psb0 faces    */
					if (this->m_tetras.size() > 0)
						docollide.useFaceNormal = true;
					else
						docollide.useFaceNormal = false;
					docollide.psb[0] = psb;
					docollide.psb[1] = this;
					docollide.psb[0]->m_ndbvt.collideTT(docollide.psb[0]->m_ndbvt.m_root,
														docollide.psb[1]->m_fdbvt.m_root,
														docollide);
				}
				else
				{
					if (psb->useSelfCollision())
					{
						btSoftColliders::CollideFF_DD docollide;
						docollide.mrg = 2 * getCollisionShape()->getMargin();
						docollide.psb[0] = this;
						docollide.psb[1] = psb;
						if (this->m_tetras.size() > 0)
							docollide.useFaceNormal = true;
						else
							docollide.useFaceNormal = false;
						/* psb0 faces vs psb0 faces    */
						calculateNormalCone(this->m_fdbvnt);
						this->m_fdbvt.selfCollideT(m_fdbvnt, docollide);
					}
				}
			}
		}
		break;
		default:
		{
		}
	}
}

void btSoftBody::geometricCollisionHandler(btSoftBody* psb)
{
	if (psb->isActive() || this->isActive())
	{
		if (this != psb)
		{
			btSoftColliders::CollideCCD docollide;
			/* common                    */
			docollide.mrg = SAFE_EPSILON;  // for rounding error instead of actual margin
			docollide.dt = psb->m_sst.sdt;
			/* psb0 nodes vs psb1 faces    */
			if (psb->m_tetras.size() > 0)
				docollide.useFaceNormal = true;
			else
				docollide.useFaceNormal = false;
			docollide.psb[0] = this;
			docollide.psb[1] = psb;
			docollide.psb[0]->m_ndbvt.collideTT(docollide.psb[0]->m_ndbvt.m_root,
												docollide.psb[1]->m_fdbvt.m_root,
												docollide);
			/* psb1 nodes vs psb0 faces    */
			if (this->m_tetras.size() > 0)
				docollide.useFaceNormal = true;
			else
				docollide.useFaceNormal = false;
			docollide.psb[0] = psb;
			docollide.psb[1] = this;
			docollide.psb[0]->m_ndbvt.collideTT(docollide.psb[0]->m_ndbvt.m_root,
												docollide.psb[1]->m_fdbvt.m_root,
												docollide);
		}
		else
		{
			if (psb->useSelfCollision())
			{
				btSoftColliders::CollideCCD docollide;
				docollide.mrg = SAFE_EPSILON;
				docollide.psb[0] = this;
				docollide.psb[1] = psb;
				docollide.dt = psb->m_sst.sdt;
				if (this->m_tetras.size() > 0)
					docollide.useFaceNormal = true;
				else
					docollide.useFaceNormal = false;
				/* psb0 faces vs psb0 faces    */
				calculateNormalCone(this->m_fdbvnt);  // should compute this outside of this scope
				this->m_fdbvt.selfCollideT(m_fdbvnt, docollide);
			}
		}
	}
}

void btSoftBody::setWindVelocity(const btVector3& velocity)
{
	m_windVelocity = velocity;
}

const btVector3& btSoftBody::getWindVelocity()
{
	return m_windVelocity;
}

int btSoftBody::calculateSerializeBufferSize() const
{
	int sz = sizeof(btSoftBodyData);
	return sz;
}

///fills the dataBuffer and returns the struct name (and 0 on failure)
const char* btSoftBody::serialize(void* dataBuffer, class btSerializer* serializer) const
{
	btSoftBodyData* sbd = (btSoftBodyData*)dataBuffer;

	btCollisionObject::serialize(&sbd->m_collisionObjectData, serializer);

	btHashMap<btHashPtr, int> m_nodeIndexMap;

	sbd->m_numMaterials = m_materials.size();
	sbd->m_materials = sbd->m_numMaterials ? (SoftBodyMaterialData**)serializer->getUniquePointer((void*)&m_materials) : 0;

	if (sbd->m_materials)
	{
		int sz = sizeof(SoftBodyMaterialData*);
		int numElem = sbd->m_numMaterials;
		btChunk* chunk = serializer->allocate(sz, numElem);
		//SoftBodyMaterialData** memPtr = chunk->m_oldPtr;
		SoftBodyMaterialData** memPtr = (SoftBodyMaterialData**)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			btSoftBody::Material* mat = m_materials[i];
			*memPtr = mat ? (SoftBodyMaterialData*)serializer->getUniquePointer((void*)mat) : 0;
			if (!serializer->findPointer(mat))
			{
				//serialize it here
				btChunk* chunk = serializer->allocate(sizeof(SoftBodyMaterialData), 1);
				SoftBodyMaterialData* memPtr = (SoftBodyMaterialData*)chunk->m_oldPtr;
				memPtr->m_flags = mat->m_flags;
				memPtr->m_angularStiffness = mat->m_kAST;
				memPtr->m_linearStiffness = mat->m_kLST;
				memPtr->m_volumeStiffness = mat->m_kVST;
				serializer->finalizeChunk(chunk, "SoftBodyMaterialData", BT_SBMATERIAL_CODE, mat);
			}
		}
		serializer->finalizeChunk(chunk, "SoftBodyMaterialData", BT_ARRAY_CODE, (void*)&m_materials);
	}

	sbd->m_numNodes = m_nodes.size();
	sbd->m_nodes = sbd->m_numNodes ? (SoftBodyNodeData*)serializer->getUniquePointer((void*)&m_nodes) : 0;
	if (sbd->m_nodes)
	{
		int sz = sizeof(SoftBodyNodeData);
		int numElem = sbd->m_numNodes;
		btChunk* chunk = serializer->allocate(sz, numElem);
		SoftBodyNodeData* memPtr = (SoftBodyNodeData*)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			m_nodes[i].m_f.serializeFloat(memPtr->m_accumulatedForce);
			memPtr->m_area = m_nodes[i].m_area;
			memPtr->m_attach = m_nodes[i].m_battach;
			memPtr->m_inverseMass = m_nodes[i].m_im;
			memPtr->m_material = m_nodes[i].m_material ? (SoftBodyMaterialData*)serializer->getUniquePointer((void*)m_nodes[i].m_material) : 0;
			m_nodes[i].m_n.serializeFloat(memPtr->m_normal);
			m_nodes[i].m_x.serializeFloat(memPtr->m_position);
			m_nodes[i].m_q.serializeFloat(memPtr->m_previousPosition);
			m_nodes[i].m_v.serializeFloat(memPtr->m_velocity);
			m_nodeIndexMap.insert(&m_nodes[i], i);
		}
		serializer->finalizeChunk(chunk, "SoftBodyNodeData", BT_SBNODE_CODE, (void*)&m_nodes);
	}

	sbd->m_numLinks = m_links.size();
	sbd->m_links = sbd->m_numLinks ? (SoftBodyLinkData*)serializer->getUniquePointer((void*)&m_links[0]) : 0;
	if (sbd->m_links)
	{
		int sz = sizeof(SoftBodyLinkData);
		int numElem = sbd->m_numLinks;
		btChunk* chunk = serializer->allocate(sz, numElem);
		SoftBodyLinkData* memPtr = (SoftBodyLinkData*)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			memPtr->m_bbending = m_links[i].m_bbending;
			memPtr->m_material = m_links[i].m_material ? (SoftBodyMaterialData*)serializer->getUniquePointer((void*)m_links[i].m_material) : 0;
			memPtr->m_nodeIndices[0] = m_links[i].m_n[0] ? m_links[i].m_n[0] - &m_nodes[0] : -1;
			memPtr->m_nodeIndices[1] = m_links[i].m_n[1] ? m_links[i].m_n[1] - &m_nodes[0] : -1;
			btAssert(memPtr->m_nodeIndices[0] < m_nodes.size());
			btAssert(memPtr->m_nodeIndices[1] < m_nodes.size());
			memPtr->m_restLength = m_links[i].m_rl;
		}
		serializer->finalizeChunk(chunk, "SoftBodyLinkData", BT_ARRAY_CODE, (void*)&m_links[0]);
	}

	sbd->m_numFaces = m_faces.size();
	sbd->m_faces = sbd->m_numFaces ? (SoftBodyFaceData*)serializer->getUniquePointer((void*)&m_faces[0]) : 0;
	if (sbd->m_faces)
	{
		int sz = sizeof(SoftBodyFaceData);
		int numElem = sbd->m_numFaces;
		btChunk* chunk = serializer->allocate(sz, numElem);
		SoftBodyFaceData* memPtr = (SoftBodyFaceData*)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			memPtr->m_material = m_faces[i].m_material ? (SoftBodyMaterialData*)serializer->getUniquePointer((void*)m_faces[i].m_material) : 0;
			m_faces[i].m_normal.serializeFloat(memPtr->m_normal);
			for (int j = 0; j < 3; j++)
			{
				memPtr->m_nodeIndices[j] = m_faces[i].m_n[j] ? m_faces[i].m_n[j] - &m_nodes[0] : -1;
			}
			memPtr->m_restArea = m_faces[i].m_ra;
		}
		serializer->finalizeChunk(chunk, "SoftBodyFaceData", BT_ARRAY_CODE, (void*)&m_faces[0]);
	}

	sbd->m_numTetrahedra = m_tetras.size();
	sbd->m_tetrahedra = sbd->m_numTetrahedra ? (SoftBodyTetraData*)serializer->getUniquePointer((void*)&m_tetras[0]) : 0;
	if (sbd->m_tetrahedra)
	{
		int sz = sizeof(SoftBodyTetraData);
		int numElem = sbd->m_numTetrahedra;
		btChunk* chunk = serializer->allocate(sz, numElem);
		SoftBodyTetraData* memPtr = (SoftBodyTetraData*)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			for (int j = 0; j < 4; j++)
			{
				m_tetras[i].m_c0[j].serializeFloat(memPtr->m_c0[j]);
				memPtr->m_nodeIndices[j] = m_tetras[i].m_n[j] ? m_tetras[i].m_n[j] - &m_nodes[0] : -1;
			}
			memPtr->m_c1 = m_tetras[i].m_c1;
			memPtr->m_c2 = m_tetras[i].m_c2;
			memPtr->m_material = m_tetras[i].m_material ? (SoftBodyMaterialData*)serializer->getUniquePointer((void*)m_tetras[i].m_material) : 0;
			memPtr->m_restVolume = m_tetras[i].m_rv;
		}
		serializer->finalizeChunk(chunk, "SoftBodyTetraData", BT_ARRAY_CODE, (void*)&m_tetras[0]);
	}

	sbd->m_numAnchors = m_anchors.size();
	sbd->m_anchors = sbd->m_numAnchors ? (SoftRigidAnchorData*)serializer->getUniquePointer((void*)&m_anchors[0]) : 0;
	if (sbd->m_anchors)
	{
		int sz = sizeof(SoftRigidAnchorData);
		int numElem = sbd->m_numAnchors;
		btChunk* chunk = serializer->allocate(sz, numElem);
		SoftRigidAnchorData* memPtr = (SoftRigidAnchorData*)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			m_anchors[i].m_c0.serializeFloat(memPtr->m_c0);
			m_anchors[i].m_c1.serializeFloat(memPtr->m_c1);
			memPtr->m_c2 = m_anchors[i].m_c2;
			m_anchors[i].m_local.serializeFloat(memPtr->m_localFrame);
			memPtr->m_nodeIndex = m_anchors[i].m_node ? m_anchors[i].m_node - &m_nodes[0] : -1;

			memPtr->m_rigidBody = m_anchors[i].m_body ? (btRigidBodyData*)serializer->getUniquePointer((void*)m_anchors[i].m_body) : 0;
			btAssert(memPtr->m_nodeIndex < m_nodes.size());
		}
		serializer->finalizeChunk(chunk, "SoftRigidAnchorData", BT_ARRAY_CODE, (void*)&m_anchors[0]);
	}

	sbd->m_config.m_dynamicFriction = m_cfg.kDF;
	sbd->m_config.m_baumgarte = m_cfg.kVCF;
	sbd->m_config.m_pressure = m_cfg.kPR;
	sbd->m_config.m_aeroModel = this->m_cfg.aeromodel;
	sbd->m_config.m_lift = m_cfg.kLF;
	sbd->m_config.m_drag = m_cfg.kDG;
	sbd->m_config.m_positionIterations = m_cfg.piterations;
	sbd->m_config.m_driftIterations = m_cfg.diterations;
	sbd->m_config.m_clusterIterations = m_cfg.citerations;
	sbd->m_config.m_velocityIterations = m_cfg.viterations;
	sbd->m_config.m_maxVolume = m_cfg.maxvolume;
	sbd->m_config.m_damping = m_cfg.kDP;
	sbd->m_config.m_poseMatch = m_cfg.kMT;
	sbd->m_config.m_collisionFlags = m_cfg.collisions;
	sbd->m_config.m_volume = m_cfg.kVC;
	sbd->m_config.m_rigidContactHardness = m_cfg.kCHR;
	sbd->m_config.m_kineticContactHardness = m_cfg.kKHR;
	sbd->m_config.m_softContactHardness = m_cfg.kSHR;
	sbd->m_config.m_anchorHardness = m_cfg.kAHR;
	sbd->m_config.m_timeScale = m_cfg.timescale;
	sbd->m_config.m_maxVolume = m_cfg.maxvolume;
	sbd->m_config.m_softRigidClusterHardness = m_cfg.kSRHR_CL;
	sbd->m_config.m_softKineticClusterHardness = m_cfg.kSKHR_CL;
	sbd->m_config.m_softSoftClusterHardness = m_cfg.kSSHR_CL;
	sbd->m_config.m_softRigidClusterImpulseSplit = m_cfg.kSR_SPLT_CL;
	sbd->m_config.m_softKineticClusterImpulseSplit = m_cfg.kSK_SPLT_CL;
	sbd->m_config.m_softSoftClusterImpulseSplit = m_cfg.kSS_SPLT_CL;

	//pose for shape matching
	{
		sbd->m_pose = (SoftBodyPoseData*)serializer->getUniquePointer((void*)&m_pose);

		int sz = sizeof(SoftBodyPoseData);
		btChunk* chunk = serializer->allocate(sz, 1);
		SoftBodyPoseData* memPtr = (SoftBodyPoseData*)chunk->m_oldPtr;

		m_pose.m_aqq.serializeFloat(memPtr->m_aqq);
		memPtr->m_bframe = m_pose.m_bframe;
		memPtr->m_bvolume = m_pose.m_bvolume;
		m_pose.m_com.serializeFloat(memPtr->m_com);

		memPtr->m_numPositions = m_pose.m_pos.size();
		memPtr->m_positions = memPtr->m_numPositions ? (btVector3FloatData*)serializer->getUniquePointer((void*)&m_pose.m_pos[0]) : 0;
		if (memPtr->m_numPositions)
		{
			int numElem = memPtr->m_numPositions;
			int sz = sizeof(btVector3Data);
			btChunk* chunk = serializer->allocate(sz, numElem);
			btVector3FloatData* memPtr = (btVector3FloatData*)chunk->m_oldPtr;
			for (int i = 0; i < numElem; i++, memPtr++)
			{
				m_pose.m_pos[i].serializeFloat(*memPtr);
			}
			serializer->finalizeChunk(chunk, "btVector3FloatData", BT_ARRAY_CODE, (void*)&m_pose.m_pos[0]);
		}
		memPtr->m_restVolume = m_pose.m_volume;
		m_pose.m_rot.serializeFloat(memPtr->m_rot);
		m_pose.m_scl.serializeFloat(memPtr->m_scale);

		memPtr->m_numWeigts = m_pose.m_wgh.size();
		memPtr->m_weights = memPtr->m_numWeigts ? (float*)serializer->getUniquePointer((void*)&m_pose.m_wgh[0]) : 0;
		if (memPtr->m_numWeigts)
		{
			int numElem = memPtr->m_numWeigts;
			int sz = sizeof(float);
			btChunk* chunk = serializer->allocate(sz, numElem);
			float* memPtr = (float*)chunk->m_oldPtr;
			for (int i = 0; i < numElem; i++, memPtr++)
			{
				*memPtr = m_pose.m_wgh[i];
			}
			serializer->finalizeChunk(chunk, "float", BT_ARRAY_CODE, (void*)&m_pose.m_wgh[0]);
		}

		serializer->finalizeChunk(chunk, "SoftBodyPoseData", BT_ARRAY_CODE, (void*)&m_pose);
	}

	//clusters for convex-cluster collision detection

	sbd->m_numClusters = m_clusters.size();
	sbd->m_clusters = sbd->m_numClusters ? (SoftBodyClusterData*)serializer->getUniquePointer((void*)m_clusters[0]) : 0;
	if (sbd->m_numClusters)
	{
		int numElem = sbd->m_numClusters;
		int sz = sizeof(SoftBodyClusterData);
		btChunk* chunk = serializer->allocate(sz, numElem);
		SoftBodyClusterData* memPtr = (SoftBodyClusterData*)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			memPtr->m_adamping = m_clusters[i]->m_adamping;
			m_clusters[i]->m_av.serializeFloat(memPtr->m_av);
			memPtr->m_clusterIndex = m_clusters[i]->m_clusterIndex;
			memPtr->m_collide = m_clusters[i]->m_collide;
			m_clusters[i]->m_com.serializeFloat(memPtr->m_com);
			memPtr->m_containsAnchor = m_clusters[i]->m_containsAnchor;
			m_clusters[i]->m_dimpulses[0].serializeFloat(memPtr->m_dimpulses[0]);
			m_clusters[i]->m_dimpulses[1].serializeFloat(memPtr->m_dimpulses[1]);
			m_clusters[i]->m_framexform.serializeFloat(memPtr->m_framexform);
			memPtr->m_idmass = m_clusters[i]->m_idmass;
			memPtr->m_imass = m_clusters[i]->m_imass;
			m_clusters[i]->m_invwi.serializeFloat(memPtr->m_invwi);
			memPtr->m_ldamping = m_clusters[i]->m_ldamping;
			m_clusters[i]->m_locii.serializeFloat(memPtr->m_locii);
			m_clusters[i]->m_lv.serializeFloat(memPtr->m_lv);
			memPtr->m_matching = m_clusters[i]->m_matching;
			memPtr->m_maxSelfCollisionImpulse = m_clusters[i]->m_maxSelfCollisionImpulse;
			memPtr->m_ndamping = m_clusters[i]->m_ndamping;
			memPtr->m_ldamping = m_clusters[i]->m_ldamping;
			memPtr->m_adamping = m_clusters[i]->m_adamping;
			memPtr->m_selfCollisionImpulseFactor = m_clusters[i]->m_selfCollisionImpulseFactor;

			memPtr->m_numFrameRefs = m_clusters[i]->m_framerefs.size();
			memPtr->m_numMasses = m_clusters[i]->m_masses.size();
			memPtr->m_numNodes = m_clusters[i]->m_nodes.size();

			memPtr->m_nvimpulses = m_clusters[i]->m_nvimpulses;
			m_clusters[i]->m_vimpulses[0].serializeFloat(memPtr->m_vimpulses[0]);
			m_clusters[i]->m_vimpulses[1].serializeFloat(memPtr->m_vimpulses[1]);
			memPtr->m_ndimpulses = m_clusters[i]->m_ndimpulses;

			memPtr->m_framerefs = memPtr->m_numFrameRefs ? (btVector3FloatData*)serializer->getUniquePointer((void*)&m_clusters[i]->m_framerefs[0]) : 0;
			if (memPtr->m_framerefs)
			{
				int numElem = memPtr->m_numFrameRefs;
				int sz = sizeof(btVector3FloatData);
				btChunk* chunk = serializer->allocate(sz, numElem);
				btVector3FloatData* memPtr = (btVector3FloatData*)chunk->m_oldPtr;
				for (int j = 0; j < numElem; j++, memPtr++)
				{
					m_clusters[i]->m_framerefs[j].serializeFloat(*memPtr);
				}
				serializer->finalizeChunk(chunk, "btVector3FloatData", BT_ARRAY_CODE, (void*)&m_clusters[i]->m_framerefs[0]);
			}

			memPtr->m_masses = memPtr->m_numMasses ? (float*)serializer->getUniquePointer((void*)&m_clusters[i]->m_masses[0]) : 0;
			if (memPtr->m_masses)
			{
				int numElem = memPtr->m_numMasses;
				int sz = sizeof(float);
				btChunk* chunk = serializer->allocate(sz, numElem);
				float* memPtr = (float*)chunk->m_oldPtr;
				for (int j = 0; j < numElem; j++, memPtr++)
				{
					*memPtr = m_clusters[i]->m_masses[j];
				}
				serializer->finalizeChunk(chunk, "float", BT_ARRAY_CODE, (void*)&m_clusters[i]->m_masses[0]);
			}

			memPtr->m_nodeIndices = memPtr->m_numNodes ? (int*)serializer->getUniquePointer((void*)&m_clusters[i]->m_nodes) : 0;
			if (memPtr->m_nodeIndices)
			{
				int numElem = memPtr->m_numMasses;
				int sz = sizeof(int);
				btChunk* chunk = serializer->allocate(sz, numElem);
				int* memPtr = (int*)chunk->m_oldPtr;
				for (int j = 0; j < numElem; j++, memPtr++)
				{
					int* indexPtr = m_nodeIndexMap.find(m_clusters[i]->m_nodes[j]);
					btAssert(indexPtr);
					*memPtr = *indexPtr;
				}
				serializer->finalizeChunk(chunk, "int", BT_ARRAY_CODE, (void*)&m_clusters[i]->m_nodes);
			}
		}
		serializer->finalizeChunk(chunk, "SoftBodyClusterData", BT_ARRAY_CODE, (void*)m_clusters[0]);
	}

	sbd->m_numJoints = m_joints.size();
	sbd->m_joints = m_joints.size() ? (btSoftBodyJointData*)serializer->getUniquePointer((void*)&m_joints[0]) : 0;

	if (sbd->m_joints)
	{
		int sz = sizeof(btSoftBodyJointData);
		int numElem = m_joints.size();
		btChunk* chunk = serializer->allocate(sz, numElem);
		btSoftBodyJointData* memPtr = (btSoftBodyJointData*)chunk->m_oldPtr;

		for (int i = 0; i < numElem; i++, memPtr++)
		{
			memPtr->m_jointType = (int)m_joints[i]->Type();
			m_joints[i]->m_refs[0].serializeFloat(memPtr->m_refs[0]);
			m_joints[i]->m_refs[1].serializeFloat(memPtr->m_refs[1]);
			memPtr->m_cfm = m_joints[i]->m_cfm;
			memPtr->m_erp = float(m_joints[i]->m_erp);
			memPtr->m_split = float(m_joints[i]->m_split);
			memPtr->m_delete = m_joints[i]->m_delete;

			for (int j = 0; j < 4; j++)
			{
				memPtr->m_relPosition[0].m_floats[j] = 0.f;
				memPtr->m_relPosition[1].m_floats[j] = 0.f;
			}
			memPtr->m_bodyA = 0;
			memPtr->m_bodyB = 0;
			if (m_joints[i]->m_bodies[0].m_soft)
			{
				memPtr->m_bodyAtype = BT_JOINT_SOFT_BODY_CLUSTER;
				memPtr->m_bodyA = serializer->getUniquePointer((void*)m_joints[i]->m_bodies[0].m_soft);
			}
			if (m_joints[i]->m_bodies[0].m_collisionObject)
			{
				memPtr->m_bodyAtype = BT_JOINT_COLLISION_OBJECT;
				memPtr->m_bodyA = serializer->getUniquePointer((void*)m_joints[i]->m_bodies[0].m_collisionObject);
			}
			if (m_joints[i]->m_bodies[0].m_rigid)
			{
				memPtr->m_bodyAtype = BT_JOINT_RIGID_BODY;
				memPtr->m_bodyA = serializer->getUniquePointer((void*)m_joints[i]->m_bodies[0].m_rigid);
			}

			if (m_joints[i]->m_bodies[1].m_soft)
			{
				memPtr->m_bodyBtype = BT_JOINT_SOFT_BODY_CLUSTER;
				memPtr->m_bodyB = serializer->getUniquePointer((void*)m_joints[i]->m_bodies[1].m_soft);
			}
			if (m_joints[i]->m_bodies[1].m_collisionObject)
			{
				memPtr->m_bodyBtype = BT_JOINT_COLLISION_OBJECT;
				memPtr->m_bodyB = serializer->getUniquePointer((void*)m_joints[i]->m_bodies[1].m_collisionObject);
			}
			if (m_joints[i]->m_bodies[1].m_rigid)
			{
				memPtr->m_bodyBtype = BT_JOINT_RIGID_BODY;
				memPtr->m_bodyB = serializer->getUniquePointer((void*)m_joints[i]->m_bodies[1].m_rigid);
			}
		}
		serializer->finalizeChunk(chunk, "btSoftBodyJointData", BT_ARRAY_CODE, (void*)&m_joints[0]);
	}

	return btSoftBodyDataName;
}

void btSoftBody::updateDeactivation(btScalar timeStep)
{
	if ((getActivationState() == ISLAND_SLEEPING) || (getActivationState() == DISABLE_DEACTIVATION))
		return;

	if (m_maxSpeedSquared < m_sleepingThreshold * m_sleepingThreshold)
	{
		m_deactivationTime += timeStep;
	}
	else
	{
		m_deactivationTime = btScalar(0.);
		setActivationState(0);
	}
}

void btSoftBody::setZeroVelocity()
{
	for (int i = 0; i < m_nodes.size(); ++i)
	{
		m_nodes[i].m_v.setZero();
	}
}

bool btSoftBody::wantsSleeping()
{
	if (getActivationState() == DISABLE_DEACTIVATION)
		return false;

	//disable deactivation
	if (gDisableDeactivation || (gDeactivationTime == btScalar(0.)))
		return false;

	if ((getActivationState() == ISLAND_SLEEPING) || (getActivationState() == WANTS_DEACTIVATION))
		return true;

	if (m_deactivationTime > gDeactivationTime)
	{
		return true;
	}
	return false;
}
