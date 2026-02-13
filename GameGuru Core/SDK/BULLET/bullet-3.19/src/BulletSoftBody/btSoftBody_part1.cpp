//
bool btSoftBody::cutLink(int node0, int node1, btScalar position)
{
	bool done = false;
	int i, ni;
	//	const btVector3	d=m_nodes[node0].m_x-m_nodes[node1].m_x;
	const btVector3 x = Lerp(m_nodes[node0].m_x, m_nodes[node1].m_x, position);
	const btVector3 v = Lerp(m_nodes[node0].m_v, m_nodes[node1].m_v, position);
	const btScalar m = 1;
	appendNode(x, m);
	appendNode(x, m);
	Node* pa = &m_nodes[node0];
	Node* pb = &m_nodes[node1];
	Node* pn[2] = {&m_nodes[m_nodes.size() - 2],
				   &m_nodes[m_nodes.size() - 1]};
	pn[0]->m_v = v;
	pn[1]->m_v = v;
	for (i = 0, ni = m_links.size(); i < ni; ++i)
	{
		const int mtch = MatchEdge(m_links[i].m_n[0], m_links[i].m_n[1], pa, pb);
		if (mtch != -1)
		{
			appendLink(i);
			Link* pft[] = {&m_links[i], &m_links[m_links.size() - 1]};
			pft[0]->m_n[1] = pn[mtch];
			pft[1]->m_n[0] = pn[1 - mtch];
			done = true;
		}
	}
	for (i = 0, ni = m_faces.size(); i < ni; ++i)
	{
		for (int k = 2, l = 0; l < 3; k = l++)
		{
			const int mtch = MatchEdge(m_faces[i].m_n[k], m_faces[i].m_n[l], pa, pb);
			if (mtch != -1)
			{
				appendFace(i);
				Face* pft[] = {&m_faces[i], &m_faces[m_faces.size() - 1]};
				pft[0]->m_n[l] = pn[mtch];
				pft[1]->m_n[k] = pn[1 - mtch];
				appendLink(pn[0], pft[0]->m_n[(l + 1) % 3], pft[0]->m_material, true);
				appendLink(pn[1], pft[0]->m_n[(l + 1) % 3], pft[0]->m_material, true);
			}
		}
	}
	if (!done)
	{
		m_ndbvt.remove(pn[0]->m_leaf);
		m_ndbvt.remove(pn[1]->m_leaf);
		m_nodes.pop_back();
		m_nodes.pop_back();
	}
	return (done);
}

//
bool btSoftBody::rayTest(const btVector3& rayFrom,
						 const btVector3& rayTo,
						 sRayCast& results)
{
	if (m_faces.size() && m_fdbvt.empty())
		initializeFaceTree();

	results.body = this;
	results.fraction = 1.f;
	results.feature = eFeature::None;
	results.index = -1;

	return (rayTest(rayFrom, rayTo, results.fraction, results.feature, results.index, false) != 0);
}

bool btSoftBody::rayFaceTest(const btVector3& rayFrom,
							 const btVector3& rayTo,
							 sRayCast& results)
{
	if (m_faces.size() == 0)
		return false;
	else
	{
		if (m_fdbvt.empty())
			initializeFaceTree();
	}

	results.body = this;
	results.fraction = 1.f;
	results.index = -1;

	return (rayFaceTest(rayFrom, rayTo, results.fraction, results.index) != 0);
}

//
void btSoftBody::setSolver(eSolverPresets::_ preset)
{
	m_cfg.m_vsequence.clear();
	m_cfg.m_psequence.clear();
	m_cfg.m_dsequence.clear();
	switch (preset)
	{
		case eSolverPresets::Positions:
			m_cfg.m_psequence.push_back(ePSolver::Anchors);
			m_cfg.m_psequence.push_back(ePSolver::RContacts);
			m_cfg.m_psequence.push_back(ePSolver::SContacts);
			m_cfg.m_psequence.push_back(ePSolver::Linear);
			break;
		case eSolverPresets::Velocities:
			m_cfg.m_vsequence.push_back(eVSolver::Linear);

			m_cfg.m_psequence.push_back(ePSolver::Anchors);
			m_cfg.m_psequence.push_back(ePSolver::RContacts);
			m_cfg.m_psequence.push_back(ePSolver::SContacts);

			m_cfg.m_dsequence.push_back(ePSolver::Linear);
			break;
	}
}

void btSoftBody::predictMotion(btScalar dt)
{
	int i, ni;

	/* Update                */
	if (m_bUpdateRtCst)
	{
		m_bUpdateRtCst = false;
		updateConstants();
		m_fdbvt.clear();
		if (m_cfg.collisions & fCollision::VF_SS)
		{
			initializeFaceTree();
		}
	}

	/* Prepare                */
	m_sst.sdt = dt * m_cfg.timescale;
	m_sst.isdt = 1 / m_sst.sdt;
	m_sst.velmrg = m_sst.sdt * 3;
	m_sst.radmrg = getCollisionShape()->getMargin();
	m_sst.updmrg = m_sst.radmrg * (btScalar)0.25;
	/* Forces                */
	addVelocity(m_worldInfo->m_gravity * m_sst.sdt);
	applyForces();
	/* Integrate            */
	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		Node& n = m_nodes[i];
		n.m_q = n.m_x;
		btVector3 deltaV = n.m_f * n.m_im * m_sst.sdt;
		{
			btScalar maxDisplacement = m_worldInfo->m_maxDisplacement;
			btScalar clampDeltaV = maxDisplacement / m_sst.sdt;
			for (int c = 0; c < 3; c++)
			{
				if (deltaV[c] > clampDeltaV)
				{
					deltaV[c] = clampDeltaV;
				}
				if (deltaV[c] < -clampDeltaV)
				{
					deltaV[c] = -clampDeltaV;
				}
			}
		}
		n.m_v += deltaV;
		n.m_x += n.m_v * m_sst.sdt;
		n.m_f = btVector3(0, 0, 0);
	}
	/* Clusters                */
	updateClusters();
	/* Bounds                */
	updateBounds();
	/* Nodes                */
	ATTRIBUTE_ALIGNED16(btDbvtVolume)
	vol;
	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		Node& n = m_nodes[i];
		vol = btDbvtVolume::FromCR(n.m_x, m_sst.radmrg);
		m_ndbvt.update(n.m_leaf,
					   vol,
					   n.m_v * m_sst.velmrg,
					   m_sst.updmrg);
	}
	/* Faces                */
	if (!m_fdbvt.empty())
	{
		for (int i = 0; i < m_faces.size(); ++i)
		{
			Face& f = m_faces[i];
			const btVector3 v = (f.m_n[0]->m_v +
								 f.m_n[1]->m_v +
								 f.m_n[2]->m_v) /
								3;
			vol = VolumeOf(f, m_sst.radmrg);
			m_fdbvt.update(f.m_leaf,
						   vol,
						   v * m_sst.velmrg,
						   m_sst.updmrg);
		}
	}
	/* Pose                    */
	updatePose();
	/* Match                */
	if (m_pose.m_bframe && (m_cfg.kMT > 0))
	{
		const btMatrix3x3 posetrs = m_pose.m_rot;
		for (int i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			Node& n = m_nodes[i];
			if (n.m_im > 0)
			{
				const btVector3 x = posetrs * m_pose.m_pos[i] + m_pose.m_com;
				n.m_x = Lerp(n.m_x, x, m_cfg.kMT);
			}
		}
	}
	/* Clear contacts        */
	m_rcontacts.resize(0);
	m_scontacts.resize(0);
	/* Optimize dbvt's        */
	m_ndbvt.optimizeIncremental(1);
	m_fdbvt.optimizeIncremental(1);
	m_cdbvt.optimizeIncremental(1);
}

//
void btSoftBody::solveConstraints()
{
	/* Apply clusters		*/
	applyClusters(false);
	/* Prepare links		*/

	int i, ni;

	for (i = 0, ni = m_links.size(); i < ni; ++i)
	{
		Link& l = m_links[i];
		l.m_c3 = l.m_n[1]->m_q - l.m_n[0]->m_q;
		l.m_c2 = 1 / (l.m_c3.length2() * l.m_c0);
	}
	/* Prepare anchors		*/
	for (i = 0, ni = m_anchors.size(); i < ni; ++i)
	{
		Anchor& a = m_anchors[i];
		const btVector3 ra = a.m_body->getWorldTransform().getBasis() * a.m_local;
		a.m_c0 = ImpulseMatrix(m_sst.sdt,
							   a.m_node->m_im,
							   a.m_body->getInvMass(),
							   a.m_body->getInvInertiaTensorWorld(),
							   ra);
		a.m_c1 = ra;
		a.m_c2 = m_sst.sdt * a.m_node->m_im;
		a.m_body->activate();
	}
	/* Solve velocities		*/
	if (m_cfg.viterations > 0)
	{
		/* Solve			*/
		for (int isolve = 0; isolve < m_cfg.viterations; ++isolve)
		{
			for (int iseq = 0; iseq < m_cfg.m_vsequence.size(); ++iseq)
			{
				getSolver(m_cfg.m_vsequence[iseq])(this, 1);
			}
		}
		/* Update			*/
		for (i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			Node& n = m_nodes[i];
			n.m_x = n.m_q + n.m_v * m_sst.sdt;
		}
	}
	/* Solve positions		*/
	if (m_cfg.piterations > 0)
	{
		for (int isolve = 0; isolve < m_cfg.piterations; ++isolve)
		{
			const btScalar ti = isolve / (btScalar)m_cfg.piterations;
			for (int iseq = 0; iseq < m_cfg.m_psequence.size(); ++iseq)
			{
				getSolver(m_cfg.m_psequence[iseq])(this, 1, ti);
			}
		}
		const btScalar vc = m_sst.isdt * (1 - m_cfg.kDP);
		for (i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			Node& n = m_nodes[i];
			n.m_v = (n.m_x - n.m_q) * vc;
			n.m_f = btVector3(0, 0, 0);
		}
	}
	/* Solve drift			*/
	if (m_cfg.diterations > 0)
	{
		const btScalar vcf = m_cfg.kVCF * m_sst.isdt;
		for (i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			Node& n = m_nodes[i];
			n.m_q = n.m_x;
		}
		for (int idrift = 0; idrift < m_cfg.diterations; ++idrift)
		{
			for (int iseq = 0; iseq < m_cfg.m_dsequence.size(); ++iseq)
			{
				getSolver(m_cfg.m_dsequence[iseq])(this, 1, 0);
			}
		}
		for (int i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			Node& n = m_nodes[i];
			n.m_v += (n.m_x - n.m_q) * vcf;
		}
	}
	/* Apply clusters		*/
	dampClusters();
	applyClusters(true);
}

