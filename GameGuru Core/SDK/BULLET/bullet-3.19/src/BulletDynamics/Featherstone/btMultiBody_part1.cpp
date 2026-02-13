void btMultiBody::fillConstraintJacobianMultiDof(int link,
												 const btVector3 &contact_point,
												 const btVector3 &normal_ang,
												 const btVector3 &normal_lin,
												 btScalar *jac,
												 btAlignedObjectArray<btScalar> &scratch_r1,
												 btAlignedObjectArray<btVector3> &scratch_v,
												 btAlignedObjectArray<btMatrix3x3> &scratch_m) const
{
	// temporary space
	int num_links = getNumLinks();
	int m_dofCount = getNumDofs();
	scratch_v.resize(3 * num_links + 3);  //(num_links + base) offsets + (num_links + base) normals_lin + (num_links + base) normals_ang
	scratch_m.resize(num_links + 1);

	btVector3 *v_ptr = &scratch_v[0];
	btVector3 *p_minus_com_local = v_ptr;
	v_ptr += num_links + 1;
	btVector3 *n_local_lin = v_ptr;
	v_ptr += num_links + 1;
	btVector3 *n_local_ang = v_ptr;
	v_ptr += num_links + 1;
	btAssert(v_ptr - &scratch_v[0] == scratch_v.size());

	//scratch_r.resize(m_dofCount);
	//btScalar *results = m_dofCount > 0 ? &scratch_r[0] : 0;

    scratch_r1.resize(m_dofCount+num_links);
    btScalar * results = m_dofCount > 0 ? &scratch_r1[0] : 0;
    btScalar* links = num_links? &scratch_r1[m_dofCount] : 0;
    int numLinksChildToRoot=0;
    int l = link;
    while (l != -1)
    {
        links[numLinksChildToRoot++]=l;
        l = m_links[l].m_parent;
    }
    
	btMatrix3x3 *rot_from_world = &scratch_m[0];

	const btVector3 p_minus_com_world = contact_point - m_basePos;
	const btVector3 &normal_lin_world = normal_lin;  //convenience
	const btVector3 &normal_ang_world = normal_ang;

	rot_from_world[0] = btMatrix3x3(m_baseQuat);

	// omega coeffients first.
	btVector3 omega_coeffs_world;
	omega_coeffs_world = p_minus_com_world.cross(normal_lin_world);
	jac[0] = omega_coeffs_world[0] + normal_ang_world[0];
	jac[1] = omega_coeffs_world[1] + normal_ang_world[1];
	jac[2] = omega_coeffs_world[2] + normal_ang_world[2];
	// then v coefficients
	jac[3] = normal_lin_world[0];
	jac[4] = normal_lin_world[1];
	jac[5] = normal_lin_world[2];

	//create link-local versions of p_minus_com and normal
	p_minus_com_local[0] = rot_from_world[0] * p_minus_com_world;
	n_local_lin[0] = rot_from_world[0] * normal_lin_world;
	n_local_ang[0] = rot_from_world[0] * normal_ang_world;

	// Set remaining jac values to zero for now.
	for (int i = 6; i < 6 + m_dofCount; ++i)
	{
		jac[i] = 0;
	}

	// Qdot coefficients, if necessary.
	if (num_links > 0 && link > -1)
	{
        // TODO: (Also, we are making 3 separate calls to this function, for the normal & the 2 friction directions,
		// which is resulting in repeated work being done...)

		// calculate required normals & positions in the local frames.
        for (int a = 0; a < numLinksChildToRoot; a++)
        {
            int i = links[numLinksChildToRoot-1-a];
        	// transform to local frame
			const int parent = m_links[i].m_parent;
			const btMatrix3x3 mtx(m_links[i].m_cachedRotParentToThis);
			rot_from_world[i + 1] = mtx * rot_from_world[parent + 1];

			n_local_lin[i + 1] = mtx * n_local_lin[parent + 1];
			n_local_ang[i + 1] = mtx * n_local_ang[parent + 1];
			p_minus_com_local[i + 1] = mtx * p_minus_com_local[parent + 1] - m_links[i].m_cachedRVector;

			// calculate the jacobian entry
			switch (m_links[i].m_jointType)
			{
				case btMultibodyLink::eRevolute:
				{
					results[m_links[i].m_dofOffset] = n_local_lin[i + 1].dot(m_links[i].getAxisTop(0).cross(p_minus_com_local[i + 1]) + m_links[i].getAxisBottom(0));
					results[m_links[i].m_dofOffset] += n_local_ang[i + 1].dot(m_links[i].getAxisTop(0));
					break;
				}
				case btMultibodyLink::ePrismatic:
				{
					results[m_links[i].m_dofOffset] = n_local_lin[i + 1].dot(m_links[i].getAxisBottom(0));
					break;
				}
				case btMultibodyLink::eSpherical:
				{
					results[m_links[i].m_dofOffset + 0] = n_local_lin[i + 1].dot(m_links[i].getAxisTop(0).cross(p_minus_com_local[i + 1]) + m_links[i].getAxisBottom(0));
					results[m_links[i].m_dofOffset + 1] = n_local_lin[i + 1].dot(m_links[i].getAxisTop(1).cross(p_minus_com_local[i + 1]) + m_links[i].getAxisBottom(1));
					results[m_links[i].m_dofOffset + 2] = n_local_lin[i + 1].dot(m_links[i].getAxisTop(2).cross(p_minus_com_local[i + 1]) + m_links[i].getAxisBottom(2));

					results[m_links[i].m_dofOffset + 0] += n_local_ang[i + 1].dot(m_links[i].getAxisTop(0));
					results[m_links[i].m_dofOffset + 1] += n_local_ang[i + 1].dot(m_links[i].getAxisTop(1));
					results[m_links[i].m_dofOffset + 2] += n_local_ang[i + 1].dot(m_links[i].getAxisTop(2));

					break;
				}
				case btMultibodyLink::ePlanar:
				{
					results[m_links[i].m_dofOffset + 0] = n_local_lin[i + 1].dot(m_links[i].getAxisTop(0).cross(p_minus_com_local[i + 1]));  // + m_links[i].getAxisBottom(0));
					results[m_links[i].m_dofOffset + 1] = n_local_lin[i + 1].dot(m_links[i].getAxisBottom(1));
					results[m_links[i].m_dofOffset + 2] = n_local_lin[i + 1].dot(m_links[i].getAxisBottom(2));

					break;
				}
				default:
				{
				}
			}
		}

		// Now copy through to output.
		//printf("jac[%d] = ", link);
		while (link != -1)
		{
			for (int dof = 0; dof < m_links[link].m_dofCount; ++dof)
			{
				jac[6 + m_links[link].m_dofOffset + dof] = results[m_links[link].m_dofOffset + dof];
				//printf("%.2f\t", jac[6 + m_links[link].m_dofOffset + dof]);
			}

			link = m_links[link].m_parent;
		}
		//printf("]\n");
	}
}

