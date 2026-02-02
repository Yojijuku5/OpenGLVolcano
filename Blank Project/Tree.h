#pragma once
#include "../nclgl/SceneNode.h"

class Tree : public SceneNode {
public:
	Tree(Mesh* sphere);
	~Tree(void) {};

	vector<Vector3>GetAllBasePos();

protected:
	SceneNode* bushBase;
	SceneNode* bushFringe;

	Vector3 basePos;
	vector<Vector3>allBasePos;
};