//
void btSoftBody::staticSolve(int iterations)
{
	for (int isolve = 0; isolve < iterations; ++isolve)
	{
		for (int iseq = 0; iseq < m_cfg.m_psequence.size(); ++iseq)
		{
			getSolver(m_cfg.m_psequence[iseq])(this, 1, 0);
		}
	}
}

//
void btSoftBody::solveCommonConstraints(btSoftBody** /*bodies*/, int /*count*/, int /*iterations*/)
{
	/// placeholder
}

//
void btSoftBody::solveClusters(const btAlignedObjectArray<btSoftBody*>& bodies)
{
	const int nb = bodies.size();
	int iterations = 0;
	int i;

	for (i = 0; i < nb; ++i)
	{
		iterations = btMax(iterations, bodies[i]->m_cfg.citerations);
	}
	for (i = 0; i < nb; ++i)
	{
		bodies[i]->prepareClusters(iterations);
	}
	for (i = 0; i < iterations; ++i)
	{
		const btScalar sor = 1;
		for (int j = 0; j < nb; ++j)
		{
			bodies[j]->solveClusters(sor);
		}
	}
	for (i = 0; i < nb; ++i)
	{
		bodies[i]->cleanupClusters();
	}
}

//
void btSoftBody::integrateMotion()
{
	/* Update			*/
	updateNormals();
}

//
btSoftBody::RayFromToCaster::RayFromToCaster(const btVector3& rayFrom, const btVector3& rayTo, btScalar mxt)
{
	m_rayFrom = rayFrom;
	m_rayNormalizedDirection = (rayTo - rayFrom);
	m_rayTo = rayTo;
	m_mint = mxt;
	m_face = 0;
	m_tests = 0;
}

//
void btSoftBody::RayFromToCaster::Process(const btDbvtNode* leaf)
{
	btSoftBody::Face& f = *(btSoftBody::Face*)leaf->data;
	const btScalar t = rayFromToTriangle(m_rayFrom, m_rayTo, m_rayNormalizedDirection,
										 f.m_n[0]->m_x,
										 f.m_n[1]->m_x,
										 f.m_n[2]->m_x,
										 m_mint);
	if ((t > 0) && (t < m_mint))
	{
		m_mint = t;
		m_face = &f;
	}
	++m_tests;
}

//
btScalar btSoftBody::RayFromToCaster::rayFromToTriangle(const btVector3& rayFrom,
														const btVector3& rayTo,
														const btVector3& rayNormalizedDirection,
														const btVector3& a,
														const btVector3& b,
														const btVector3& c,
														btScalar maxt)
{
	static const btScalar ceps = -SIMD_EPSILON * 10;
	static const btScalar teps = SIMD_EPSILON * 10;

	const btVector3 n = btCross(b - a, c - a);
	const btScalar d = btDot(a, n);
	const btScalar den = btDot(rayNormalizedDirection, n);
	if (!btFuzzyZero(den))
	{
		const btScalar num = btDot(rayFrom, n) - d;
		const btScalar t = -num / den;
		if ((t > teps) && (t < maxt))
		{
			const btVector3 hit = rayFrom + rayNormalizedDirection * t;
			if ((btDot(n, btCross(a - hit, b - hit)) > ceps) &&
				(btDot(n, btCross(b - hit, c - hit)) > ceps) &&
				(btDot(n, btCross(c - hit, a - hit)) > ceps))
			{
				return (t);
			}
		}
	}
	return (-1);
}

//
void btSoftBody::pointersToIndices()
{
#define PTR2IDX(_p_, _b_) reinterpret_cast<btSoftBody::Node*>((_p_) - (_b_))
	btSoftBody::Node* base = m_nodes.size() ? &m_nodes[0] : 0;
	int i, ni;

	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		if (m_nodes[i].m_leaf)
		{
			m_nodes[i].m_leaf->data = *(void**)&i;
		}
	}
	for (i = 0, ni = m_links.size(); i < ni; ++i)
	{
		m_links[i].m_n[0] = PTR2IDX(m_links[i].m_n[0], base);
		m_links[i].m_n[1] = PTR2IDX(m_links[i].m_n[1], base);
	}
	for (i = 0, ni = m_faces.size(); i < ni; ++i)
	{
		m_faces[i].m_n[0] = PTR2IDX(m_faces[i].m_n[0], base);
		m_faces[i].m_n[1] = PTR2IDX(m_faces[i].m_n[1], base);
		m_faces[i].m_n[2] = PTR2IDX(m_faces[i].m_n[2], base);
		if (m_faces[i].m_leaf)
		{
			m_faces[i].m_leaf->data = *(void**)&i;
		}
	}
	for (i = 0, ni = m_anchors.size(); i < ni; ++i)
	{
		m_anchors[i].m_node = PTR2IDX(m_anchors[i].m_node, base);
	}
	for (i = 0, ni = m_notes.size(); i < ni; ++i)
	{
		for (int j = 0; j < m_notes[i].m_rank; ++j)
		{
			m_notes[i].m_nodes[j] = PTR2IDX(m_notes[i].m_nodes[j], base);
		}
	}
#undef PTR2IDX
}

//
void btSoftBody::indicesToPointers(const int* map)
{
#define IDX2PTR(_p_, _b_) map ? (&(_b_)[map[(((char*)_p_) - (char*)0)]]) : (&(_b_)[(((char*)_p_) - (char*)0)])
	btSoftBody::Node* base = m_nodes.size() ? &m_nodes[0] : 0;
	int i, ni;

	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		if (m_nodes[i].m_leaf)
		{
			m_nodes[i].m_leaf->data = &m_nodes[i];
		}
	}
	for (i = 0, ni = m_links.size(); i < ni; ++i)
	{
		m_links[i].m_n[0] = IDX2PTR(m_links[i].m_n[0], base);
		m_links[i].m_n[1] = IDX2PTR(m_links[i].m_n[1], base);
	}
	for (i = 0, ni = m_faces.size(); i < ni; ++i)
	{
		m_faces[i].m_n[0] = IDX2PTR(m_faces[i].m_n[0], base);
		m_faces[i].m_n[1] = IDX2PTR(m_faces[i].m_n[1], base);
		m_faces[i].m_n[2] = IDX2PTR(m_faces[i].m_n[2], base);
		if (m_faces[i].m_leaf)
		{
			m_faces[i].m_leaf->data = &m_faces[i];
		}
	}
	for (i = 0, ni = m_anchors.size(); i < ni; ++i)
	{
		m_anchors[i].m_node = IDX2PTR(m_anchors[i].m_node, base);
	}
	for (i = 0, ni = m_notes.size(); i < ni; ++i)
	{
		for (int j = 0; j < m_notes[i].m_rank; ++j)
		{
			m_notes[i].m_nodes[j] = IDX2PTR(m_notes[i].m_nodes[j], base);
		}
	}
#undef IDX2PTR
}

