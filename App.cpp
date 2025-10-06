#include "App.hpp"

using namespace std;
using namespace sqrp;

App::App(std::string appName, unsigned int windowWidth, unsigned int windowHeight)
	: Application(appName, windowWidth, windowHeight)
{

}

void App::OnStart()
{
	device_.Init(*this);

	swapchain_ = device_.CreateSwapchain(windowWidth_, windowHeight_);

	renderPass_ = device_.CreateRenderPass(swapchain_);
	std::array<vk::Format, 3> gbufferFormats = {
		vk::Format::eR8G8B8A8Unorm,     // BaseColor + Metalness
		vk::Format::eR16G16B16A16Sfloat, // Normal + Roughness
		vk::Format::eR8G8B8A8Unorm      // Emissive + AO
	};
	std::map<string, AttachmentInfo> geometryAttachmentNameToInfo;
	AttachmentInfo gbuffer0; // BaseColor + Metalness
	gbuffer0.attachmentDesc
		.setFormat(swapchain_->GetSurfaceFormat())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
	gbuffer0.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	geometryAttachmentNameToInfo["BaseColorMetallness"] = gbuffer0;

	//AttachmentInfo gbuffer1; // Normal + Roughness
	//gbuffer1 = gbuffer0;
	//gbuffer1.attachmentDesc.format = vk::Format::eR16G16B16A16Sfloat;
	//gbuffer1.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;

	//AttachmentInfo gbuffer2; // Emissive + AO
	//gbuffer2 = gbuffer0;
	//gbuffer2.attachmentDesc.format = vk::Format::eR8G8B8A8Unorm;
	//gbuffer2.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;

	AttachmentInfo depthAttachment;
	depthAttachment.attachmentDesc
		.setFormat(vk::Format::eD32Sfloat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal);
	depthAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	geometryAttachmentNameToInfo["Depth"] = depthAttachment;

	SubPassInfo geometrySubpass;
	geometrySubpass.attachmentInfos = {"BaseColorMetallness", "Depth"};
	//geometrySubpass.attachmentInfos = { gbuffer0, /*&gbuffer1, &gbuffer2,*/ depthAttachment };

	geometryRenderPass_ = device_.CreateRenderPass(
		{ geometrySubpass }, geometryAttachmentNameToInfo
	);

	std::map<string, AttachmentInfo> lightingAttachmentNameToInfo;
	AttachmentInfo swapchainAttachment;
	swapchainAttachment.attachmentDesc
		.setFormat(swapchain_->GetSurfaceFormat())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
	swapchainAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	lightingAttachmentNameToInfo["Swapchain"] = swapchainAttachment;	

	SubPassInfo lightingSubpass;
	lightingSubpass.attachmentInfos = { "Swapchain"};

	lightingRenderPass_ = device_.CreateRenderPass(
		{ lightingSubpass }, lightingAttachmentNameToInfo
	);

	frameBuffer_ = device_.CreateFrameBuffer(renderPass_, swapchain_);

	geometryFrameBuffer_ = device_.CreateFrameBuffer(
		geometryRenderPass_,
		{
			//{ gbufferFormats[0], vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, "BaseColorMetallness"},
			//{ gbufferFormats[1], vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, "NormalRoughness"},
			//{ gbufferFormats[2], vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, "EmissiveAO"},
			{ vk::Format::eD32Sfloat, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, "Depth"}
		},
		swapchain_->GetWidth(),
		swapchain_->GetHeight(),
		swapchain_->GetInflightCount(),
		swapchain_
	);

	lightingFrameBuffer_ = device_.CreateFrameBuffer(
		lightingRenderPass_,
		swapchain_,
		false
	);

	mesh_ = device_.CreateMesh(string(MODEL_DIR) + "Suzanne.gltf");

	// Camera
	camera_.Init((float)GetWindowWidth() / (float)GetWindowHeight(), glm::vec3(0.0f, 0.0f, 5.0f)); // Note sign

	// Light
	light0_.pos = glm::vec4(10.0f, 10.0f, -5.0f, 1.0f);
	light0_.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Sphere0
	object_ = {};
	/*XMMATRIX modelMat = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(180)), XMMatrixIdentity());
	sphere0_.model = modelMat;
	sphere0_.invTransModel = XMMatrixTranspose(XMMatrixInverse(nullptr, sphere0_.model));*/

	cameraBuffer_ = device_.CreateBuffer(sizeof(CameraMatrix), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });

	objectBuffer_ = device_.CreateBuffer(sizeof(TransformMatrix), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	objectBuffer_->Write(object_);

	lightBuffer_ = device_.CreateBuffer(sizeof(Light), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	lightBuffer_->Write(light0_);

	colorBuffer_ = device_.CreateBuffer(sizeof(glm::vec4), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	colorBuffer_->Write(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	descriptorSet_ = device_.CreateDescriptorSet({
		{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
		{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
		{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
		{ colorBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment }
		}
	);

	vertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "Lambert.shader", sqrp::ShaderType::Vertex);
	pixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "Lambert.shader", sqrp::ShaderType::Pixel);

	geomVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "geometrypass.shader", sqrp::ShaderType::Vertex);
	geomPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "geometrypass.shader", sqrp::ShaderType::Pixel);
	lightVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "lightingpass.shader", sqrp::ShaderType::Vertex);
	lightPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "lightingpass.shader", sqrp::ShaderType::Pixel);

	//pipeline_ = device_.CreatePipeline(renderPass_, swapchain_, vertShader_, pixelShader_, descriptorSet_);
	pipeline_ = device_.CreatePipeline(renderPass_, swapchain_, geomVertShader_, geomPixelShader_, descriptorSet_);
	
	geometryDescriptorSets_.resize(swapchain_->GetInflightCount());
	lightingDescriptorSets_.resize(swapchain_->GetInflightCount());
	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		geometryDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ colorBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment }
			}
		);

		/*lightingDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ geometryFrameBuffer_->GetAttachmentImage(0, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ geometryFrameBuffer_->GetAttachmentImage(1, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ geometryFrameBuffer_->GetAttachmentImage(2, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			}
		);*/
	}

	geometryPipeline_ = device_.CreatePipeline(geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_, geometryDescriptorSets_[0]);
	//lightingPipeline_ = device_.CreatePipeline(lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_, lightingDescriptorSets_[0]);
}

