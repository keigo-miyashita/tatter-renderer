#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

#include <nfd.hpp>

enum GuiEdge : int
{
	None = 0x00000000,
	Left = 0x00000001,
	Right = 0x00000010,
	Up = 0x00000100,
	Down = 0x00001000
};

struct GuiWindowSize {
	uint32_t width;
	uint32_t height;
	uint32_t changedWidth;
	uint32_t changedHeight;
};

class App;

class GuiManager
{
private:
	App* app_;

	sqrp::GUIHandle gui_;

	sqrp::ImageHandle translateIconImage_ = nullptr;
	sqrp::ImageHandle rotationIconImage_ = nullptr;
	sqrp::ImageHandle scaleIconImage_ = nullptr;

	std::vector<sqrp::DescriptorSetHandle> guiDescriptorSets_;
	sqrp::DescriptorSetHandle sceneViewDescSet_ = nullptr;
	sqrp::DescriptorSetHandle translateIconDescSet_ = nullptr;
	sqrp::DescriptorSetHandle rotationIconDescSet_ = nullptr;
	sqrp::DescriptorSetHandle scaleIconDescSet_ = nullptr;

	float sceneViewScaleX_ = 0.8f;
	float sceneViewScaleY_ = 0.7f;

	bool isChangedSceneSize_ = false;
	bool isNeedRecreate_ = false;
	bool isNeedReloadModel_ = false;
	bool isNeedReloadEnvMap_ = false;
	bool isChangedEnvMap_ = false;
	std::string newModelPath_ = "";
	std::string newEnvMapPath_ = "";
	std::string addedModelName_ = "";
	std::string deletedModelName_ = "";
	int selectedObjectIndex_ = 0;
	std::string selectedObjectName_ = "";
	int selectedEnvMapIndex_ = 0;
	std::string selectedEnvMapName_ = "";
	bool isShowGuizmo_ = true;
	bool isModifiedRotation_ = false;
	int renderMode_ = 1; // 0: Forward, 1: G-Buffer
	ImGuizmo::OPERATION gizmoOperation_ = ImGuizmo::TRANSLATE;
	float edgeThreshold_ = 20.0f; // ÉäÉTÉCÉYñ≥å¯ïù
	std::array<int, 9> dir_ = { -1 /*None*/, 0/*Left*/, 1/*Right*/, 2/*Up*/, 3/*Down*/, 4/*UpLeft*/, 5/*UpRight*/, 6/*DownLeft*/, 7/*DownRight*/ };
	int catchedSceneEdge_ = -1;
	int catchedInspectorDir_ = -1;
	int catchedAssetDir_ = -1;

	GuiWindowSize sceneViewSize_;
	GuiWindowSize inspectorViewSize_;
	GuiWindowSize assetViewSize_;

	sqrp::ImageHandle CreateIcon(std::string path);
	void DefineGUIStyle();

public:
	GuiManager(App* app);
	~GuiManager() = default;

	void DrawGui();
	void UpdateGUISize();
	void Recreate();

	sqrp::GUIHandle GetGui();
	GuiWindowSize GetSceneViewSize();
	GuiWindowSize GetInspectorViewSize();
	GuiWindowSize GetAssetViewSize();
	int GetSelectedObjectIndex();
	std::string GetSelectedObjectName();
	std::string GetSelectedEnvMapName();
	bool IsChangedSceneSize();
	bool IsNeedRecreate();
	bool IsNeedReloadModel();
	bool IsNeedReloadEnvMap();
	bool IsChangedEnvMap();
	int GetRenderMode();
	std::string GetNewModelPath();
	std::string GetNewEnvMapPath();
	std::string GetAddedModelName();
	std::string GetDeletedModelName();

	void SetSelectedObjectIndex(int index);
	void SetSelectedObjectName(std::string name);
	void SetSelectedEnvMapName(std::string name);
	void SetIsChangedEnvMap(bool isChanged);
};

using GuiManagerHandle = std::shared_ptr<GuiManager>;