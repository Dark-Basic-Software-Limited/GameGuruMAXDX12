void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4 & ParentTransform)
{
	std::string NodeName(pNode->mName.data);

	const aiAnimation* pAnimation = m_pScene->mAnimations[0];

	aiMatrix4x4 NodeTransformation(pNode->mTransformation);

	const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);
	aiMatrix4x4 BindPose;
	bool bFoundAnim = false;
	if (pNodeAnim) {

		aiVector3D pos = pNodeAnim->mPositionKeys[0].mValue;
		aiVector3D scale = pNodeAnim->mScalingKeys[0].mValue;
		aiQuaternion rot = pNodeAnim->mRotationKeys[0].mValue;
		BindPose = aiMatrix4x4(scale, rot, pos);

		aiVector3D Scaling = scale;
		aiMatrix4x4 ScalingM;
		InitScaleTransform(ScalingM, Scaling.x, Scaling.y, Scaling.z);

		// Interpolate rotation and generate rotation transformation matrix
		aiQuaternion RotationQ = rot;
		aiMatrix4x4 RotationM = aiMatrix4x4(RotationQ.GetMatrix());

		// Interpolate translation and generate translation transformation matrix
		aiVector3D Translation = pos;
		aiMatrix4x4 TranslationM;
		InitTranslationTransform(TranslationM, Translation.x, Translation.y, Translation.z);

		// Combine the above transformations
		//NodeTransformation = TranslationM * RotationM * ScalingM;


		bFoundAnim = true;
	}

	aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

	if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
		int BoneIndex = m_BoneMapping[NodeName];


		m_BoneInfo[BoneIndex].GlobalTransform = CalculateGlobalTransform(pNode, pNode->mParent);


		//m_GlobalInverseTransform to inverse(parentTransform)

//		m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation *
//			m_BoneInfo[BoneIndex].BoneOffset;

		aiMatrix4x4 m_ParentInverseTransform = ParentTransform;
		m_ParentInverseTransform.Inverse();
//		m_BoneInfo[BoneIndex].FinalTransformation = m_ParentInverseTransform * GlobalTransformation *
//			m_BoneInfo[BoneIndex].BoneOffset;

		//m_ArmatureInverseTransform
//		m_BoneInfo[BoneIndex].FinalTransformation = m_ArmatureInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
		//BindPose

//		m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;

		m_BoneInfo[BoneIndex].FinalTransformation = m_BoneInfo[BoneIndex].GlobalTransform;

		m_BoneInfo[BoneIndex].NodeTransformation = NodeTransformation;
		m_BoneInfo[BoneIndex].bNodeFound = bFoundAnim;
	}

	for (int i = 0; i < pNode->mNumChildren; i++) {
		ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
	}
}

#else

bool LoadAssImpObject(char* pModelFilename, sObject** ppObject, enumScalingMode eScalingMode)
{
	return false;
}

#endif
