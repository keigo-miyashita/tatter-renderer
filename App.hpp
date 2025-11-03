#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

#include <nfd.hpp>

#include "EnvironmentMap.hpp"
#include "GuiManager.hpp"
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

class GuiManager;

class App : public sqrp::Application
{
	friend class GuiManager;
private:
	sqrp::Device device_;
	sqrp::Compiler compiler_;

	//sqrp::GUIHandle gui_;

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

	// string : Object name, vector : mesh index, vector : primitive index, vector : DescriptorSet per inflight
	std::unordered_map<std::string, std::vector<std::vector<std::vector<sqrp::DescriptorSetHandle>>>> forwardDescriptorSets_;
	sqrp::GraphicsPipelineHandle forwardPipeline_;
	std::unordered_map<std::string, std::vector<std::vector<std::vector<sqrp::DescriptorSetHandle>>>> geomDescriptorSets_;
	sqrp::GraphicsPipelineHandle geomPipeline_;
	std::unordered_map<std::string, std::vector<std::vector<std::vector<sqrp::DescriptorSetHandle>>>> lightDescriptorSets_;
	sqrp::GraphicsPipelineHandle lightPipeline_;

	std::vector<sqrp::DescriptorSetHandle> skyboxDescriptorSets_;
	sqrp::GraphicsPipelineHandle skyboxPipeline_;

	std::vector<sqrp::DescriptorSetHandle> toneMapDescriptorSets_;
	sqrp::GraphicsPipelineHandle toneMapPipeline_;

	GuiManagerHandle guiManager_;

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