void btMultiBody::wakeUp()
{
	m_sleepTimer = 0;
	m_awake = true;
}

void btMultiBody::goToSleep()
{
	m_awake = false;
}

void btMultiBody::checkMotionAndSleepIfRequired(btScalar timestep)
{
	extern bool gDisableDeactivation;
	if (!m_canSleep || gDisableDeactivation)
	{
		m_awake = true;
		m_sleepTimer = 0;
		return;
	}

	

	// motion is computed as omega^2 + v^2 + (sum of squares of joint velocities)
	btScalar motion = 0;
	{
		for (int i = 0; i < 6 + m_dofCount; ++i)
			motion += m_realBuf[i] * m_realBuf[i];
	}

	if (motion < m_sleepEpsilon)
	{
		m_sleepTimer += timestep;
		if (m_sleepTimer > m_sleepTimeout)
		{
			goToSleep();
		}
	}
	else
	{
		m_sleepTimer = 0;
		if (m_canWakeup)
		{
			if (!m_awake)
				wakeUp();
		}
	}
}

void btMultiBody::forwardKinematics(btAlignedObjectArray<btQuaternion> &world_to_local, btAlignedObjectArray<btVector3> &local_origin)
{
	int num_links = getNumLinks();

	// Cached 3x3 rotation matrices from parent frame to this frame.
	btMatrix3x3 *rot_from_parent = (btMatrix3x3 *)&m_matrixBuf[0];

	rot_from_parent[0] = btMatrix3x3(m_baseQuat);  //m_baseQuat assumed to be alias!?

	for (int i = 0; i < num_links; ++i)
	{
		rot_from_parent[i + 1] = btMatrix3x3(m_links[i].m_cachedRotParentToThis);
	}

	int nLinks = getNumLinks();
	///base + num m_links
	world_to_local.resize(nLinks + 1);
	local_origin.resize(nLinks + 1);

	world_to_local[0] = getWorldToBaseRot();
	local_origin[0] = getBasePos();

	for (int k = 0; k < getNumLinks(); k++)
	{
		const int parent = getParent(k);
		world_to_local[k + 1] = getParentToLocalRot(k) * world_to_local[parent + 1];
		local_origin[k + 1] = local_origin[parent + 1] + (quatRotate(world_to_local[k + 1].inverse(), getRVector(k)));
	}

	for (int link = 0; link < getNumLinks(); link++)
	{
		int index = link + 1;

		btVector3 posr = local_origin[index];
		btScalar quat[4] = {-world_to_local[index].x(), -world_to_local[index].y(), -world_to_local[index].z(), world_to_local[index].w()};
		btTransform tr;
		tr.setIdentity();
		tr.setOrigin(posr);
		tr.setRotation(btQuaternion(quat[0], quat[1], quat[2], quat[3]));
		getLink(link).m_cachedWorldTransform = tr;
	}
}

