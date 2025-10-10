#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

struct Light
{
	glm::vec4 pos;
	glm::vec4 color;
};

class App : public sqrp::Application
{
private:
	sqrp::Device device_;
	sqrp::Compiler compiler_;

	sqrp::GUIHandle gui_;

	sqrp::SwapchainHandle swapchain_;
	sqrp::RenderPassHandle renderPass_;
	sqrp::RenderPassHandle geometryRenderPass_;
	sqrp::RenderPassHandle lightingRenderPass_;
	sqrp::RenderPassHandle presentRenderPass_;
	sqrp::FrameBufferHandle	frameBuffer_;
	sqrp::FrameBufferHandle	geometryFrameBuffer_;
	sqrp::FrameBufferHandle	lightingFrameBuffer_;
	sqrp::FrameBufferHandle presentFrameBuffer_;
	sqrp::MeshHandle mesh_;

	sqrp::Camera camera_;
	Light light0_;
	sqrp::TransformMatrix object_;

	sqrp::BufferHandle cameraBuffer_;
	sqrp::BufferHandle objectBuffer_;
	sqrp::BufferHandle lightBuffer_;
	sqrp::BufferHandle colorBuffer_;

	sqrp::ShaderHandle vertShader_;
	sqrp::ShaderHandle pixelShader_;
	sqrp::ShaderHandle geomVertShader_;
	sqrp::ShaderHandle geomPixelShader_;
	sqrp::ShaderHandle lightVertShader_;
	sqrp::ShaderHandle lightPixelShader_;

	sqrp::DescriptorSetHandle descriptorSet_;
	sqrp::PipelineHandle pipeline_;

	std::vector<sqrp::DescriptorSetHandle> guiDescriptorSets_;

	std::vector<sqrp::DescriptorSetHandle> geometryDescriptorSets_;
	sqrp::PipelineHandle geometryPipeline_;
	std::vector<sqrp::DescriptorSetHandle> lightingDescriptorSets_;
	sqrp::PipelineHandle lightingPipeline_;

	float sceneViewScaleX_ = 0.8f;
	float sceneViewScaleY_ = 0.7f;

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


public:
	App(std::string appName = "tatter-renderer", unsigned int windowWidth = 1920, unsigned int windowHeight = 1080);
	~App() = default;

	virtual void OnStart() override;
	virtual void OnUpdate() override;
	virtual void OnResize(unsigned int width, unsigned int height) override;
	virtual void OnTerminate() override;
};