//
int btSoftBody::rayTest(const btVector3& rayFrom, const btVector3& rayTo,
						btScalar& mint, eFeature::_& feature, int& index, bool bcountonly) const
{
	int cnt = 0;
	btVector3 dir = rayTo - rayFrom;

	if (bcountonly || m_fdbvt.empty())
	{ /* Full search	*/

		for (int i = 0, ni = m_faces.size(); i < ni; ++i)
		{
			const btSoftBody::Face& f = m_faces[i];

			const btScalar t = RayFromToCaster::rayFromToTriangle(rayFrom, rayTo, dir,
																  f.m_n[0]->m_x,
																  f.m_n[1]->m_x,
																  f.m_n[2]->m_x,
																  mint);
			if (t > 0)
			{
				++cnt;
				if (!bcountonly)
				{
					feature = btSoftBody::eFeature::Face;
					index = i;
					mint = t;
				}
			}
		}
	}
	else
	{ /* Use dbvt	*/
		RayFromToCaster collider(rayFrom, rayTo, mint);

		btDbvt::rayTest(m_fdbvt.m_root, rayFrom, rayTo, collider);
		if (collider.m_face)
		{
			mint = collider.m_mint;
			feature = btSoftBody::eFeature::Face;
			index = (int)(collider.m_face - &m_faces[0]);
			cnt = 1;
		}
	}

	for (int i = 0; i < m_tetras.size(); i++)
	{
		const btSoftBody::Tetra& tet = m_tetras[i];
		int tetfaces[4][3] = {{0, 1, 2}, {0, 1, 3}, {1, 2, 3}, {0, 2, 3}};
		for (int f = 0; f < 4; f++)
		{
			int index0 = tetfaces[f][0];
			int index1 = tetfaces[f][1];
			int index2 = tetfaces[f][2];
			btVector3 v0 = tet.m_n[index0]->m_x;
			btVector3 v1 = tet.m_n[index1]->m_x;
			btVector3 v2 = tet.m_n[index2]->m_x;

			const btScalar t = RayFromToCaster::rayFromToTriangle(rayFrom, rayTo, dir,
																  v0, v1, v2,
																  mint);
			if (t > 0)
			{
				++cnt;
				if (!bcountonly)
				{
					feature = btSoftBody::eFeature::Tetra;
					index = i;
					mint = t;
				}
			}
		}
	}
	return (cnt);
}

int btSoftBody::rayFaceTest(const btVector3& rayFrom, const btVector3& rayTo,
							btScalar& mint, int& index) const
{
	int cnt = 0;
	{ /* Use dbvt	*/
		RayFromToCaster collider(rayFrom, rayTo, mint);

		btDbvt::rayTest(m_fdbvt.m_root, rayFrom, rayTo, collider);
		if (collider.m_face)
		{
			mint = collider.m_mint;
			index = (int)(collider.m_face - &m_faces[0]);
			cnt = 1;
		}
	}
	return (cnt);
}

//
static inline btDbvntNode* copyToDbvnt(const btDbvtNode* n)
{
	if (n == 0)
		return 0;
	btDbvntNode* root = new btDbvntNode(n);
	if (n->isinternal())
	{
		btDbvntNode* c0 = copyToDbvnt(n->childs[0]);
		root->childs[0] = c0;
		btDbvntNode* c1 = copyToDbvnt(n->childs[1]);
		root->childs[1] = c1;
	}
	return root;
}

static inline void calculateNormalCone(btDbvntNode* root)
{
	if (!root)
		return;
	if (root->isleaf())
	{
		const btSoftBody::Face* face = (btSoftBody::Face*)root->data;
		root->normal = face->m_normal;
		root->angle = 0;
	}
	else
	{
		btVector3 n0(0, 0, 0), n1(0, 0, 0);
		btScalar a0 = 0, a1 = 0;
		if (root->childs[0])
		{
			calculateNormalCone(root->childs[0]);
			n0 = root->childs[0]->normal;
			a0 = root->childs[0]->angle;
		}
		if (root->childs[1])
		{
			calculateNormalCone(root->childs[1]);
			n1 = root->childs[1]->normal;
			a1 = root->childs[1]->angle;
		}
		root->normal = (n0 + n1).safeNormalize();
		root->angle = btMax(a0, a1) + btAngle(n0, n1) * 0.5;
	}
}

void btSoftBody::initializeFaceTree()
{
	BT_PROFILE("btSoftBody::initializeFaceTree");
	m_fdbvt.clear();
	// create leaf nodes;
	btAlignedObjectArray<btDbvtNode*> leafNodes;
	leafNodes.resize(m_faces.size());
	for (int i = 0; i < m_faces.size(); ++i)
	{
		Face& f = m_faces[i];
		ATTRIBUTE_ALIGNED16(btDbvtVolume)
		vol = VolumeOf(f, 0);
		btDbvtNode* node = new (btAlignedAlloc(sizeof(btDbvtNode), 16)) btDbvtNode();
		node->parent = NULL;
		node->data = &f;
		node->childs[1] = 0;
		node->volume = vol;
		leafNodes[i] = node;
		f.m_leaf = node;
	}
	btAlignedObjectArray<btAlignedObjectArray<int> > adj;
	adj.resize(m_faces.size());
	// construct the adjacency list for triangles
	for (int i = 0; i < adj.size(); ++i)
	{
		for (int j = i + 1; j < adj.size(); ++j)
		{
			int dup = 0;
			for (int k = 0; k < 3; ++k)
			{
				for (int l = 0; l < 3; ++l)
				{
					if (m_faces[i].m_n[k] == m_faces[j].m_n[l])
					{
						++dup;
						break;
					}
				}
				if (dup == 2)
				{
					adj[i].push_back(j);
					adj[j].push_back(i);
				}
			}
		}
	}
	m_fdbvt.m_root = buildTreeBottomUp(leafNodes, adj);
	if (m_fdbvnt)
		delete m_fdbvnt;
	m_fdbvnt = copyToDbvnt(m_fdbvt.m_root);
	updateFaceTree(false, false);
	rebuildNodeTree();
}

//
void btSoftBody::rebuildNodeTree()
{
	m_ndbvt.clear();
	btAlignedObjectArray<btDbvtNode*> leafNodes;
	leafNodes.resize(m_nodes.size());
	for (int i = 0; i < m_nodes.size(); ++i)
	{
		Node& n = m_nodes[i];
		ATTRIBUTE_ALIGNED16(btDbvtVolume)
		vol = btDbvtVolume::FromCR(n.m_x, 0);
		btDbvtNode* node = new (btAlignedAlloc(sizeof(btDbvtNode), 16)) btDbvtNode();
		node->parent = NULL;
		node->data = &n;
		node->childs[1] = 0;
		node->volume = vol;
		leafNodes[i] = node;
		n.m_leaf = node;
	}
	btAlignedObjectArray<btAlignedObjectArray<int> > adj;
	adj.resize(m_nodes.size());
	btAlignedObjectArray<int> old_id;
	old_id.resize(m_nodes.size());
	for (int i = 0; i < m_nodes.size(); ++i)
		old_id[i] = m_nodes[i].index;
	for (int i = 0; i < m_nodes.size(); ++i)
		m_nodes[i].index = i;
	for (int i = 0; i < m_links.size(); ++i)
	{
		Link& l = m_links[i];
		adj[l.m_n[0]->index].push_back(l.m_n[1]->index);
		adj[l.m_n[1]->index].push_back(l.m_n[0]->index);
	}
	m_ndbvt.m_root = buildTreeBottomUp(leafNodes, adj);
	for (int i = 0; i < m_nodes.size(); ++i)
		m_nodes[i].index = old_id[i];
}

//
btVector3 btSoftBody::evaluateCom() const
{
	btVector3 com(0, 0, 0);
	if (m_pose.m_bframe)
	{
		for (int i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			com += m_nodes[i].m_x * m_pose.m_wgh[i];
		}
	}
	return (com);
}

bool btSoftBody::checkContact(const btCollisionObjectWrapper* colObjWrap,
							  const btVector3& x,
							  btScalar margin,
							  btSoftBody::sCti& cti) const
{
	btVector3 nrm;
	const btCollisionShape* shp = colObjWrap->getCollisionShape();
	//    const btRigidBody *tmpRigid = btRigidBody::upcast(colObjWrap->getCollisionObject());
	//const btTransform &wtr = tmpRigid ? tmpRigid->getWorldTransform() : colObjWrap->getWorldTransform();
	const btTransform& wtr = colObjWrap->getWorldTransform();
	//todo: check which transform is needed here

	btScalar dst =
		m_worldInfo->m_sparsesdf.Evaluate(
			wtr.invXform(x),
			shp,
			nrm,
			margin);
	if (dst < 0)
	{
		cti.m_colObj = colObjWrap->getCollisionObject();
		cti.m_normal = wtr.getBasis() * nrm;
		cti.m_offset = -btDot(cti.m_normal, x - cti.m_normal * dst);
		return (true);
	}
	return (false);
}

//
bool btSoftBody::checkDeformableContact(const btCollisionObjectWrapper* colObjWrap,
										const btVector3& x,
										btScalar margin,
										btSoftBody::sCti& cti, bool predict) const
{
	btVector3 nrm;
	const btCollisionShape* shp = colObjWrap->getCollisionShape();
	const btCollisionObject* tmpCollisionObj = colObjWrap->getCollisionObject();
	// use the position x_{n+1}^* = x_n + dt * v_{n+1}^* where v_{n+1}^* = v_n + dtg for collision detect
	// but resolve contact at x_n
	btTransform wtr = (predict) ? (colObjWrap->m_preTransform != NULL ? tmpCollisionObj->getInterpolationWorldTransform() * (*colObjWrap->m_preTransform) : tmpCollisionObj->getInterpolationWorldTransform())
								: colObjWrap->getWorldTransform();
	btScalar dst =
		m_worldInfo->m_sparsesdf.Evaluate(
			wtr.invXform(x),
			shp,
			nrm,
			margin);

	if (!predict)
	{
		cti.m_colObj = colObjWrap->getCollisionObject();
		cti.m_normal = wtr.getBasis() * nrm;
		cti.m_offset = dst;
	}
	if (dst < 0)
		return true;
	return (false);
}

