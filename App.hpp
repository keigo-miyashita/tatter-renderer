#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

#include <nfd.hpp>

#include "Material.hpp"
#include "EnvironmentMap.hpp"

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

enum GuiDir : int
{
	None		= 0x00000000,
	Left		= 0x00000001,
	Right		= 0x00000010,
	Up			= 0x00000100,
	Down		= 0x00001000
};

struct Light
{
	glm::vec4 pos;
	glm::vec4 color;
};

struct ModelData
{
	sqrp::MeshHandle mesh_ = nullptr;
	MaterialHandle material_ = nullptr;
};

//struct ObjectData
//{
//	sqrp::
//};

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

	sqrp::RenderPassHandle renderPass_;
	sqrp::RenderPassHandle forwardRenderPass_;
	sqrp::RenderPassHandle geometryRenderPass_;
	sqrp::RenderPassHandle lightingRenderPass_;
	sqrp::RenderPassHandle skyboxRenderPass_;
	sqrp::RenderPassHandle toneMapRenderPass_;
	sqrp::RenderPassHandle presentRenderPass_;
	sqrp::FrameBufferHandle	frameBuffer_;
	sqrp::FrameBufferHandle	forwardFrameBuffer_;
	sqrp::FrameBufferHandle	geometryFrameBuffer_;
	sqrp::FrameBufferHandle	lightingFrameBuffer_;
	sqrp::FrameBufferHandle skyboxFrameBuffer_;
	sqrp::FrameBufferHandle toneMapFrameBuffer_;
	sqrp::FrameBufferHandle presentFrameBuffer_;
	/*sqrp::MeshHandle mesh_;
	MaterialHandle material_;*/
	std::unordered_map<std::string, ModelData> models_;
	EnvMapHandle envMap_;

	sqrp::Camera camera_;
	Light light0_;
	std::vector<sqrp::Object> objects_ = {};

	sqrp::BufferHandle cameraBuffer_;
	sqrp::BufferHandle objectBuffer_;
	sqrp::BufferHandle lightBuffer_;
	sqrp::BufferHandle colorBuffer_;
	sqrp::BufferHandle detailCameraBuffer_;

	sqrp::ShaderHandle vertShader_;
	sqrp::ShaderHandle pixelShader_;
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

	sqrp::DescriptorSetHandle descriptorSet_;
	sqrp::GraphicsPipelineHandle pipeline_;

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

	bool isShowGuizmo_ = true;
	bool isModifiedRotation_ = false;
	int renderMode_ = 1; // 0: Forward, 1: G-Buffer
	ImGuizmo::OPERATION gizmoOperation_ = ImGuizmo::TRANSLATE;
	float edgeThreshold_ = 20.0f; // ÉäÉTÉCÉYñ≥å¯ïù
	std::array<int, 9> dir_ = { -1 /*None*/, 0/*Left*/, 1/*Right*/, 2/*Up*/, 3/*Down*/, 4/*UpLeft*/, 5/*UpRight*/, 6/*DownLeft*/, 7/*DownRight*/ };
	int catchSceneDir_ = -1;
	int catchPanelDir_ = -1;
	int catchFilePanelDir_ = -1;

	uint32_t sceneWidth_ = windowWidth_ * sceneViewScaleX_;
	uint32_t sceneHeight_ = windowHeight_ * sceneViewScaleY_;
	uint32_t changedSceneWidth_ = sceneWidth_;
	uint32_t changedSceneHeight_ = sceneHeight_;
	uint32_t panelWidth_ = windowWidth_ * (1.0f - sceneViewScaleX_);
	uint32_t panelHeight_ = windowHeight_;
	uint32_t changedPanelWidth_ = panelWidth_;
	uint32_t changedPanelHeight_ = panelHeight_;
	uint32_t filePanelWidth_ = windowWidth_ * sceneViewScaleX_;
	uint32_t filePanelHeight_ = windowHeight_ * (1.0f - sceneViewScaleY_);
	uint32_t changedFilePanelWidth_ = filePanelWidth_;
	uint32_t changedFilePanelHeight_ = filePanelHeight_;

	void Recreate();

public:
	App(std::string appName = "tatter-renderer", unsigned int windowWidth = 1920, unsigned int windowHeight = 1080);
	~App() = default;

	virtual void OnStart() override;
	virtual void OnUpdate() override;
	virtual void OnResize(unsigned int width, unsigned int height) override;
	virtual void OnTerminate() override;
};