void btMultiBody::updateCollisionObjectWorldTransforms(btAlignedObjectArray<btQuaternion> &world_to_local, btAlignedObjectArray<btVector3> &local_origin)
{
	world_to_local.resize(getNumLinks() + 1);
	local_origin.resize(getNumLinks() + 1);

	world_to_local[0] = getWorldToBaseRot();
	local_origin[0] = getBasePos();

	if (getBaseCollider())
	{
		btVector3 posr = local_origin[0];
		//	float pos[4]={posr.x(),posr.y(),posr.z(),1};
		btScalar quat[4] = {-world_to_local[0].x(), -world_to_local[0].y(), -world_to_local[0].z(), world_to_local[0].w()};
		btTransform tr;
		tr.setIdentity();
		tr.setOrigin(posr);
		tr.setRotation(btQuaternion(quat[0], quat[1], quat[2], quat[3]));

		getBaseCollider()->setWorldTransform(tr);
        getBaseCollider()->setInterpolationWorldTransform(tr);
	}

	for (int k = 0; k < getNumLinks(); k++)
	{
		const int parent = getParent(k);
		world_to_local[k + 1] = getParentToLocalRot(k) * world_to_local[parent + 1];
		local_origin[k + 1] = local_origin[parent + 1] + (quatRotate(world_to_local[k + 1].inverse(), getRVector(k)));
	}

	for (int m = 0; m < getNumLinks(); m++)
	{
		btMultiBodyLinkCollider *col = getLink(m).m_collider;
		if (col)
		{
			int link = col->m_link;
			btAssert(link == m);

			int index = link + 1;

			btVector3 posr = local_origin[index];
			//			float pos[4]={posr.x(),posr.y(),posr.z(),1};
			btScalar quat[4] = {-world_to_local[index].x(), -world_to_local[index].y(), -world_to_local[index].z(), world_to_local[index].w()};
			btTransform tr;
			tr.setIdentity();
			tr.setOrigin(posr);
			tr.setRotation(btQuaternion(quat[0], quat[1], quat[2], quat[3]));

			col->setWorldTransform(tr);
            col->setInterpolationWorldTransform(tr);
		}
	}
}