//
// Compute barycentric coordinates (u, v, w) for
// point p with respect to triangle (a, b, c)
static void getBarycentric(const btVector3& p, btVector3& a, btVector3& b, btVector3& c, btVector3& bary)
{
	btVector3 v0 = b - a, v1 = c - a, v2 = p - a;
	btScalar d00 = v0.dot(v0);
	btScalar d01 = v0.dot(v1);
	btScalar d11 = v1.dot(v1);
	btScalar d20 = v2.dot(v0);
	btScalar d21 = v2.dot(v1);
	btScalar denom = d00 * d11 - d01 * d01;
	bary.setY((d11 * d20 - d01 * d21) / denom);
	bary.setZ((d00 * d21 - d01 * d20) / denom);
	bary.setX(btScalar(1) - bary.getY() - bary.getZ());
}

//
bool btSoftBody::checkDeformableFaceContact(const btCollisionObjectWrapper* colObjWrap,
											Face& f,
											btVector3& contact_point,
											btVector3& bary,
											btScalar margin,
											btSoftBody::sCti& cti, bool predict) const
{
	btVector3 nrm;
	const btCollisionShape* shp = colObjWrap->getCollisionShape();
	const btCollisionObject* tmpCollisionObj = colObjWrap->getCollisionObject();
	// use the position x_{n+1}^* = x_n + dt * v_{n+1}^* where v_{n+1}^* = v_n + dtg for collision detect
	// but resolve contact at x_n
	btTransform wtr = (predict) ? (colObjWrap->m_preTransform != NULL ? tmpCollisionObj->getInterpolationWorldTransform() * (*colObjWrap->m_preTransform) : tmpCollisionObj->getInterpolationWorldTransform())
								: colObjWrap->getWorldTransform();
	btScalar dst;
	btGjkEpaSolver2::sResults results;

//	#define USE_QUADRATURE 1

	// use collision quadrature point
#ifdef USE_QUADRATURE
	{
		dst = SIMD_INFINITY;
		btVector3 local_nrm;
		for (int q = 0; q < m_quads.size(); ++q)
		{
			btVector3 p;
			if (predict)
				p = BaryEval(f.m_n[0]->m_q, f.m_n[1]->m_q, f.m_n[2]->m_q, m_quads[q]);
			else
				p = BaryEval(f.m_n[0]->m_x, f.m_n[1]->m_x, f.m_n[2]->m_x, m_quads[q]);
			btScalar local_dst = m_worldInfo->m_sparsesdf.Evaluate(
				wtr.invXform(p),
				shp,
				local_nrm,
				margin);
			if (local_dst < dst)
			{
				if (local_dst < 0 && predict)
					return true;
				dst = local_dst;
				contact_point = p;
				bary = m_quads[q];
				nrm = local_nrm;
			}
			if (!predict)
			{
				cti.m_colObj = colObjWrap->getCollisionObject();
				cti.m_normal = wtr.getBasis() * nrm;
				cti.m_offset = dst;
			}
		}
		return (dst < 0);
	}
#endif

	// collision detection using x*
	btTransform triangle_transform;
	triangle_transform.setIdentity();
	triangle_transform.setOrigin(f.m_n[0]->m_q);
	btTriangleShape triangle(btVector3(0, 0, 0), f.m_n[1]->m_q - f.m_n[0]->m_q, f.m_n[2]->m_q - f.m_n[0]->m_q);
	btVector3 guess(0, 0, 0);
	const btConvexShape* csh = static_cast<const btConvexShape*>(shp);
	btGjkEpaSolver2::SignedDistance(&triangle, triangle_transform, csh, wtr, guess, results);
	dst = results.distance - 2.0 * csh->getMargin() - margin;  // margin padding so that the distance = the actual distance between face and rigid - margin of rigid - margin of deformable
	if (dst >= 0)
		return false;

	// Use consistent barycenter to recalculate distance.
	if (this->m_cacheBarycenter)
	{
		if (f.m_pcontact[3] != 0)
		{
			for (int i = 0; i < 3; ++i)
				bary[i] = f.m_pcontact[i];
			contact_point = BaryEval(f.m_n[0]->m_x, f.m_n[1]->m_x, f.m_n[2]->m_x, bary);
			const btConvexShape* csh = static_cast<const btConvexShape*>(shp);
			btGjkEpaSolver2::SignedDistance(contact_point, margin, csh, wtr, results);
			cti.m_colObj = colObjWrap->getCollisionObject();
			dst = results.distance;
			cti.m_normal = results.normal;
			cti.m_offset = dst;

			//point-convex CD
			wtr = colObjWrap->getWorldTransform();
			btTriangleShape triangle2(btVector3(0, 0, 0), f.m_n[1]->m_x - f.m_n[0]->m_x, f.m_n[2]->m_x - f.m_n[0]->m_x);
			triangle_transform.setOrigin(f.m_n[0]->m_x);
			btGjkEpaSolver2::SignedDistance(&triangle2, triangle_transform, csh, wtr, guess, results);

			dst = results.distance - csh->getMargin() - margin;
			return true;
		}
	}

	// Use triangle-convex CD.
	wtr = colObjWrap->getWorldTransform();
	btTriangleShape triangle2(btVector3(0, 0, 0), f.m_n[1]->m_x - f.m_n[0]->m_x, f.m_n[2]->m_x - f.m_n[0]->m_x);
	triangle_transform.setOrigin(f.m_n[0]->m_x);
	btGjkEpaSolver2::SignedDistance(&triangle2, triangle_transform, csh, wtr, guess, results);
	contact_point = results.witnesses[0];
	getBarycentric(contact_point, f.m_n[0]->m_x, f.m_n[1]->m_x, f.m_n[2]->m_x, bary);

	for (int i = 0; i < 3; ++i)
		f.m_pcontact[i] = bary[i];

	dst = results.distance - csh->getMargin() - margin;
	cti.m_colObj = colObjWrap->getCollisionObject();
	cti.m_normal = results.normal;
	cti.m_offset = dst;
	return true;
}

void btSoftBody::updateNormals()
{
	const btVector3 zv(0, 0, 0);
	int i, ni;

	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		m_nodes[i].m_n = zv;
	}
	for (i = 0, ni = m_faces.size(); i < ni; ++i)
	{
		btSoftBody::Face& f = m_faces[i];
		const btVector3 n = btCross(f.m_n[1]->m_x - f.m_n[0]->m_x,
									f.m_n[2]->m_x - f.m_n[0]->m_x);
		f.m_normal = n;
		f.m_normal.safeNormalize();
		f.m_n[0]->m_n += n;
		f.m_n[1]->m_n += n;
		f.m_n[2]->m_n += n;
	}
	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		btScalar len = m_nodes[i].m_n.length();
		if (len > SIMD_EPSILON)
			m_nodes[i].m_n /= len;
	}
}

//
void btSoftBody::updateBounds()
{
	/*if( m_acceleratedSoftBody )
	{
		// If we have an accelerated softbody we need to obtain the bounds correctly
		// For now (slightly hackily) just have a very large AABB
		// TODO: Write get bounds kernel
		// If that is updating in place, atomic collisions might be low (when the cloth isn't perfectly aligned to an axis) and we could
		// probably do a test and exchange reasonably efficiently.

		m_bounds[0] = btVector3(-1000, -1000, -1000);
		m_bounds[1] = btVector3(1000, 1000, 1000);

	} else {*/
	//    if (m_ndbvt.m_root)
	//    {
	//        const btVector3& mins = m_ndbvt.m_root->volume.Mins();
	//        const btVector3& maxs = m_ndbvt.m_root->volume.Maxs();
	//        const btScalar csm = getCollisionShape()->getMargin();
	//        const btVector3 mrg = btVector3(csm,
	//                                        csm,
	//                                        csm) *
	//                              1;  // ??? to investigate...
	//        m_bounds[0] = mins - mrg;
	//        m_bounds[1] = maxs + mrg;
	//        if (0 != getBroadphaseHandle())
	//        {
	//            m_worldInfo->m_broadphase->setAabb(getBroadphaseHandle(),
	//                                               m_bounds[0],
	//                                               m_bounds[1],
	//                                               m_worldInfo->m_dispatcher);
	//        }
	//    }
	//    else
	//    {
	//        m_bounds[0] =
	//            m_bounds[1] = btVector3(0, 0, 0);
	//    }
	if (m_nodes.size())
	{
		btVector3 mins = m_nodes[0].m_x;
		btVector3 maxs = m_nodes[0].m_x;
		for (int i = 1; i < m_nodes.size(); ++i)
		{
			for (int d = 0; d < 3; ++d)
			{
				if (m_nodes[i].m_x[d] > maxs[d])
					maxs[d] = m_nodes[i].m_x[d];
				if (m_nodes[i].m_x[d] < mins[d])
					mins[d] = m_nodes[i].m_x[d];
			}
		}
		const btScalar csm = getCollisionShape()->getMargin();
		const btVector3 mrg = btVector3(csm,
										csm,
										csm);
		m_bounds[0] = mins - mrg;
		m_bounds[1] = maxs + mrg;
		if (0 != getBroadphaseHandle())
		{
			m_worldInfo->m_broadphase->setAabb(getBroadphaseHandle(),
											   m_bounds[0],
											   m_bounds[1],
											   m_worldInfo->m_dispatcher);
		}
	}
	else
	{
		m_bounds[0] =
			m_bounds[1] = btVector3(0, 0, 0);
	}
}

