#include <cstdlib>
#include <ctime>
#include <random>
#include "Tree.h"

Tree::Tree(Mesh* sphere) {
	std::mt19937 rng1(static_cast<unsigned int>(time(nullptr)));
	std::uniform_real_distribution<float> dist1x(-75.0f, 8.0f);
	std::uniform_real_distribution<float> dist1z(-20.0f, 45.0f);
	std::mt19937 rng2(static_cast<unsigned int>(time(nullptr)));
	std::uniform_real_distribution<float> dist2(-2.0f, 2.0f);

	for (int i = 0; i < 50; ++i) {
		float testX = dist1x(rng1);
		float testZ = dist1z(rng1);

		bushBase = new SceneNode(sphere, Vector4(0, 1, 0, 1));
		bushBase->SetModelScale(Vector3(1, 1, 1));
		bushBase->SetTransform(Matrix4::Translation(Vector3(testX, 9, testZ)));//original (-64, 11, -12)
		//edgesx = 8, -75
		//edgesz = 45, -20
		basePos = Vector3(testX, 11, testZ);
		allBasePos.push_back(basePos);
		AddChild(bushBase);

		for (int i = 0; i < 20; ++i) {
			float randX = dist2(rng2);
			float randY = dist2(rng2);
			float randZ = dist2(rng2);

			bushFringe = new SceneNode(sphere, Vector4(0, 1, 0, 1));
			bushFringe->SetModelScale(Vector3(1, 1, 1));
			bushFringe->SetTransform(Matrix4::Translation(Vector3(randX, randY, randZ)));
			bushBase->AddChild(bushFringe);
		}
	}
}

vector<Vector3> Tree::GetAllBasePos() {
	return allBasePos;
}