void btMultiBody::updateCollisionObjectInterpolationWorldTransforms(btAlignedObjectArray<btQuaternion> &world_to_local, btAlignedObjectArray<btVector3> &local_origin)
{
    world_to_local.resize(getNumLinks() + 1);
    local_origin.resize(getNumLinks() + 1);
    
		if(isBaseKinematic()){
        world_to_local[0] = getWorldToBaseRot();
        local_origin[0] = getBasePos();
		}
		else
		{
        world_to_local[0] = getInterpolateWorldToBaseRot();
        local_origin[0] = getInterpolateBasePos();
		}
    
    if (getBaseCollider())
    {
        btVector3 posr = local_origin[0];
        //    float pos[4]={posr.x(),posr.y(),posr.z(),1};
        btScalar quat[4] = {-world_to_local[0].x(), -world_to_local[0].y(), -world_to_local[0].z(), world_to_local[0].w()};
        btTransform tr;
        tr.setIdentity();
        tr.setOrigin(posr);
        tr.setRotation(btQuaternion(quat[0], quat[1], quat[2], quat[3]));
        
        getBaseCollider()->setInterpolationWorldTransform(tr);
    }
    
    for (int k = 0; k < getNumLinks(); k++)
    {
        const int parent = getParent(k);
        world_to_local[k + 1] = getInterpolateParentToLocalRot(k) * world_to_local[parent + 1];
        local_origin[k + 1] = local_origin[parent + 1] + (quatRotate(world_to_local[k + 1].inverse(), getInterpolateRVector(k)));
    }
    
    for (int m = 0; m < getNumLinks(); m++)
    {
        btMultiBodyLinkCollider *col = getLink(m).m_collider;
        if (col)
        {
            int link = col->m_link;
            btAssert(link == m);
            
            int index = link + 1;
            
            btVector3 posr = local_origin[index];
            //            float pos[4]={posr.x(),posr.y(),posr.z(),1};
            btScalar quat[4] = {-world_to_local[index].x(), -world_to_local[index].y(), -world_to_local[index].z(), world_to_local[index].w()};
            btTransform tr;
            tr.setIdentity();
            tr.setOrigin(posr);
            tr.setRotation(btQuaternion(quat[0], quat[1], quat[2], quat[3]));
            
            col->setInterpolationWorldTransform(tr);
        }
    }
}

int btMultiBody::calculateSerializeBufferSize() const
{
	int sz = sizeof(btMultiBodyData);
	return sz;
}

///fills the dataBuffer and returns the struct name (and 0 on failure)
const char *btMultiBody::serialize(void *dataBuffer, class btSerializer *serializer) const
{
	btMultiBodyData *mbd = (btMultiBodyData *)dataBuffer;
	getBasePos().serialize(mbd->m_baseWorldPosition);
	getWorldToBaseRot().inverse().serialize(mbd->m_baseWorldOrientation);
	getBaseVel().serialize(mbd->m_baseLinearVelocity);
	getBaseOmega().serialize(mbd->m_baseAngularVelocity);

	mbd->m_baseMass = this->getBaseMass();
	getBaseInertia().serialize(mbd->m_baseInertia);
	{
		char *name = (char *)serializer->findNameForPointer(m_baseName);
		mbd->m_baseName = (char *)serializer->getUniquePointer(name);
		if (mbd->m_baseName)
		{
			serializer->serializeName(name);
		}
	}
	mbd->m_numLinks = this->getNumLinks();
	if (mbd->m_numLinks)
	{
		int sz = sizeof(btMultiBodyLinkData);
		int numElem = mbd->m_numLinks;
		btChunk *chunk = serializer->allocate(sz, numElem);
		btMultiBodyLinkData *memPtr = (btMultiBodyLinkData *)chunk->m_oldPtr;
		for (int i = 0; i < numElem; i++, memPtr++)
		{
			memPtr->m_jointType = getLink(i).m_jointType;
			memPtr->m_dofCount = getLink(i).m_dofCount;
			memPtr->m_posVarCount = getLink(i).m_posVarCount;

			getLink(i).m_inertiaLocal.serialize(memPtr->m_linkInertia);

			getLink(i).m_absFrameTotVelocity.m_topVec.serialize(memPtr->m_absFrameTotVelocityTop);
			getLink(i).m_absFrameTotVelocity.m_bottomVec.serialize(memPtr->m_absFrameTotVelocityBottom);
			getLink(i).m_absFrameLocVelocity.m_topVec.serialize(memPtr->m_absFrameLocVelocityTop);
			getLink(i).m_absFrameLocVelocity.m_bottomVec.serialize(memPtr->m_absFrameLocVelocityBottom);

			memPtr->m_linkMass = getLink(i).m_mass;
			memPtr->m_parentIndex = getLink(i).m_parent;
			memPtr->m_jointDamping = getLink(i).m_jointDamping;
			memPtr->m_jointFriction = getLink(i).m_jointFriction;
			memPtr->m_jointLowerLimit = getLink(i).m_jointLowerLimit;
			memPtr->m_jointUpperLimit = getLink(i).m_jointUpperLimit;
			memPtr->m_jointMaxForce = getLink(i).m_jointMaxForce;
			memPtr->m_jointMaxVelocity = getLink(i).m_jointMaxVelocity;

			getLink(i).m_eVector.serialize(memPtr->m_parentComToThisPivotOffset);
			getLink(i).m_dVector.serialize(memPtr->m_thisPivotToThisComOffset);
			getLink(i).m_zeroRotParentToThis.serialize(memPtr->m_zeroRotParentToThis);
			btAssert(memPtr->m_dofCount <= 3);
			for (int dof = 0; dof < getLink(i).m_dofCount; dof++)
			{
				getLink(i).getAxisBottom(dof).serialize(memPtr->m_jointAxisBottom[dof]);
				getLink(i).getAxisTop(dof).serialize(memPtr->m_jointAxisTop[dof]);

				memPtr->m_jointTorque[dof] = getLink(i).m_jointTorque[dof];
				memPtr->m_jointVel[dof] = getJointVelMultiDof(i)[dof];
			}
			int numPosVar = getLink(i).m_posVarCount;
			for (int posvar = 0; posvar < numPosVar; posvar++)
			{
				memPtr->m_jointPos[posvar] = getLink(i).m_jointPos[posvar];
			}

			{
				char *name = (char *)serializer->findNameForPointer(m_links[i].m_linkName);
				memPtr->m_linkName = (char *)serializer->getUniquePointer(name);
				if (memPtr->m_linkName)
				{
					serializer->serializeName(name);
				}
			}
			{
				char *name = (char *)serializer->findNameForPointer(m_links[i].m_jointName);
				memPtr->m_jointName = (char *)serializer->getUniquePointer(name);
				if (memPtr->m_jointName)
				{
					serializer->serializeName(name);
				}
			}
			memPtr->m_linkCollider = (btCollisionObjectData *)serializer->getUniquePointer(getLink(i).m_collider);
		}
		serializer->finalizeChunk(chunk, btMultiBodyLinkDataName, BT_ARRAY_CODE, (void *)&m_links[0]);
	}
	mbd->m_links = mbd->m_numLinks ? (btMultiBodyLinkData *)serializer->getUniquePointer((void *)&m_links[0]) : 0;

	// Fill padding with zeros to appease msan.
#ifdef BT_USE_DOUBLE_PRECISION
	memset(mbd->m_padding, 0, sizeof(mbd->m_padding));
#endif

	return btMultiBodyDataName;
}

