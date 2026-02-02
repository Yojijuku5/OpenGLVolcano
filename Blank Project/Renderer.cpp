#include "Renderer.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include "../nclgl/SceneNode.h"
#include "Tree.h";

#define SHADOWSIZE 2048
const int POST_PASSES = 10;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	quad = Mesh::GenerateQuad();

	volcano = Mesh::LoadFromMeshFile("pPlane1.msh");
	deadTree = Mesh::LoadFromMeshFile("dead_tree.msh");
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	lavaTop = Mesh::LoadFromMeshFile("lava_top.msh");

	if (!volcano || !deadTree || !sphere || !lavaTop) {
		std::cout << "Failed to load mesh" << std::endl;
	}

	LoadTextures();

	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	lightShader = new Shader("bumpVertex.glsl", "bumpFragment.glsl");
	texShader = new Shader("texturedVertex.glsl", "texturedFragment.glsl");
	shadowShader = new Shader("shadowVertex.glsl", "shadowFragment.glsl");
	sceneShader = new Shader("shadowSceneVertex.glsl", "shadowSceneFragment.glsl");
	postShader = new Shader("bumpVertex.glsl", "processFragment.glsl");

	if (!skyboxShader->LoadSuccess() || !lightShader->LoadSuccess() || !texShader->LoadSuccess() || !shadowShader->LoadSuccess() || !sceneShader->LoadSuccess() || !postShader->LoadSuccess()) {
		return;
	}

	LoadPostTextures();
	LoadShadowTextures();

	camera = new Camera(-30.0f, 100.0f, Vector3(0.9f, 40.0f, 1.0f));

	root = new Tree(sphere);
	root->AddChild(new Tree(sphere));

	light = new Light(Vector3(20.0f, 60.0f, 30.0f), Vector4(1, 1, 0, 1), 1000.0f);

	sceneTransforms.resize(4);
	sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 1));
	sceneTime = 0.0f;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	lavaRotate = 0.0f;
	lavaCycle = 0.0f;

	init = true;
}

Renderer::~Renderer(void) {
	delete quad;
	delete volcano;
	delete deadTree;
	delete sphere;
	delete lavaTop;
	delete skyboxShader;
	delete lightShader;
	delete texShader;
	delete shadowShader;
	delete postShader;
	delete camera;
	delete root;
	delete light;

	for (auto& i : sceneMeshes) {
		delete i;
	}
	
	glDeleteTextures(1, &cubeMap);
	glDeleteTextures(1, &cubeMap2);
	glDeleteTextures(1, &earthTex);
	glDeleteTextures(1, &lavaTex);
	glDeleteTextures(1, &burntTex);

	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);

	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
}

void Renderer::LoadTextures() {
	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"watered_west.jpg", TEXTUREDIR"watered_east.jpg", TEXTUREDIR"watered_up.jpg", TEXTUREDIR"watered_down.jpg", TEXTUREDIR"watered_south.jpg", TEXTUREDIR"watered_north.jpg", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
	cubeMap2 = SOIL_load_OGL_cubemap(TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg", TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg", TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	lavaTex = SOIL_load_OGL_texture(TEXTUREDIR"lava.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	burntTex = SOIL_load_OGL_texture(TEXTUREDIR"burnt.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	if (!cubeMap || !earthTex || !earthBump || !lavaTex || !burntTex) {
		std::cout << "Failed to load texture!" << std::endl;
		return;
	}

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(lavaTex, true);
	SetTextureRepeating(burntTex, true);
}

void Renderer::LoadPostTextures() {
	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &processFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0]) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::LoadShadowTextures() {
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);

	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	sceneMeshes.emplace_back(Mesh::GenerateQuad());
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("pPlane1.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("dead_tree.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));
}

void Renderer::RenderScene() {
	DrawScene();
	if (fboToggle) {
		DrawPostProcess();
		PresentScene();
	}

	if (nightToggle) {
		NightScene();
	}
	else {
		light->SetColour(Vector4(1, 1, 1, 1));
		light->SetRadius(1000.0f);
		light->SetPosition(Vector3(20.0f, 60.0f, 30.0f));

		lavaPos = Vector3(50, -10, 307);
	}
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	root->Update(dt);

	lavaRotate += dt * 4.0f;
	lavaCycle += dt * 0.1f;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_P)) {
		nightToggle = !nightToggle;
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_1)) {
		fboToggle = !fboToggle;
	}

	for (int i = 1; i < 4; ++i) {
		Vector3 t = Vector3(-10 + (5 * i), 2.0f + sin(sceneTime * i), 0);
		sceneTransforms[i] = Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10 * i, Vector3(1, 0, 0));
	}
}