//
void btSoftBody::updatePose()
{
	if (m_pose.m_bframe)
	{
		btSoftBody::Pose& pose = m_pose;
		const btVector3 com = evaluateCom();
		/* Com			*/
		pose.m_com = com;
		/* Rotation		*/
		btMatrix3x3 Apq;
		const btScalar eps = SIMD_EPSILON;
		Apq[0] = Apq[1] = Apq[2] = btVector3(0, 0, 0);
		Apq[0].setX(eps);
		Apq[1].setY(eps * 2);
		Apq[2].setZ(eps * 3);
		for (int i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			const btVector3 a = pose.m_wgh[i] * (m_nodes[i].m_x - com);
			const btVector3& b = pose.m_pos[i];
			Apq[0] += a.x() * b;
			Apq[1] += a.y() * b;
			Apq[2] += a.z() * b;
		}
		btMatrix3x3 r, s;
		PolarDecompose(Apq, r, s);
		pose.m_rot = r;
		pose.m_scl = pose.m_aqq * r.transpose() * Apq;
		if (m_cfg.maxvolume > 1)
		{
			const btScalar idet = Clamp<btScalar>(1 / pose.m_scl.determinant(),
												  1, m_cfg.maxvolume);
			pose.m_scl = Mul(pose.m_scl, idet);
		}
	}
}

//
void btSoftBody::updateArea(bool averageArea)
{
	int i, ni;

	/* Face area		*/
	for (i = 0, ni = m_faces.size(); i < ni; ++i)
	{
		Face& f = m_faces[i];
		f.m_ra = AreaOf(f.m_n[0]->m_x, f.m_n[1]->m_x, f.m_n[2]->m_x);
	}

	/* Node area		*/

	if (averageArea)
	{
		btAlignedObjectArray<int> counts;
		counts.resize(m_nodes.size(), 0);
		for (i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			m_nodes[i].m_area = 0;
		}
		for (i = 0, ni = m_faces.size(); i < ni; ++i)
		{
			btSoftBody::Face& f = m_faces[i];
			for (int j = 0; j < 3; ++j)
			{
				const int index = (int)(f.m_n[j] - &m_nodes[0]);
				counts[index]++;
				f.m_n[j]->m_area += btFabs(f.m_ra);
			}
		}
		for (i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			if (counts[i] > 0)
				m_nodes[i].m_area /= (btScalar)counts[i];
			else
				m_nodes[i].m_area = 0;
		}
	}
	else
	{
		// initialize node area as zero
		for (i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			m_nodes[i].m_area = 0;
		}

		for (i = 0, ni = m_faces.size(); i < ni; ++i)
		{
			btSoftBody::Face& f = m_faces[i];

			for (int j = 0; j < 3; ++j)
			{
				f.m_n[j]->m_area += f.m_ra;
			}
		}

		for (i = 0, ni = m_nodes.size(); i < ni; ++i)
		{
			m_nodes[i].m_area *= 0.3333333f;
		}
	}
}

void btSoftBody::updateLinkConstants()
{
	int i, ni;

	/* Links		*/
	for (i = 0, ni = m_links.size(); i < ni; ++i)
	{
		Link& l = m_links[i];
		Material& m = *l.m_material;
		l.m_c0 = (l.m_n[0]->m_im + l.m_n[1]->m_im) / m.m_kLST;
	}
}

void btSoftBody::updateConstants()
{
	resetLinkRestLengths();
	updateLinkConstants();
	updateArea();
}

//
void btSoftBody::initializeClusters()
{
	int i;

	for (i = 0; i < m_clusters.size(); ++i)
	{
		Cluster& c = *m_clusters[i];
		c.m_imass = 0;
		c.m_masses.resize(c.m_nodes.size());
		for (int j = 0; j < c.m_nodes.size(); ++j)
		{
			if (c.m_nodes[j]->m_im == 0)
			{
				c.m_containsAnchor = true;
				c.m_masses[j] = BT_LARGE_FLOAT;
			}
			else
			{
				c.m_masses[j] = btScalar(1.) / c.m_nodes[j]->m_im;
			}
			c.m_imass += c.m_masses[j];
		}
		c.m_imass = btScalar(1.) / c.m_imass;
		c.m_com = btSoftBody::clusterCom(&c);
		c.m_lv = btVector3(0, 0, 0);
		c.m_av = btVector3(0, 0, 0);
		c.m_leaf = 0;
		/* Inertia	*/
		btMatrix3x3& ii = c.m_locii;
		ii[0] = ii[1] = ii[2] = btVector3(0, 0, 0);
		{
			int i, ni;

			for (i = 0, ni = c.m_nodes.size(); i < ni; ++i)
			{
				const btVector3 k = c.m_nodes[i]->m_x - c.m_com;
				const btVector3 q = k * k;
				const btScalar m = c.m_masses[i];
				ii[0][0] += m * (q[1] + q[2]);
				ii[1][1] += m * (q[0] + q[2]);
				ii[2][2] += m * (q[0] + q[1]);
				ii[0][1] -= m * k[0] * k[1];
				ii[0][2] -= m * k[0] * k[2];
				ii[1][2] -= m * k[1] * k[2];
			}
		}
		ii[1][0] = ii[0][1];
		ii[2][0] = ii[0][2];
		ii[2][1] = ii[1][2];

		ii = ii.inverse();

		/* Frame	*/
		c.m_framexform.setIdentity();
		c.m_framexform.setOrigin(c.m_com);
		c.m_framerefs.resize(c.m_nodes.size());
		{
			int i;
			for (i = 0; i < c.m_framerefs.size(); ++i)
			{
				c.m_framerefs[i] = c.m_nodes[i]->m_x - c.m_com;
			}
		}
	}
}

//
void btSoftBody::updateClusters()
{
	BT_PROFILE("UpdateClusters");
	int i;

	for (i = 0; i < m_clusters.size(); ++i)
	{
		btSoftBody::Cluster& c = *m_clusters[i];
		const int n = c.m_nodes.size();
		//const btScalar			invn=1/(btScalar)n;
		if (n)
		{
			/* Frame				*/
			const btScalar eps = btScalar(0.0001);
			btMatrix3x3 m, r, s;
			m[0] = m[1] = m[2] = btVector3(0, 0, 0);
			m[0][0] = eps * 1;
			m[1][1] = eps * 2;
			m[2][2] = eps * 3;
			c.m_com = clusterCom(&c);
			for (int i = 0; i < c.m_nodes.size(); ++i)
			{
				const btVector3 a = c.m_nodes[i]->m_x - c.m_com;
				const btVector3& b = c.m_framerefs[i];
				m[0] += a[0] * b;
				m[1] += a[1] * b;
				m[2] += a[2] * b;
			}
			PolarDecompose(m, r, s);
			c.m_framexform.setOrigin(c.m_com);
			c.m_framexform.setBasis(r);
			/* Inertia			*/
#if 1 /* Constant	*/
			c.m_invwi = c.m_framexform.getBasis() * c.m_locii * c.m_framexform.getBasis().transpose();
#else
#if 0 /* Sphere	*/ 
			const btScalar	rk=(2*c.m_extents.length2())/(5*c.m_imass);
			const btVector3	inertia(rk,rk,rk);
			const btVector3	iin(btFabs(inertia[0])>SIMD_EPSILON?1/inertia[0]:0,
				btFabs(inertia[1])>SIMD_EPSILON?1/inertia[1]:0,
				btFabs(inertia[2])>SIMD_EPSILON?1/inertia[2]:0);

			c.m_invwi=c.m_xform.getBasis().scaled(iin)*c.m_xform.getBasis().transpose();
#else /* Actual	*/
			c.m_invwi[0] = c.m_invwi[1] = c.m_invwi[2] = btVector3(0, 0, 0);
			for (int i = 0; i < n; ++i)
			{
				const btVector3 k = c.m_nodes[i]->m_x - c.m_com;
				const btVector3 q = k * k;
				const btScalar m = 1 / c.m_nodes[i]->m_im;
				c.m_invwi[0][0] += m * (q[1] + q[2]);
				c.m_invwi[1][1] += m * (q[0] + q[2]);
				c.m_invwi[2][2] += m * (q[0] + q[1]);
				c.m_invwi[0][1] -= m * k[0] * k[1];
				c.m_invwi[0][2] -= m * k[0] * k[2];
				c.m_invwi[1][2] -= m * k[1] * k[2];
			}
			c.m_invwi[1][0] = c.m_invwi[0][1];
			c.m_invwi[2][0] = c.m_invwi[0][2];
			c.m_invwi[2][1] = c.m_invwi[1][2];
			c.m_invwi = c.m_invwi.inverse();
#endif
#endif
			/* Velocities			*/
			c.m_lv = btVector3(0, 0, 0);
			c.m_av = btVector3(0, 0, 0);
			{
				int i;

				for (i = 0; i < n; ++i)
				{
					const btVector3 v = c.m_nodes[i]->m_v * c.m_masses[i];
					c.m_lv += v;
					c.m_av += btCross(c.m_nodes[i]->m_x - c.m_com, v);
				}
			}
			c.m_lv = c.m_imass * c.m_lv * (1 - c.m_ldamping);
			c.m_av = c.m_invwi * c.m_av * (1 - c.m_adamping);
			c.m_vimpulses[0] =
				c.m_vimpulses[1] = btVector3(0, 0, 0);
			c.m_dimpulses[0] =
				c.m_dimpulses[1] = btVector3(0, 0, 0);
			c.m_nvimpulses = 0;
			c.m_ndimpulses = 0;
			/* Matching				*/
			if (c.m_matching > 0)
			{
				for (int j = 0; j < c.m_nodes.size(); ++j)
				{
					Node& n = *c.m_nodes[j];
					const btVector3 x = c.m_framexform * c.m_framerefs[j];
					n.m_x = Lerp(n.m_x, x, c.m_matching);
				}
			}
			/* Dbvt					*/
			if (c.m_collide)
			{
				btVector3 mi = c.m_nodes[0]->m_x;
				btVector3 mx = mi;
				for (int j = 1; j < n; ++j)
				{
					mi.setMin(c.m_nodes[j]->m_x);
					mx.setMax(c.m_nodes[j]->m_x);
				}
				ATTRIBUTE_ALIGNED16(btDbvtVolume)
				bounds = btDbvtVolume::FromMM(mi, mx);
				if (c.m_leaf)
					m_cdbvt.update(c.m_leaf, bounds, c.m_lv * m_sst.sdt * 3, m_sst.radmrg);
				else
					c.m_leaf = m_cdbvt.insert(bounds, &c);
			}
		}
	}
}