void btMultiBody::saveKinematicState(btScalar timeStep)
{
	//todo: clamp to some (user definable) safe minimum timestep, to limit maximum angular/linear velocities
	if (m_kinematic_calculate_velocity && timeStep != btScalar(0.))
	{
		btVector3 linearVelocity, angularVelocity;
		btTransformUtil::calculateVelocity(getInterpolateBaseWorldTransform(), getBaseWorldTransform(), timeStep, linearVelocity, angularVelocity);
		setBaseVel(linearVelocity);
		setBaseOmega(angularVelocity);
		setInterpolateBaseWorldTransform(getBaseWorldTransform());
	}
}

void btMultiBody::setLinkDynamicType(const int i, int type)
{
	if (i == -1)
	{
		setBaseDynamicType(type);
	}
	else if (i >= 0 && i < getNumLinks())
	{
		if (m_links[i].m_collider)
		{
			m_links[i].m_collider->setDynamicType(type);
		}
	}
}

bool btMultiBody::isLinkStaticOrKinematic(const int i) const
{
	if (i == -1)
	{
		return isBaseStaticOrKinematic();
	}
	else
	{
		if (m_links[i].m_collider)
			return m_links[i].m_collider->isStaticOrKinematic();
	}
	return false;
}

bool btMultiBody::isLinkKinematic(const int i) const
{
	if (i == -1)
	{
		return isBaseKinematic();
	}
	else
	{
		if (m_links[i].m_collider)
			return m_links[i].m_collider->isKinematic();
	}
	return false;
}

bool btMultiBody::isLinkAndAllAncestorsStaticOrKinematic(const int i) const
{
	int link = i;
	while (link != -1) {
		if (!isLinkStaticOrKinematic(link))
			return false;
		link = m_links[link].m_parent;
	}
	return isBaseStaticOrKinematic();
}

bool btMultiBody::isLinkAndAllAncestorsKinematic(const int i) const
{
	int link = i;
	while (link != -1) {
		if (!isLinkKinematic(link))
			return false;
		link = m_links[link].m_parent;
	}
	return isBaseKinematic();
}