void App::OnUpdate()
{
	camera_.Update();
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });

	swapchain_->WaitFrame();

	//cout << "test" << endl;
	uint32_t infligtIndex = swapchain_->GetCurrentInflightIndex();

	auto& commandBuffer = swapchain_->GetCurrentCommandBuffer();

	commandBuffer->Begin();

	/*commandBuffer->BeginRenderPass(renderPass_, frameBuffer_);

	commandBuffer->SetViewport(swapchain_->GetWidth(), swapchain_->GetHeight());
	commandBuffer->SetScissor(swapchain_->GetWidth(), swapchain_->GetHeight());
	commandBuffer->BindPipeline(pipeline_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindDescriptorSet(pipeline_, descriptorSet_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindMeshBuffer(mesh_);
	commandBuffer->DrawMesh(mesh_);

	commandBuffer->EndRenderPass();*/

	/*commandBuffer->BeginRenderPass(renderPass_, frameBuffer_, swapchain_);

	commandBuffer->SetViewport(swapchain_->GetWidth(), swapchain_->GetHeight());
	commandBuffer->SetScissor(swapchain_->GetWidth(), swapchain_->GetHeight());
	commandBuffer->BindPipeline(pipeline_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindDescriptorSet(pipeline_, descriptorSet_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindMeshBuffer(mesh_);
	commandBuffer->DrawMesh(mesh_);

	commandBuffer->EndRenderPass();*/

	/*commandBuffer->TransitionLayout(geometryFrameBuffer_->GetAttachmentImage(0, infligtIndex), vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
	commandBuffer->TransitionLayout(geometryFrameBuffer_->GetAttachmentImage(1, infligtIndex), vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
	commandBuffer->TransitionLayout(geometryFrameBuffer_->GetAttachmentImage(2, infligtIndex), vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);*/

	commandBuffer->BeginRenderPass(geometryRenderPass_, geometryFrameBuffer_, swapchain_);

	commandBuffer->SetViewport(swapchain_->GetWidth(), swapchain_->GetHeight());
	commandBuffer->SetScissor(swapchain_->GetWidth(), swapchain_->GetHeight());
	commandBuffer->BindPipeline(geometryPipeline_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindDescriptorSet(geometryPipeline_, geometryDescriptorSets_[infligtIndex], vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindMeshBuffer(mesh_);
	commandBuffer->DrawMesh(mesh_);

	commandBuffer->EndRenderPass();

	/*commandBuffer->TransitionLayout(geometryFrameBuffer_->GetAttachmentImage(0, infligtIndex), vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	commandBuffer->TransitionLayout(geometryFrameBuffer_->GetAttachmentImage(1, infligtIndex), vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	commandBuffer->TransitionLayout(geometryFrameBuffer_->GetAttachmentImage(2, infligtIndex), vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);*/

	//commandBuffer->BeginRenderPass(lightingRenderPass_, lightingFrameBuffer_, swapchain_);

	//commandBuffer->SetViewport(swapchain_->GetWidth(), swapchain_->GetHeight());
	//commandBuffer->SetScissor(swapchain_->GetWidth(), swapchain_->GetHeight());
	//commandBuffer->BindPipeline(lightingPipeline_, vk::PipelineBindPoint::eGraphics);
	//commandBuffer->BindDescriptorSet(lightingPipeline_, lightingDescriptorSets_[infligtIndex], vk::PipelineBindPoint::eGraphics);
	////commandBuffer->BindMeshBuffer(mesh_);
	////commandBuffer->DrawMesh(mesh_);
	//commandBuffer->Draw(3, 1); // Fullscreen Triangle

	//commandBuffer->EndRenderPass();

	commandBuffer->End();

	device_.Submit(
		QueueContextType::General, commandBuffer, vk::PipelineStageFlagBits::eColorAttachmentOutput,
		swapchain_->GetImageAcquireSemaphore(), swapchain_->GetRenderCompleteSemaphore(), swapchain_->GetCurrentFence()
	);

	swapchain_->Present();
}

void App::OnTerminate()
{
	device_.WaitIdle(QueueContextType::General);
}