//
void btSoftBody::cleanupClusters()
{
	for (int i = 0; i < m_joints.size(); ++i)
	{
		m_joints[i]->Terminate(m_sst.sdt);
		if (m_joints[i]->m_delete)
		{
			btAlignedFree(m_joints[i]);
			m_joints.remove(m_joints[i--]);
		}
	}
}

//
void btSoftBody::prepareClusters(int iterations)
{
	for (int i = 0; i < m_joints.size(); ++i)
	{
		m_joints[i]->Prepare(m_sst.sdt, iterations);
	}
}

//
void btSoftBody::solveClusters(btScalar sor)
{
	for (int i = 0, ni = m_joints.size(); i < ni; ++i)
	{
		m_joints[i]->Solve(m_sst.sdt, sor);
	}
}

//
void btSoftBody::applyClusters(bool drift)
{
	BT_PROFILE("ApplyClusters");
	//	const btScalar					f0=m_sst.sdt;
	//const btScalar					f1=f0/2;
	btAlignedObjectArray<btVector3> deltas;
	btAlignedObjectArray<btScalar> weights;
	deltas.resize(m_nodes.size(), btVector3(0, 0, 0));
	weights.resize(m_nodes.size(), 0);
	int i;

	if (drift)
	{
		for (i = 0; i < m_clusters.size(); ++i)
		{
			Cluster& c = *m_clusters[i];
			if (c.m_ndimpulses)
			{
				c.m_dimpulses[0] /= (btScalar)c.m_ndimpulses;
				c.m_dimpulses[1] /= (btScalar)c.m_ndimpulses;
			}
		}
	}

	for (i = 0; i < m_clusters.size(); ++i)
	{
		Cluster& c = *m_clusters[i];
		if (0 < (drift ? c.m_ndimpulses : c.m_nvimpulses))
		{
			const btVector3 v = (drift ? c.m_dimpulses[0] : c.m_vimpulses[0]) * m_sst.sdt;
			const btVector3 w = (drift ? c.m_dimpulses[1] : c.m_vimpulses[1]) * m_sst.sdt;
			for (int j = 0; j < c.m_nodes.size(); ++j)
			{
				const int idx = int(c.m_nodes[j] - &m_nodes[0]);
				const btVector3& x = c.m_nodes[j]->m_x;
				const btScalar q = c.m_masses[j];
				deltas[idx] += (v + btCross(w, x - c.m_com)) * q;
				weights[idx] += q;
			}
		}
	}
	for (i = 0; i < deltas.size(); ++i)
	{
		if (weights[i] > 0)
		{
			m_nodes[i].m_x += deltas[i] / weights[i];
		}
	}
}

//
void btSoftBody::dampClusters()
{
	int i;

	for (i = 0; i < m_clusters.size(); ++i)
	{
		Cluster& c = *m_clusters[i];
		if (c.m_ndamping > 0)
		{
			for (int j = 0; j < c.m_nodes.size(); ++j)
			{
				Node& n = *c.m_nodes[j];
				if (n.m_im > 0)
				{
					const btVector3 vx = c.m_lv + btCross(c.m_av, c.m_nodes[j]->m_q - c.m_com);
					if (vx.length2() <= n.m_v.length2())
					{
						n.m_v += c.m_ndamping * (vx - n.m_v);
					}
				}
			}
		}
	}
}

void btSoftBody::setSpringStiffness(btScalar k)
{
	for (int i = 0; i < m_links.size(); ++i)
	{
		m_links[i].Feature::m_material->m_kLST = k;
	}
	m_repulsionStiffness = k;
}

void btSoftBody::setGravityFactor(btScalar gravFactor)
{
	m_gravityFactor = gravFactor;
}

void btSoftBody::setCacheBarycenter(bool cacheBarycenter)
{
	m_cacheBarycenter = cacheBarycenter;
}

void btSoftBody::initializeDmInverse()
{
	btScalar unit_simplex_measure = 1. / 6.;

	for (int i = 0; i < m_tetras.size(); ++i)
	{
		Tetra& t = m_tetras[i];
		btVector3 c1 = t.m_n[1]->m_x - t.m_n[0]->m_x;
		btVector3 c2 = t.m_n[2]->m_x - t.m_n[0]->m_x;
		btVector3 c3 = t.m_n[3]->m_x - t.m_n[0]->m_x;
		btMatrix3x3 Dm(c1.getX(), c2.getX(), c3.getX(),
					   c1.getY(), c2.getY(), c3.getY(),
					   c1.getZ(), c2.getZ(), c3.getZ());
		t.m_element_measure = Dm.determinant() * unit_simplex_measure;
		t.m_Dm_inverse = Dm.inverse();

		// calculate the first three columns of P^{-1}
		btVector3 a = t.m_n[0]->m_x;
		btVector3 b = t.m_n[1]->m_x;
		btVector3 c = t.m_n[2]->m_x;
		btVector3 d = t.m_n[3]->m_x;

		btScalar det = 1 / (a[0] * b[1] * c[2] - a[0] * b[1] * d[2] - a[0] * b[2] * c[1] + a[0] * b[2] * d[1] + a[0] * c[1] * d[2] - a[0] * c[2] * d[1] + a[1] * (-b[0] * c[2] + b[0] * d[2] + b[2] * c[0] - b[2] * d[0] - c[0] * d[2] + c[2] * d[0]) + a[2] * (b[0] * c[1] - b[0] * d[1] + b[1] * (d[0] - c[0]) + c[0] * d[1] - c[1] * d[0]) - b[0] * c[1] * d[2] + b[0] * c[2] * d[1] + b[1] * c[0] * d[2] - b[1] * c[2] * d[0] - b[2] * c[0] * d[1] + b[2] * c[1] * d[0]);

		btScalar P11 = -b[2] * c[1] + d[2] * c[1] + b[1] * c[2] + b[2] * d[1] - c[2] * d[1] - b[1] * d[2];
		btScalar P12 = b[2] * c[0] - d[2] * c[0] - b[0] * c[2] - b[2] * d[0] + c[2] * d[0] + b[0] * d[2];
		btScalar P13 = -b[1] * c[0] + d[1] * c[0] + b[0] * c[1] + b[1] * d[0] - c[1] * d[0] - b[0] * d[1];
		btScalar P21 = a[2] * c[1] - d[2] * c[1] - a[1] * c[2] - a[2] * d[1] + c[2] * d[1] + a[1] * d[2];
		btScalar P22 = -a[2] * c[0] + d[2] * c[0] + a[0] * c[2] + a[2] * d[0] - c[2] * d[0] - a[0] * d[2];
		btScalar P23 = a[1] * c[0] - d[1] * c[0] - a[0] * c[1] - a[1] * d[0] + c[1] * d[0] + a[0] * d[1];
		btScalar P31 = -a[2] * b[1] + d[2] * b[1] + a[1] * b[2] + a[2] * d[1] - b[2] * d[1] - a[1] * d[2];
		btScalar P32 = a[2] * b[0] - d[2] * b[0] - a[0] * b[2] - a[2] * d[0] + b[2] * d[0] + a[0] * d[2];
		btScalar P33 = -a[1] * b[0] + d[1] * b[0] + a[0] * b[1] + a[1] * d[0] - b[1] * d[0] - a[0] * d[1];
		btScalar P41 = a[2] * b[1] - c[2] * b[1] - a[1] * b[2] - a[2] * c[1] + b[2] * c[1] + a[1] * c[2];
		btScalar P42 = -a[2] * b[0] + c[2] * b[0] + a[0] * b[2] + a[2] * c[0] - b[2] * c[0] - a[0] * c[2];
		btScalar P43 = a[1] * b[0] - c[1] * b[0] - a[0] * b[1] - a[1] * c[0] + b[1] * c[0] + a[0] * c[1];

		btVector4 p1(P11 * det, P21 * det, P31 * det, P41 * det);
		btVector4 p2(P12 * det, P22 * det, P32 * det, P42 * det);
		btVector4 p3(P13 * det, P23 * det, P33 * det, P43 * det);

		t.m_P_inv[0] = p1;
		t.m_P_inv[1] = p2;
		t.m_P_inv[2] = p3;
	}
}

