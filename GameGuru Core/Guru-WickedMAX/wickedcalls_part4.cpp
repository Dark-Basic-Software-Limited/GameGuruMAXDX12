//iAction = 1 Burst all. 2 = Pause. - 3 = Resume. - 4 = Restart - 5 - visible - 6 = not visible. - 7 = pause emit - 8 = resume emit
void WickedCall_PerformEmitterAction(int iAction, uint32_t emitter_root)
{

	Scene& scene = wiScene::GetScene();

	//PE: Scan emitters.
	for (int i = 0; i < scene.emitters.GetCount(); i++)
	{
		Entity emitter = scene.emitters.GetEntity(i);
		HierarchyComponent* hier = scene.hierarchy.GetComponent(emitter);
		if (hier)
		{
			if (hier->parentID == emitter_root)
			{
				wiEmittedParticle* ec = scene.emitters.GetComponent(emitter);
				switch (iAction)
				{
					case 1:
					{
						ec->Burst(0);
						break;
					}
					case 2:
					{
						ec->SetPaused(true);
						break;
					}
					case 3:
					{
						ec->SetPaused(false);
						break;
					}
					case 4:
					{
						ec->Restart();
						break;
					}
					case 5:
					{
						ec->SetVisible(true);
						break;
					}
					case 6:
					{
						ec->SetVisible(false);
						break;
					}
					case 7:
					{
						ec->SetEmitPaused(true);
						break;
					}
					case 8:
					{
						ec->SetEmitPaused(false);
						break;
					}
				}
			}
		}
	}
}

bool WickedCall_ParticleEffectPositionRotation(uint32_t root, float fX, float fY, float fZ, float fXa, float fYa, float fZa)
{
	Scene& scene = wiScene::GetScene();
	TransformComponent* root_tranform = scene.transforms.GetComponent(root);
	if (root_tranform)
	{
		root_tranform->ClearTransform();

		float rotationRadiansX = fXa * (XM_PI / 180.0f);
		float rotationRadiansY = fYa * (XM_PI / 180.0f);
		float rotationRadiansZ = fZa * (XM_PI / 180.0f);
		XMFLOAT3 rot = { rotationRadiansX ,rotationRadiansY ,rotationRadiansZ }; //PE: 0 - XM_2PI
		root_tranform->RotateRollPitchYaw(rot);

		root_tranform->Translate(XMFLOAT3(fX, fY, fZ));
		root_tranform->UpdateTransform();
		return true;
	}
	return false;
}

bool WickedCall_ParticleEffectPosition(uint32_t root, float fX, float fY, float fZ)
{
	Scene& scene = wiScene::GetScene();
	TransformComponent* root_tranform = scene.transforms.GetComponent(root);
	if (root_tranform)
	{
		root_tranform->ClearTransform();
		root_tranform->Translate(XMFLOAT3(fX, fY, fZ));
		root_tranform->UpdateTransform();
		return true;
	}
	return false;
}

//#define WPEDebug
void WickedCall_UpdateEmitters(void)
{
#ifdef OPTICK_ENABLE
	OPTICK_EVENT();
#endif

	//PE: Scan emitters.
	std::vector< uint32_t> parent_used;
	parent_used.clear();
	Scene& scene = wiScene::GetScene();
	for (int i = 0; i < scene.emitters.GetCount(); i++)
	{
		Entity emitter = scene.emitters.GetEntity(i);
		wiEmittedParticle* ec = scene.emitters.GetComponent(emitter);

#ifdef WPEDebug
		if (ec && ec->IsVolumeEnabled())
		{
			XMFLOAT4X4 hoverBox;
			wiScene::TransformComponent* pTransform = wiScene::GetScene().transforms.GetComponent(emitter);
			if (pTransform)
			{
				if (1)
				{
					AABB aabb;
					XMFLOAT3 pos = pTransform->GetPosition();

					XMFLOAT3 sca = pTransform->GetScale();
					aabb._min = XMFLOAT3(pos.x - sca.x, pos.y, pos.z - sca.z);
					aabb._max = XMFLOAT3(pos.x + sca.x, pos.y + sca.y, pos.z + sca.z);

					XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix()); // *pTransform->GetLocalMatrix());
					XMVECTOR S, R, T;
					XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&hoverBox));

					//XMVECTOR R_local = XMLoadFloat4(&root_tranform->rotation_local);
					XMVECTOR R_local = XMLoadFloat4(&pTransform->rotation_local);
					XMStoreFloat4x4(&hoverBox,
						XMMatrixScalingFromVector(S) *
						XMMatrixRotationQuaternion(R_local) *
						XMMatrixTranslationFromVector(T));

					wiRenderer::DrawBox(hoverBox, XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
				}
			}
		}
