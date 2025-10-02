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
	sqrp::SwapchainHandle swapchain_;
	sqrp::RenderPassHandle renderPass_;
	sqrp::FrameBufferHandle	frameBuffer_;
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

	sqrp::DescriptorSetHandle descriptorSet_;
	sqrp::PipelineHandle pipeline_;


public:
	App(std::string appName = "tatter-renderer", unsigned int windowWidth = 1280, unsigned int windowHeight = 720);
	~App() = default;

	virtual void OnStart() override;
	virtual void OnUpdate() override;
	virtual void OnTerminate() override;
};