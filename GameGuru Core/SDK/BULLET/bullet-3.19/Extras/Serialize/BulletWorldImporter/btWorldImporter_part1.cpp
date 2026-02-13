void btWorldImporter::convertRigidBodyFloat(btRigidBodyFloatData* colObjData)
{
	btScalar mass = btScalar(colObjData->m_inverseMass ? 1.f / colObjData->m_inverseMass : 0.f);
	btVector3 localInertia;
	localInertia.setZero();
	btCollisionShape** shapePtr = m_shapeMap.find(colObjData->m_collisionObjectData.m_collisionShape);
	if (shapePtr && *shapePtr)
	{
		btTransform startTransform;
		colObjData->m_collisionObjectData.m_worldTransform.m_origin.m_floats[3] = 0.f;
		startTransform.deSerializeFloat(colObjData->m_collisionObjectData.m_worldTransform);

		//	startTransform.setBasis(btMatrix3x3::getIdentity());
		btCollisionShape* shape = (btCollisionShape*)*shapePtr;
		if (shape->isNonMoving())
		{
			mass = 0.f;
		}
		if (mass)
		{
			shape->calculateLocalInertia(mass, localInertia);
		}
		bool isDynamic = mass != 0.f;
		btRigidBody* body = createRigidBody(isDynamic, mass, startTransform, shape, colObjData->m_collisionObjectData.m_name);
		body->setFriction(colObjData->m_collisionObjectData.m_friction);
		body->setRestitution(colObjData->m_collisionObjectData.m_restitution);
		btVector3 linearFactor, angularFactor;
		linearFactor.deSerializeFloat(colObjData->m_linearFactor);
		angularFactor.deSerializeFloat(colObjData->m_angularFactor);
		body->setLinearFactor(linearFactor);
		body->setAngularFactor(angularFactor);

#ifdef USE_INTERNAL_EDGE_UTILITY
		if (shape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)
		{
			btBvhTriangleMeshShape* trimesh = (btBvhTriangleMeshShape*)shape;
			if (trimesh->getTriangleInfoMap())
			{
				body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
			}
		}
#endif  //USE_INTERNAL_EDGE_UTILITY
		m_bodyMap.insert(colObjData, body);
	}
	else
	{
		printf("error: no shape found\n");
	}
}

void btWorldImporter::convertRigidBodyDouble(btRigidBodyDoubleData* colObjData)
{
	btScalar mass = btScalar(colObjData->m_inverseMass ? 1.f / colObjData->m_inverseMass : 0.f);
	btVector3 localInertia;
	localInertia.setZero();
	btCollisionShape** shapePtr = m_shapeMap.find(colObjData->m_collisionObjectData.m_collisionShape);
	if (shapePtr && *shapePtr)
	{
		btTransform startTransform;
		colObjData->m_collisionObjectData.m_worldTransform.m_origin.m_floats[3] = 0.f;
		startTransform.deSerializeDouble(colObjData->m_collisionObjectData.m_worldTransform);

		//	startTransform.setBasis(btMatrix3x3::getIdentity());
		btCollisionShape* shape = (btCollisionShape*)*shapePtr;
		if (shape->isNonMoving())
		{
			mass = 0.f;
		}
		if (mass)
		{
			shape->calculateLocalInertia(mass, localInertia);
		}
		bool isDynamic = mass != 0.f;
		btRigidBody* body = createRigidBody(isDynamic, mass, startTransform, shape, colObjData->m_collisionObjectData.m_name);
		body->setFriction(btScalar(colObjData->m_collisionObjectData.m_friction));
		body->setRestitution(btScalar(colObjData->m_collisionObjectData.m_restitution));
		btVector3 linearFactor, angularFactor;
		linearFactor.deSerializeDouble(colObjData->m_linearFactor);
		angularFactor.deSerializeDouble(colObjData->m_angularFactor);
		body->setLinearFactor(linearFactor);
		body->setAngularFactor(angularFactor);

#ifdef USE_INTERNAL_EDGE_UTILITY
		if (shape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)
		{
			btBvhTriangleMeshShape* trimesh = (btBvhTriangleMeshShape*)shape;
			if (trimesh->getTriangleInfoMap())
			{
				body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
			}
		}
#endif  //USE_INTERNAL_EDGE_UTILITY
		m_bodyMap.insert(colObjData, body);
	}
	else
	{
		printf("error: no shape found\n");
	}
}