#endif

		//PE: If bFollowCamera , find InDoor , OutDoor , UnderWater.
		//PE: bFindFloor ONLY if ec->bFollowCamera
		if (ec && (ec->bFindFloor || ec->bFollowCamera))
		{
			HierarchyComponent* hier = scene.hierarchy.GetComponent(emitter);
			if (hier)
			{
				if (hier->parentID != wiECS::INVALID_ENTITY)
				{
					bool bAlreadySet = false;
					for (int a = 0; a > parent_used.size(); a++)
					{
						if (parent_used[a] == hier->parentID)
						{
							bAlreadySet = true;
							break;
						}
					}
					if (!bAlreadySet)
					{
						parent_used.push_back(hier->parentID);
						TransformComponent* root_tranform = scene.transforms.GetComponent(hier->parentID);
						if (root_tranform)
						{

							if (ec->bFollowCamera)
							{
								float fX, fY, fZ;
								fX = CameraPositionX();
								//PE: PlayerHeight might have to be removed in GGM
								fY = CameraPositionY(); // -PlayerHeight;
								fZ = CameraPositionZ();
								root_tranform->ClearTransform();
								root_tranform->Translate(XMFLOAT3(fX, fY, fZ));
								root_tranform->UpdateTransform();
							}
							if (ec->bFindFloor && ec->bFollowCamera)
							{
								float fX = root_tranform->GetPosition().x;
								float fZ = root_tranform->GetPosition().z;
								float height = BT_GetGroundHeight(t.terrain.TerrainID, CameraPositionX(), CameraPositionZ());
								root_tranform->ClearTransform();
								root_tranform->Translate(XMFLOAT3(fX, height, fZ));
								root_tranform->UpdateTransform();
							}
						}
					}
				}
			}
		}
	}
}

uint32_t WickedCall_CreateEmitter(std::string& name, float posX, float posY, float posZ, uint32_t proot)
{
	XMFLOAT3 position = { posX , posY, posZ };

	//wiScene::Scene& scene = wiScene::GetScene();
	Scene scene;
	XMMATRIX& transformMatrix = XMMatrixIdentity();

	//PE: Create emitter.
	Entity entity = CreateEntity();

	scene.names.Create(entity) = name;
	scene.emitters.Create(entity).count = 10;

	wiEmittedParticle* ec;
	ec = scene.emitters.GetComponent(entity);
	ec->count = 40;
	ec->life = 2.5f;
	ec->size = 2;
	ec->gravity = XMFLOAT3(0, 9, 0);

	TransformComponent& transform = scene.transforms.Create(entity);
	transform.ClearTransform();
	transform.Translate(position);
	transform.Scale(XMFLOAT3(3, 1, 3));
	transform.UpdateTransform();

	scene.materials.Create(entity).userBlendMode = BLENDMODE_ADDITIVE; // BLENDMODE_ALPHA;

	//PE: Create root.
	Entity root;
	bool bUsePrevRoot = false;
	if (proot > 0)
	{
		root = proot;
		bUsePrevRoot = true;
	}
	else
	{
		root = CreateEntity();
		scene.transforms.Create(root);
		scene.layers.Create(root).layerMask = ~0;
	}

	if (!bUsePrevRoot)
	{
		//PE: Parent all unparented transforms to new root entity
		for (size_t i = 0; i < scene.transforms.GetCount() - 1; ++i) // GetCount() - 1 because the last added was the "root"
		{
			Entity entity = scene.transforms.GetEntity(i);
			if (!scene.hierarchy.Contains(entity))
			{
				scene.Component_Attach(entity, root);
			}
		}
		//PE: The root component is transformed, scene is updated:
		scene.transforms.GetComponent(root)->MatrixTransform(transformMatrix);
		scene.Update(0);
	}
	GetScene().Merge(scene);

	//PE: Find name;
	wiScene::Scene& sceneR = wiScene::GetScene();

	for (int i = 0; i < sceneR.emitters.GetCount(); i++)
	{

		Entity emitter = sceneR.emitters.GetEntity(i);
		Entity text = sceneR.names.GetIndex(emitter);
		if (text > 0)
		{
			if (sceneR.names[text].name == name)
			{
				entity = emitter;
				break;
			}
		}
	}

	if (bUsePrevRoot)
	{
		sceneR.Component_Attach(entity, root);
		wiScene::TransformComponent* pTransform = wiScene::GetScene().transforms.GetComponent(entity);
		pTransform->ClearTransform();
		pTransform->Translate(position);
		pTransform->UpdateTransform();
	}

	return entity;
}

