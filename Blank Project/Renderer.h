#pragma once
#include "../nclgl/OGLRenderer.h"
#include "Tree.h"
class Camera;
class Shader;
class HeightMap;
class SceneNode;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);
	void RenderScene() override;
	void UpdateScene(float dt) override;

protected:
	void LoadTextures();
	void LoadPostTextures();
	void LoadShadowTextures();
	void DrawScene();
	void DrawSkybox();
	void DrawVolcano();
	void NightScene();
	void DrawLava();
	void DrawDeadTrees(Vector3 pos);
	void DrawLavaPit();
	void DrawNode(SceneNode* n);
	void DrawShadow();
	void DrawMainScene();
	void DrawPostProcess();
	void PresentScene();

	bool nightToggle = false;
	bool fboToggle = false;

	Vector3 pos;
	Tree* root;

	float lavaRotate;
	float lavaCycle;

	Vector3 lavaPos = Vector3(50, -10, 307);

	Mesh* quad;
	Mesh* volcano;
	Mesh* deadTree;
	Mesh* sphere;
	Mesh* lavaTop;

	GLuint cubeMap;
	GLuint cubeMap2;
	GLuint earthTex;
	GLuint earthBump;
	GLuint lavaTex;
	GLuint burntTex;

	//post processing textures
	GLuint bufferFBO;
	GLuint processFBO;
	GLuint bufferColourTex[2];
	GLuint bufferDepthTex;

	//shadow textures
	GLuint shadowTex;
	GLuint shadowFBO;
	vector<Mesh*> sceneMeshes;
	vector<Matrix4> sceneTransforms;
	float sceneTime;

	Shader* skyboxShader;
	Shader* lightShader;
	Shader* texShader;
	Shader* shadowShader;
	Shader* sceneShader;
	Shader* postShader;

	Camera* camera;

	Light* light;
};