static btScalar Dot4(const btVector4& a, const btVector4& b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

void btSoftBody::updateDeformation()
{
	btQuaternion q;
	for (int i = 0; i < m_tetras.size(); ++i)
	{
		btSoftBody::Tetra& t = m_tetras[i];
		btVector3 c1 = t.m_n[1]->m_q - t.m_n[0]->m_q;
		btVector3 c2 = t.m_n[2]->m_q - t.m_n[0]->m_q;
		btVector3 c3 = t.m_n[3]->m_q - t.m_n[0]->m_q;
		btMatrix3x3 Ds(c1.getX(), c2.getX(), c3.getX(),
					   c1.getY(), c2.getY(), c3.getY(),
					   c1.getZ(), c2.getZ(), c3.getZ());
		t.m_F = Ds * t.m_Dm_inverse;

		btSoftBody::TetraScratch& s = m_tetraScratches[i];
		s.m_F = t.m_F;
		s.m_J = t.m_F.determinant();
		btMatrix3x3 C = t.m_F.transpose() * t.m_F;
		s.m_trace = C[0].getX() + C[1].getY() + C[2].getZ();
		s.m_cofF = t.m_F.adjoint().transpose();

		btVector3 a = t.m_n[0]->m_q;
		btVector3 b = t.m_n[1]->m_q;
		btVector3 c = t.m_n[2]->m_q;
		btVector3 d = t.m_n[3]->m_q;
		btVector4 q1(a[0], b[0], c[0], d[0]);
		btVector4 q2(a[1], b[1], c[1], d[1]);
		btVector4 q3(a[2], b[2], c[2], d[2]);
		btMatrix3x3 B(Dot4(q1, t.m_P_inv[0]), Dot4(q1, t.m_P_inv[1]), Dot4(q1, t.m_P_inv[2]),
					  Dot4(q2, t.m_P_inv[0]), Dot4(q2, t.m_P_inv[1]), Dot4(q2, t.m_P_inv[2]),
					  Dot4(q3, t.m_P_inv[0]), Dot4(q3, t.m_P_inv[1]), Dot4(q3, t.m_P_inv[2]));
		q.setRotation(btVector3(0, 0, 1), 0);
		B.extractRotation(q, 0.01);  // precision of the rotation is not very important for visual correctness.
		btMatrix3x3 Q(q);
		s.m_corotation = Q;
	}
}

void btSoftBody::advanceDeformation()
{
	updateDeformation();
	for (int i = 0; i < m_tetras.size(); ++i)
	{
		m_tetraScratchesTn[i] = m_tetraScratches[i];
	}
}
//
void btSoftBody::Joint::Prepare(btScalar dt, int)
{
	m_bodies[0].activate();
	m_bodies[1].activate();
}

//
void btSoftBody::LJoint::Prepare(btScalar dt, int iterations)
{
	static const btScalar maxdrift = 4;
	Joint::Prepare(dt, iterations);
	m_rpos[0] = m_bodies[0].xform() * m_refs[0];
	m_rpos[1] = m_bodies[1].xform() * m_refs[1];
	m_drift = Clamp(m_rpos[0] - m_rpos[1], maxdrift) * m_erp / dt;
	m_rpos[0] -= m_bodies[0].xform().getOrigin();
	m_rpos[1] -= m_bodies[1].xform().getOrigin();
	m_massmatrix = ImpulseMatrix(m_bodies[0].invMass(), m_bodies[0].invWorldInertia(), m_rpos[0],
								 m_bodies[1].invMass(), m_bodies[1].invWorldInertia(), m_rpos[1]);
	if (m_split > 0)
	{
		m_sdrift = m_massmatrix * (m_drift * m_split);
		m_drift *= 1 - m_split;
	}
	m_drift /= (btScalar)iterations;
}

//
void btSoftBody::LJoint::Solve(btScalar dt, btScalar sor)
{
	const btVector3 va = m_bodies[0].velocity(m_rpos[0]);
	const btVector3 vb = m_bodies[1].velocity(m_rpos[1]);
	const btVector3 vr = va - vb;
	btSoftBody::Impulse impulse;
	impulse.m_asVelocity = 1;
	impulse.m_velocity = m_massmatrix * (m_drift + vr * m_cfm) * sor;
	m_bodies[0].applyImpulse(-impulse, m_rpos[0]);
	m_bodies[1].applyImpulse(impulse, m_rpos[1]);
}

//
void btSoftBody::LJoint::Terminate(btScalar dt)
{
	if (m_split > 0)
	{
		m_bodies[0].applyDImpulse(-m_sdrift, m_rpos[0]);
		m_bodies[1].applyDImpulse(m_sdrift, m_rpos[1]);
	}
}

//
void btSoftBody::AJoint::Prepare(btScalar dt, int iterations)
{
	static const btScalar maxdrift = SIMD_PI / 16;
	m_icontrol->Prepare(this);
	Joint::Prepare(dt, iterations);
	m_axis[0] = m_bodies[0].xform().getBasis() * m_refs[0];
	m_axis[1] = m_bodies[1].xform().getBasis() * m_refs[1];
	m_drift = NormalizeAny(btCross(m_axis[1], m_axis[0]));
	m_drift *= btMin(maxdrift, btAcos(Clamp<btScalar>(btDot(m_axis[0], m_axis[1]), -1, +1)));
	m_drift *= m_erp / dt;
	m_massmatrix = AngularImpulseMatrix(m_bodies[0].invWorldInertia(), m_bodies[1].invWorldInertia());
	if (m_split > 0)
	{
		m_sdrift = m_massmatrix * (m_drift * m_split);
		m_drift *= 1 - m_split;
	}
	m_drift /= (btScalar)iterations;
}

//
void btSoftBody::AJoint::Solve(btScalar dt, btScalar sor)
{
	const btVector3 va = m_bodies[0].angularVelocity();
	const btVector3 vb = m_bodies[1].angularVelocity();
	const btVector3 vr = va - vb;
	const btScalar sp = btDot(vr, m_axis[0]);
	const btVector3 vc = vr - m_axis[0] * m_icontrol->Speed(this, sp);
	btSoftBody::Impulse impulse;
	impulse.m_asVelocity = 1;
	impulse.m_velocity = m_massmatrix * (m_drift + vc * m_cfm) * sor;
	m_bodies[0].applyAImpulse(-impulse);
	m_bodies[1].applyAImpulse(impulse);
}

//
void btSoftBody::AJoint::Terminate(btScalar dt)
{
	if (m_split > 0)
	{
		m_bodies[0].applyDAImpulse(-m_sdrift);
		m_bodies[1].applyDAImpulse(m_sdrift);
	}
}

//
void btSoftBody::CJoint::Prepare(btScalar dt, int iterations)
{
	Joint::Prepare(dt, iterations);
	const bool dodrift = (m_life == 0);
	m_delete = (++m_life) > m_maxlife;
	if (dodrift)
	{
		m_drift = m_drift * m_erp / dt;
		if (m_split > 0)
		{
			m_sdrift = m_massmatrix * (m_drift * m_split);
			m_drift *= 1 - m_split;
		}
		m_drift /= (btScalar)iterations;
	}
	else
	{
		m_drift = m_sdrift = btVector3(0, 0, 0);
	}
}

//
void btSoftBody::CJoint::Solve(btScalar dt, btScalar sor)
{
	const btVector3 va = m_bodies[0].velocity(m_rpos[0]);
	const btVector3 vb = m_bodies[1].velocity(m_rpos[1]);
	const btVector3 vrel = va - vb;
	const btScalar rvac = btDot(vrel, m_normal);
	btSoftBody::Impulse impulse;
	impulse.m_asVelocity = 1;
	impulse.m_velocity = m_drift;
	if (rvac < 0)
	{
		const btVector3 iv = m_normal * rvac;
		const btVector3 fv = vrel - iv;
		impulse.m_velocity += iv + fv * m_friction;
	}
	impulse.m_velocity = m_massmatrix * impulse.m_velocity * sor;

	if (m_bodies[0].m_soft == m_bodies[1].m_soft)
	{
		if ((impulse.m_velocity.getX() == impulse.m_velocity.getX()) && (impulse.m_velocity.getY() == impulse.m_velocity.getY()) &&
			(impulse.m_velocity.getZ() == impulse.m_velocity.getZ()))
		{
			if (impulse.m_asVelocity)
			{
				if (impulse.m_velocity.length() < m_bodies[0].m_soft->m_maxSelfCollisionImpulse)
				{
				}
				else
				{
					m_bodies[0].applyImpulse(-impulse * m_bodies[0].m_soft->m_selfCollisionImpulseFactor, m_rpos[0]);
					m_bodies[1].applyImpulse(impulse * m_bodies[0].m_soft->m_selfCollisionImpulseFactor, m_rpos[1]);
				}
			}
		}
	}
	else
	{
		m_bodies[0].applyImpulse(-impulse, m_rpos[0]);
		m_bodies[1].applyImpulse(impulse, m_rpos[1]);
	}
}

//
void btSoftBody::CJoint::Terminate(btScalar dt)
{
	if (m_split > 0)
	{
		m_bodies[0].applyDImpulse(-m_sdrift, m_rpos[0]);
		m_bodies[1].applyDImpulse(m_sdrift, m_rpos[1]);
	}
}

//
void btSoftBody::applyForces()
{
	BT_PROFILE("SoftBody applyForces");
	//	const btScalar					dt =			m_sst.sdt;
	const btScalar kLF = m_cfg.kLF;
	const btScalar kDG = m_cfg.kDG;
	const btScalar kPR = m_cfg.kPR;
	const btScalar kVC = m_cfg.kVC;
	const bool as_lift = kLF > 0;
	const bool as_drag = kDG > 0;
	const bool as_pressure = kPR != 0;
	const bool as_volume = kVC > 0;
	const bool as_aero = as_lift ||
						 as_drag;
	//const bool						as_vaero =		as_aero	&&
	//												(m_cfg.aeromodel < btSoftBody::eAeroModel::F_TwoSided);
	//const bool						as_faero =		as_aero	&&
	//												(m_cfg.aeromodel >= btSoftBody::eAeroModel::F_TwoSided);
	const bool use_medium = as_aero;
	const bool use_volume = as_pressure ||
							as_volume;
	btScalar volume = 0;
	btScalar ivolumetp = 0;
	btScalar dvolumetv = 0;
	btSoftBody::sMedium medium;
	if (use_volume)
	{
		volume = getVolume();
		ivolumetp = 1 / btFabs(volume) * kPR;
		dvolumetv = (m_pose.m_volume - volume) * kVC;
	}
	/* Per vertex forces			*/
	int i, ni;

	for (i = 0, ni = m_nodes.size(); i < ni; ++i)
	{
		btSoftBody::Node& n = m_nodes[i];
		if (n.m_im > 0)
		{
			if (use_medium)
			{
				/* Aerodynamics			*/
				addAeroForceToNode(m_windVelocity, i);
			}
			/* Pressure				*/
			if (as_pressure)
			{
				n.m_f += n.m_n * (n.m_area * ivolumetp);
			}
			/* Volume				*/
			if (as_volume)
			{
				n.m_f += n.m_n * (n.m_area * dvolumetv);
			}
		}
	}

	/* Per face forces				*/
	for (i = 0, ni = m_faces.size(); i < ni; ++i)
	{
		//	btSoftBody::Face&	f=m_faces[i];

		/* Aerodynamics			*/
		addAeroForceToFace(m_windVelocity, i);
	}
}

//
void btSoftBody::setMaxStress(btScalar maxStress)
{
	m_cfg.m_maxStress = maxStress;
}

//
void btSoftBody::interpolateRenderMesh()
{
	if (m_z.size() > 0)
	{
		for (int i = 0; i < m_renderNodes.size(); ++i)
		{
			const Node* p0 = m_renderNodesParents[i][0];
			const Node* p1 = m_renderNodesParents[i][1];
			const Node* p2 = m_renderNodesParents[i][2];
			btVector3 normal = btCross(p1->m_x - p0->m_x, p2->m_x - p0->m_x);
			btVector3 unit_normal = normal.normalized();
			RenderNode& n = m_renderNodes[i];
			n.m_x.setZero();
			for (int j = 0; j < 3; ++j)
			{
				n.m_x += m_renderNodesParents[i][j]->m_x * m_renderNodesInterpolationWeights[i][j];
			}
			n.m_x += m_z[i] * unit_normal;
		}
	}
	else
	{
		for (int i = 0; i < m_renderNodes.size(); ++i)
		{
			RenderNode& n = m_renderNodes[i];
			n.m_x.setZero();
			for (int j = 0; j < 4; ++j)
			{
				if (m_renderNodesParents[i].size())
				{
					n.m_x += m_renderNodesParents[i][j]->m_x * m_renderNodesInterpolationWeights[i][j];
				}
			}
		}
	}
}

void btSoftBody::setCollisionQuadrature(int N)
{
	for (int i = 0; i <= N; ++i)
	{
		for (int j = 0; i + j <= N; ++j)
		{
			m_quads.push_back(btVector3(btScalar(i) / btScalar(N), btScalar(j) / btScalar(N), btScalar(N - i - j) / btScalar(N)));
		}
	}
}

//
void btSoftBody::PSolve_Anchors(btSoftBody* psb, btScalar kst, btScalar ti)
{
	BT_PROFILE("PSolve_Anchors");
	const btScalar kAHR = psb->m_cfg.kAHR * kst;
	const btScalar dt = psb->m_sst.sdt;
	for (int i = 0, ni = psb->m_anchors.size(); i < ni; ++i)
	{
		const Anchor& a = psb->m_anchors[i];
		const btTransform& t = a.m_body->getWorldTransform();
		Node& n = *a.m_node;
		const btVector3 wa = t * a.m_local;
		const btVector3 va = a.m_body->getVelocityInLocalPoint(a.m_c1) * dt;
		const btVector3 vb = n.m_x - n.m_q;
		const btVector3 vr = (va - vb) + (wa - n.m_x) * kAHR;
		const btVector3 impulse = a.m_c0 * vr * a.m_influence;
		n.m_x += impulse * a.m_c2;
		a.m_body->applyImpulse(-impulse, a.m_c1);
	}
}

//
void btSoftBody::PSolve_RContacts(btSoftBody* psb, btScalar kst, btScalar ti)
{
	BT_PROFILE("PSolve_RContacts");
	const btScalar dt = psb->m_sst.sdt;
	const btScalar mrg = psb->getCollisionShape()->getMargin();
	btMultiBodyJacobianData jacobianData;
	for (int i = 0, ni = psb->m_rcontacts.size(); i < ni; ++i)
	{
		const RContact& c = psb->m_rcontacts[i];
		const sCti& cti = c.m_cti;
		if (cti.m_colObj->hasContactResponse())
		{
			btVector3 va(0, 0, 0);
			btRigidBody* rigidCol = 0;
			btMultiBodyLinkCollider* multibodyLinkCol = 0;
			btScalar* deltaV;

			if (cti.m_colObj->getInternalType() == btCollisionObject::CO_RIGID_BODY)
			{
				rigidCol = (btRigidBody*)btRigidBody::upcast(cti.m_colObj);
				va = rigidCol ? rigidCol->getVelocityInLocalPoint(c.m_c1) * dt : btVector3(0, 0, 0);
			}
			else if (cti.m_colObj->getInternalType() == btCollisionObject::CO_FEATHERSTONE_LINK)
			{
				multibodyLinkCol = (btMultiBodyLinkCollider*)btMultiBodyLinkCollider::upcast(cti.m_colObj);
				if (multibodyLinkCol)
				{
					const int ndof = multibodyLinkCol->m_multiBody->getNumDofs() + 6;
					jacobianData.m_jacobians.resize(ndof);
					jacobianData.m_deltaVelocitiesUnitImpulse.resize(ndof);
					btScalar* jac = &jacobianData.m_jacobians[0];

					multibodyLinkCol->m_multiBody->fillContactJacobianMultiDof(multibodyLinkCol->m_link, c.m_node->m_x, cti.m_normal, jac, jacobianData.scratch_r, jacobianData.scratch_v, jacobianData.scratch_m);
					deltaV = &jacobianData.m_deltaVelocitiesUnitImpulse[0];
					multibodyLinkCol->m_multiBody->calcAccelerationDeltasMultiDof(&jacobianData.m_jacobians[0], deltaV, jacobianData.scratch_r, jacobianData.scratch_v);

					btScalar vel = 0.0;
					for (int j = 0; j < ndof; ++j)
					{
						vel += multibodyLinkCol->m_multiBody->getVelocityVector()[j] * jac[j];
					}
					va = cti.m_normal * vel * dt;
				}
			}

			const btVector3 vb = c.m_node->m_x - c.m_node->m_q;
			const btVector3 vr = vb - va;
			const btScalar dn = btDot(vr, cti.m_normal);
			if (dn <= SIMD_EPSILON)
			{
				const btScalar dp = btMin((btDot(c.m_node->m_x, cti.m_normal) + cti.m_offset), mrg);
				const btVector3 fv = vr - (cti.m_normal * dn);
				// c0 is the impulse matrix, c3 is 1 - the friction coefficient or 0, c4 is the contact hardness coefficient
				const btVector3 impulse = c.m_c0 * ((vr - (fv * c.m_c3) + (cti.m_normal * (dp * c.m_c4))) * kst);
				c.m_node->m_x -= impulse * c.m_c2;

				if (cti.m_colObj->getInternalType() == btCollisionObject::CO_RIGID_BODY)
				{
					if (rigidCol)
						rigidCol->applyImpulse(impulse, c.m_c1);
				}
				else if (cti.m_colObj->getInternalType() == btCollisionObject::CO_FEATHERSTONE_LINK)
				{
					if (multibodyLinkCol)
					{
						double multiplier = 0.5;
						multibodyLinkCol->m_multiBody->applyDeltaVeeMultiDof(deltaV, -impulse.length() * multiplier);
					}
				}
			}
		}
	}
}

