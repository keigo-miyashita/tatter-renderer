#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

#include <nfd.hpp>

#include "EnvironmentMap.hpp"
#include "Material.hpp"
#include "Scene.hpp"

struct DetailCamera
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 invView;
	glm::mat4 invProj;
	glm::vec4 pos;
	float nearZ;
	float farZ;
};

struct Light
{
	glm::vec4 pos;
	glm::vec4 color;
};

enum GuiDir : int
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

class App : public sqrp::Application
{
private:
	sqrp::Device device_;
	sqrp::Compiler compiler_;

	sqrp::GUIHandle gui_;

	sqrp::SwapchainHandle swapchain_;

	std::vector<sqrp::ImageHandle> baseColorMetallnessImages_;
	std::vector<sqrp::ImageHandle> normalRoughnessImages_;
	std::vector<sqrp::ImageHandle> emissiveAOImages_;
	std::vector<sqrp::ImageHandle> depthImages_;
	std::vector<sqrp::ImageHandle> renderImages_;
	std::vector<sqrp::ImageHandle> toneMappedImages_;

	sqrp::RenderPassHandle forwardRenderPass_;
	sqrp::RenderPassHandle geometryRenderPass_;
	sqrp::RenderPassHandle lightingRenderPass_;
	sqrp::RenderPassHandle skyboxRenderPass_;
	sqrp::RenderPassHandle toneMapRenderPass_;
	sqrp::RenderPassHandle presentRenderPass_;
	sqrp::FrameBufferHandle	forwardFrameBuffer_;
	sqrp::FrameBufferHandle	geometryFrameBuffer_;
	sqrp::FrameBufferHandle	lightingFrameBuffer_;
	sqrp::FrameBufferHandle skyboxFrameBuffer_;
	sqrp::FrameBufferHandle toneMapFrameBuffer_;
	sqrp::FrameBufferHandle presentFrameBuffer_;
	std::unordered_map<std::string, std::shared_ptr<ModelData>> models_;
	std::unordered_map<std::string, EnvironmentMapHandle> envMaps_;

	std::unordered_map<std::string, std::shared_ptr<ObjectData>> objectData_;

	sqrp::Camera camera_;
	Light light0_;
	std::vector<std::string> objectNames_ = {};
	std::vector<std::string> envMapNames_ = {};

	sqrp::BufferHandle cameraBuffer_;
	sqrp::BufferHandle lightBuffer_;
	sqrp::BufferHandle colorBuffer_;
	sqrp::BufferHandle detailCameraBuffer_;

	sqrp::ShaderHandle forwardVertShader_;
	sqrp::ShaderHandle forwardPixelShader_;
	sqrp::ShaderHandle geomVertShader_;
	sqrp::ShaderHandle geomPixelShader_;
	sqrp::ShaderHandle lightVertShader_;
	sqrp::ShaderHandle lightPixelShader_;
	sqrp::ShaderHandle envMapCompShader_;
	sqrp::ShaderHandle irradianceCompShader_;
	sqrp::ShaderHandle prefilterCompShader_;
	sqrp::ShaderHandle brdfLUTCompShader_;
	sqrp::ShaderHandle skyboxVertShader_;
	sqrp::ShaderHandle skyboxPixelShader_;
	sqrp::ShaderHandle toneMapVertShader_;
	sqrp::ShaderHandle toneMapPixelShader_;

	std::unordered_map<std::string, std::vector<sqrp::DescriptorSetHandle>> forwardDescriptorSets_;
	sqrp::GraphicsPipelineHandle forwardPipeline_;
	std::unordered_map<std::string, std::vector<sqrp::DescriptorSetHandle>> geomDescriptorSets_;
	sqrp::GraphicsPipelineHandle geomPipeline_;
	std::unordered_map<std::string, std::vector<sqrp::DescriptorSetHandle>> lightDescriptorSets_;
	sqrp::GraphicsPipelineHandle lightPipeline_;

	std::vector<sqrp::DescriptorSetHandle> skyboxDescriptorSets_;
	sqrp::GraphicsPipelineHandle skyboxPipeline_;

	std::vector<sqrp::DescriptorSetHandle> toneMapDescriptorSets_;
	sqrp::GraphicsPipelineHandle toneMapPipeline_;

	std::vector<sqrp::DescriptorSetHandle> guiDescriptorSets_;

	float sceneViewScaleX_ = 0.8f;
	float sceneViewScaleY_ = 0.7f;

	bool isNeedRecreate_ = false;
	bool isNeedReloadModel_ = false;
	bool isNeedReloadEnvMap_ = false;
	bool isChangedEnvMap_ = false;
	std::string newModelPath_ = "";
	std::string newEnvMapPath_ = "";
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
	int catchSceneDir_ = -1;
	int catchPanelDir_ = -1;
	int catchFilePanelDir_ = -1;

	GuiWindowSize sceneViewSize_ = {
		(uint32_t)(windowWidth_ * sceneViewScaleX_),
		(uint32_t)(windowHeight_ * sceneViewScaleY_),
		(uint32_t)(windowWidth_ * sceneViewScaleX_),
		(uint32_t)(windowHeight_ * sceneViewScaleY_)
	};
	GuiWindowSize inspectorViewSize_ = {
		(uint32_t)(windowWidth_ * (1.0f - sceneViewScaleX_)),
		(uint32_t)(windowHeight_),
		(uint32_t)(windowWidth_ * (1.0f - sceneViewScaleX_)),
		(uint32_t)(windowHeight_)
	};
	GuiWindowSize filePanelSize_ = {
		(uint32_t)(windowWidth_ * sceneViewScaleX_),
		(uint32_t)(windowHeight_ * (1.0f - sceneViewScaleY_)),
		(uint32_t)(windowWidth_ * sceneViewScaleX_),
		(uint32_t)(windowHeight_ * (1.0f - sceneViewScaleY_))
	};
	uint32_t inspectorHeight_ = windowHeight_ * 0.1f;

	sqrp::ImageHandle potisionIconImage_;
	sqrp::DescriptorSetHandle potisionIconDescSet_;
	sqrp::ImageHandle rotationIconImage_;
	sqrp::DescriptorSetHandle rotationIconDescSet_;
	sqrp::ImageHandle scaleIconImage_;
	sqrp::DescriptorSetHandle scaleIconDescSet_;

	sqrp::ImageHandle CreateIcon(std::string path);
	void DefineGUIStyle();
	void Recreate();

public:
	App(std::string appName = "tatter-renderer", unsigned int windowWidth = 1920, unsigned int windowHeight = 1080);
	~App() = default;

	virtual void OnStart() override;
	virtual void OnUpdate() override;
	virtual void OnResize(unsigned int width, unsigned int height) override;
	virtual void OnTerminate() override;
};