uint32_t GetVisibleWEmitters( void )
{
	uint32_t total_visible = 0;
	Scene& scene = wiScene::GetScene();
	for (int i = 0; i < scene.emitters.GetCount(); i++)
	{
		Entity emitter = scene.emitters.GetEntity(i);
		wiEmittedParticle& ec = scene.emitters[i];
		if (!ec.IsVisible())
			continue;
		if (!ec.IsActive())
			continue;
		total_visible++;
	}
	return total_visible;
}

#endif


void WickedCall_SetShaderParameter(int obj, int parameter , float value)
{
	sObject* GetObjectData(int iID);
	sObject* pObject = GetObjectData(obj);
	if (pObject)
	{
		for (int i = 0; i < pObject->iMeshCount; i++)
		{
			sMesh* mesh = pObject->ppMeshList[i];

			if (mesh)
			{
				wiScene::MeshComponent* meshComponent = wiScene::GetScene().meshes.GetComponent(mesh->wickedmeshindex);

				if (meshComponent)
				{
					// get material settings from mesh material or WEMaterial
					uint64_t materialEntity = meshComponent->subsets[0].materialID;
					wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
					if (pMeshMaterial->customShaderID >= 0)
					{
						bool bChanged = false;
						if (parameter == 1)
						{
							if (value != pMeshMaterial->customShaderParam1)
							{
								pMeshMaterial->customShaderParam1 = value;
								bChanged = true;
							}
						}
						if (parameter == 2)
						{
							if (value != pMeshMaterial->customShaderParam2)
							{
								pMeshMaterial->customShaderParam2 = value;
								bChanged = true;
							}
						}
						if (parameter == 3)
						{
							if (value != pMeshMaterial->customShaderParam3)
							{
								pMeshMaterial->customShaderParam3 = value;
								bChanged = true;
							}
						}
						if (parameter == 4)
						{
							if (value != pMeshMaterial->customShaderParam4)
							{
								pMeshMaterial->customShaderParam4 = value;
								bChanged = true;
							}
						}
						if (parameter == 5)
						{
							if (value != pMeshMaterial->customShaderParam5)
							{
								pMeshMaterial->customShaderParam5 = value;
								bChanged = true;
							}
						}
						if (parameter == 6)
						{
							if (value != pMeshMaterial->customShaderParam6)
							{
								pMeshMaterial->customShaderParam6 = value;
								bChanged = true;
							}
						}
						if (parameter == 7)
						{
							if (value != pMeshMaterial->customShaderParam7)
							{
								pMeshMaterial->customShaderParam7 = value;
								bChanged = true;
							}
						}
						if(bChanged)
							pMeshMaterial->SetDirty();

					}
				}
			}
		}

	}

}