void Renderer::DrawScene() {
	if (fboToggle) {
		glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	}
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	DrawSkybox();
	DrawVolcano();

	vector<Vector3>treePos = root->GetAllBasePos();
	for (Vector3 &pos : treePos) {
		DrawDeadTrees(pos);
	}
	if (!nightToggle) {
		DrawNode(root);
	}
	else {
		DrawLavaPit();
	}
	DrawLava();

	//DrawShadow();
	//DrawMainScene();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);

	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeMap"), 0);
	glActiveTexture(GL_TEXTURE0);
	if (nightToggle) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap2);
	}
	else {
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	}

	UpdateShaderMatrices();
	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawVolcano() {

	BindShader(lightShader);
	SetShaderLight(*light);

	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	volcano->Draw();
}

void Renderer::DrawDeadTrees(Vector3 pos) {
	BindShader(lightShader);
	SetShaderLight(*light);

	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, burntTex);

	modelMatrix = Matrix4::Translation(Vector3(pos.x + 95, pos.y - 13, pos.z + 318));
	//modelMatrix = Matrix4::Translation(Vector3(30, 0, 307));
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	deadTree->Draw();
}

void Renderer::DrawNode(SceneNode* n) {
	BindShader(lightShader);
	SetShaderLight(*light);

	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());

		glUniformMatrix4fv(glGetUniformLocation(lightShader->GetProgram(), "modelMatrix"), 1, false, model.values);
		glUniform4fv(glGetUniformLocation(lightShader->GetProgram(), "lightColour"), 1, (float*)&n->GetColour());
		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);
		n->Draw(*this);
	}

	for (vector<SceneNode*>::const_iterator i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i) {
		DrawNode(*i);
	}
}

void Renderer::DrawLava() {
	BindShader(lightShader);
	SetShaderLight(*light);

	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lavaTex);

	modelMatrix = Matrix4::Translation(lavaPos);
	textureMatrix = Matrix4::Translation(Vector3(lavaCycle, 0.0f, lavaCycle)) * Matrix4::Scale(Vector3(1, 1, 1)) * Matrix4::Rotation(lavaRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();

	lavaTop->Draw();
}

void Renderer::DrawLavaPit() {
	BindShader(lightShader);
	SetShaderLight(*light);

	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lavaTex);

	modelMatrix = Matrix4::Translation(Vector3(-35, 7, 12)) * Matrix4::Scale(Vector3(50, 50, 50)) * Matrix4::Rotation(90, Vector3(1, 0, 0));
	textureMatrix = Matrix4::Translation(Vector3(lavaCycle, 0.0f, lavaCycle)) * Matrix4::Scale(Vector3(1, 1, 1)) * Matrix4::Rotation(lavaRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();

	quad->Draw();
}

void Renderer::NightScene() {
	light->SetColour(Vector4(1, 1, 1, 1));
	light->SetRadius(70.0f);
	light->SetPosition(Vector3(-25, 30, 20));

	lavaPos = Vector3(50, 0, 307);
}

void Renderer::DrawShadow() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1, 100, 1, 45);
	shadowMatrix = projMatrix * viewMatrix;

	for (int i = 0; i < 4; ++i) {
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawMainScene() {
	BindShader(sceneShader);
	SetShaderLight(*light);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 2);

	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	for (int i = 0; i < 4; ++i) {
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}
}

void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(postShader);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glUniform1i(glGetUniformLocation(postShader->GetProgram(), "sceneTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(postShader->GetProgram(), "isVertical"), 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
		quad->Draw();

		glUniform1i(glGetUniformLocation(postShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
		quad->Draw();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(texShader);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();

	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glUniform1i(glGetUniformLocation(texShader->GetProgram(), "diffuseTex"), 0);

	quad->Draw();
}