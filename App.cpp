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
		.setFormat(gbufferFormats[0])
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	gbuffer0.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	geometryAttachmentNameToInfo["BaseColorMetallness"] = gbuffer0;

	AttachmentInfo gbuffer1; // Normal + Roughness
	gbuffer1 = gbuffer0;
	gbuffer1.attachmentDesc.format = gbufferFormats[1];
	geometryAttachmentNameToInfo["NormalRoughness"] = gbuffer1;

	AttachmentInfo gbuffer2; // Emissive + AO
	gbuffer2 = gbuffer0;
	gbuffer2.attachmentDesc.format = gbufferFormats[2];
	geometryAttachmentNameToInfo["EmissiveAO"] = gbuffer2;

	AttachmentInfo depthAttachment;
	depthAttachment.attachmentDesc
		.setFormat(vk::Format::eD32Sfloat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	depthAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	geometryAttachmentNameToInfo["Depth"] = depthAttachment;

	SubPassInfo geometrySubpass;
	geometrySubpass.attachmentInfos = {"BaseColorMetallness", "NormalRoughness", "EmissiveAO",  "Depth"};

	geometryRenderPass_ = device_.CreateRenderPass(
		{ geometrySubpass }, geometryAttachmentNameToInfo
	);

	std::map<string, AttachmentInfo> lightingAttachmentNameToInfo;
	AttachmentInfo renderingResultAttachment;
	renderingResultAttachment.attachmentDesc
		.setFormat(swapchain_->GetSurfaceFormat())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	renderingResultAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	lightingAttachmentNameToInfo["RenderingResult"] = renderingResultAttachment;

	SubPassInfo lightingSubpass;
	lightingSubpass.attachmentInfos = { "RenderingResult"};

	lightingRenderPass_ = device_.CreateRenderPass(
		{ lightingSubpass }, lightingAttachmentNameToInfo
	);

	std::map<string, AttachmentInfo> presentAttachmentNameToInfo;
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
	presentAttachmentNameToInfo["Swapchain"] = swapchainAttachment;

	SubPassInfo presentSubpass;
	presentSubpass.attachmentInfos = { "Swapchain" };

	presentRenderPass_ = device_.CreateRenderPass(
		{ presentSubpass }, presentAttachmentNameToInfo
	);

	frameBuffer_ = device_.CreateFrameBuffer(renderPass_, swapchain_);

	geometryFrameBuffer_ = device_.CreateFrameBuffer(
		geometryRenderPass_,
		{
			{ gbufferFormats[0], vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, "BaseColorMetallness"},
			{ gbufferFormats[1], vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, "NormalRoughness"},
			{ gbufferFormats[2], vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, "EmissiveAO"},
			{ vk::Format::eD32Sfloat, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, "Depth"}
		},
		sceneWidth_,
		sceneHeight_,
		swapchain_->GetInflightCount()
	);

	lightingFrameBuffer_ = device_.CreateFrameBuffer(
		lightingRenderPass_,
		{
			{ swapchain_->GetSurfaceFormat(), vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, "RenderingResult"},
		},
		sceneWidth_,
		sceneHeight_,
		swapchain_->GetInflightCount()
	);

	presentFrameBuffer_ = device_.CreateFrameBuffer(
		presentRenderPass_,
		swapchain_,
		false
	);

	mesh_ = device_.CreateMesh(string(MODEL_DIR) + "Suzanne.gltf");

	// Camera
	camera_.Init((float)sceneWidth_ / (float)sceneHeight_, glm::vec3(0.0f, 0.0f, 5.0f)); // Note sign

	// Light
	light0_.pos = glm::vec4(10.0f, 10.0f, -5.0f, 1.0f);
	light0_.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Sphere0
	object_ = {};

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

	pipeline_ = device_.CreatePipeline(renderPass_, swapchain_, vertShader_, pixelShader_, descriptorSet_);
	
	geometryDescriptorSets_.resize(swapchain_->GetInflightCount());
	lightingDescriptorSets_.resize(swapchain_->GetInflightCount());
	guiDescriptorSets_.resize(swapchain_->GetInflightCount());
	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		geometryDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ colorBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment }
			}
		);

		lightingDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ colorBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ geometryFrameBuffer_->GetAttachmentImage(0, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ geometryFrameBuffer_->GetAttachmentImage(1, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ geometryFrameBuffer_->GetAttachmentImage(2, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			}
		);

		guiDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ lightingFrameBuffer_->GetAttachmentImage(0, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment },
			}
		);
	}

	geometryPipeline_ = device_.CreatePipeline(geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_, geometryDescriptorSets_[0]);
	lightingPipeline_ = device_.CreatePipeline(lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_, lightingDescriptorSets_[0]);

	gui_ = device_.CreateGUI(pWindow_, swapchain_, presentRenderPass_);

	panelWidth_ = windowWidth_ - sceneWidth_;
	panelHeight_ = windowHeight_;
	filePanelWidth_ = sceneWidth_;
	filePanelHeight_ = windowHeight_ - sceneHeight_;
	changedSceneWidth_ = sceneWidth_;
	changedSceneHeight_ = sceneHeight_;
	changedPanelWidth_ = panelWidth_;
	changedPanelHeight_ = panelHeight_;
	changedFilePanelWidth_ = filePanelWidth_;
	changedFilePanelHeight_ = filePanelHeight_;
}

void App::OnUpdate()
{
	bool isChangedSceneSize = false;
	if (changedSceneWidth_ != sceneWidth_ && !isChangedSceneSize) {
		sceneWidth_ = changedSceneWidth_;
		panelWidth_ = windowWidth_ - sceneWidth_;
		filePanelWidth_ = sceneWidth_;
		cout << "Change scene width" << endl;
		isChangedSceneSize = true;
	}
	if (changedSceneHeight_ != sceneHeight_ && !isChangedSceneSize) {
		cout << "Scene height = " << sceneHeight_ << endl;
		cout << "changedSceneHeight_ = " << changedSceneHeight_ << endl;
		sceneHeight_ = changedSceneHeight_;
		filePanelHeight_ = windowHeight_ - sceneHeight_;
		cout << "Scene height = " << sceneHeight_ << endl;
		isChangedSceneSize = true;
	}
	if (changedPanelWidth_ != panelWidth_ && !isChangedSceneSize) {
		panelWidth_ = changedPanelWidth_;
		sceneWidth_ = windowWidth_ - panelWidth_;
		filePanelWidth_ = sceneWidth_;
		filePanelHeight_ = windowHeight_ - sceneHeight_;
		cout << "change panel" << endl;
		isChangedSceneSize = true;
	}
	if (changedFilePanelHeight_ != filePanelHeight_ && !isChangedSceneSize) {
		cout << "change file panel" << endl;
		filePanelHeight_ = changedFilePanelHeight_;
		sceneHeight_ = windowHeight_ - filePanelHeight_;
		sceneWidth_ = filePanelWidth_;
		panelWidth_ = windowWidth_ - sceneWidth_;
		isChangedSceneSize = true;
	}
	if (changedFilePanelWidth_ != filePanelWidth_ && !isChangedSceneSize) {
		cout << "change file panel" << endl;
		filePanelWidth_ = changedFilePanelWidth_;
		sceneWidth_ = filePanelWidth_;
		panelWidth_ = windowWidth_ - sceneWidth_;
		isChangedSceneSize = true;
	}

	if (isChangedSceneSize) {

		device_.WaitIdle(QueueContextType::General);
		swapchain_->Recreate(windowWidth_, windowHeight_);
		geometryFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
		lightingFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
		presentFrameBuffer_->Recreate(windowWidth_, windowHeight_);

		geometryDescriptorSets_.clear();
		lightingDescriptorSets_.clear();
		guiDescriptorSets_.clear();
		geometryDescriptorSets_.resize(swapchain_->GetInflightCount());
		lightingDescriptorSets_.resize(swapchain_->GetInflightCount());
		guiDescriptorSets_.resize(swapchain_->GetInflightCount());
		for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
			geometryDescriptorSets_[i] = device_.CreateDescriptorSet({
				{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ colorBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment }
				}
			);

			lightingDescriptorSets_[i] = device_.CreateDescriptorSet({
				{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ colorBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ geometryFrameBuffer_->GetAttachmentImage(0, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ geometryFrameBuffer_->GetAttachmentImage(1, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ geometryFrameBuffer_->GetAttachmentImage(2, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				}
				);

			guiDescriptorSets_[i] = device_.CreateDescriptorSet({
				{ lightingFrameBuffer_->GetAttachmentImage(0, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment },
				}
				);
		}

		geometryPipeline_.reset();
		lightingPipeline_.reset();

		geometryPipeline_ = device_.CreatePipeline(geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_, geometryDescriptorSets_[0]);
		lightingPipeline_ = device_.CreatePipeline(lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_, lightingDescriptorSets_[0]);
	}

	camera_.Update(sceneWidth_, sceneHeight_);
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });

	swapchain_->WaitFrame();

	uint32_t infligtIndex = swapchain_->GetCurrentInflightIndex();

	auto& commandBuffer = swapchain_->GetCurrentCommandBuffer();

	commandBuffer->Begin();

	commandBuffer->BeginRenderPass(geometryRenderPass_, geometryFrameBuffer_, infligtIndex);
	commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
	commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
	commandBuffer->BindPipeline(geometryPipeline_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindDescriptorSet(geometryPipeline_, geometryDescriptorSets_[infligtIndex], vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindMeshBuffer(mesh_);
	commandBuffer->DrawMesh(mesh_);
	commandBuffer->EndRenderPass();

	commandBuffer->ImageBarrier(
		geometryFrameBuffer_->GetAttachmentImage(0, infligtIndex),
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::AccessFlagBits::eShaderRead
	);
	commandBuffer->ImageBarrier(
		geometryFrameBuffer_->GetAttachmentImage(1, infligtIndex),
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::AccessFlagBits::eShaderRead
	);
	commandBuffer->ImageBarrier(
		geometryFrameBuffer_->GetAttachmentImage(2, infligtIndex),
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::AccessFlagBits::eShaderRead
	);

	commandBuffer->BeginRenderPass(lightingRenderPass_, lightingFrameBuffer_, infligtIndex);
	commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
	commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
	commandBuffer->BindPipeline(lightingPipeline_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindDescriptorSet(lightingPipeline_, lightingDescriptorSets_[infligtIndex], vk::PipelineBindPoint::eGraphics);
	commandBuffer->Draw(3, 1); // Fullscreen Triangle
	commandBuffer->EndRenderPass();

	commandBuffer->BeginRenderPass(presentRenderPass_, presentFrameBuffer_, infligtIndex);
	commandBuffer->SetViewport(swapchain_->GetWidth(), swapchain_->GetHeight());
	commandBuffer->SetScissor(swapchain_->GetWidth(), swapchain_->GetHeight());

	gui_->NewFrame();

	ImGuiIO& io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(sceneWidth_, sceneHeight_));
	ImGui::Begin("Scene View", nullptr);
	ImVec2 size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
		Input::SetCatchInput(true);
	}
	changedSceneWidth_ = size.x;
	changedSceneHeight_ = size.y;
	ImTextureID texId = (ImTextureID)((VkDescriptorSet)guiDescriptorSets_[infligtIndex]->GetDescriptorSet());
	ImGui::Image(texId, ImVec2(sceneWidth_, sceneHeight_)); // シーンテクスチャ表示
	ImGui::End();

	// 2. ツールパネル描画（横に並べる）
	float position[] = {0.2f, 0.2f, 0.2f};
	float rotation[] = {0.2f, 0.2f, 0.2f};
	ImGui::SetNextWindowPos(ImVec2(sceneWidth_, 0));
	ImGui::SetNextWindowSize(ImVec2(panelWidth_, windowHeight_));
	ImGui::Begin("Inspector", nullptr);
	size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
		cout << "hovered panel" << endl;
		Input::SetCatchInput(false);
	}
	changedPanelWidth_ = size.x;
	changedPanelHeight_ = size.y;
	ImGui::Text("Transform");
	ImGui::SliderFloat3("Position", position, -10.0f, 10.0f);
	ImGui::SliderFloat3("Rotation", rotation, -180.0f, 180.0f);
	ImGui::End();

	// 3. ファイルツールパネル描画（縦に並べる）
	ImGui::SetNextWindowPos(ImVec2(0, sceneHeight_));
	ImGui::SetNextWindowSize(ImVec2(filePanelWidth_, filePanelHeight_));
	ImGui::Begin("Test", nullptr);
	size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
		cout << "hovered file panel" << endl;
		Input::SetCatchInput(false);
	}
	changedFilePanelWidth_ = size.x;
	changedFilePanelHeight_ = size.y;
	ImGui::Text("Test Transform");
	ImGui::SliderFloat3("Position", position, -10.0f, 10.0f);
	ImGui::SliderFloat3("Rotation", rotation, -180.0f, 180.0f);
	ImGui::End();

	commandBuffer->DrawGui(*gui_);
	commandBuffer->EndRenderPass();

	commandBuffer->End();

	device_.Submit(
		QueueContextType::General, commandBuffer, vk::PipelineStageFlagBits::eColorAttachmentOutput,
		swapchain_->GetImageAcquireSemaphore(), swapchain_->GetRenderCompleteSemaphore(), swapchain_->GetCurrentFence()
	);

	swapchain_->Present();
}

void App::OnResize(unsigned int width, unsigned int height)
{
	if (width == 0 || height == 0) return;
	windowWidth_ = width;
	windowHeight_ = height;

	sceneWidth_ = windowWidth_ * sceneViewScaleX_;
	sceneHeight_ = windowHeight_ * sceneViewScaleY_;
	changedSceneWidth_ = sceneWidth_;
	changedSceneHeight_ = sceneHeight_;
	panelWidth_ = windowWidth_ * (1.0f - sceneViewScaleX_);
	panelHeight_ = windowHeight_;
	changedPanelWidth_ = panelWidth_;
	changedPanelHeight_ = panelHeight_;
	filePanelWidth_ = windowWidth_ * sceneViewScaleX_;
	filePanelHeight_ = windowHeight_ * (1.0f - sceneViewScaleY_);
	changedFilePanelWidth_ = filePanelWidth_;
	changedFilePanelHeight_ = filePanelHeight_;

	device_.WaitIdle(QueueContextType::General);
	swapchain_->Recreate(width, height);
	geometryFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	lightingFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	presentFrameBuffer_->Recreate(width, height);

	geometryDescriptorSets_.clear();
	lightingDescriptorSets_.clear();
	guiDescriptorSets_.clear();
	geometryDescriptorSets_.resize(swapchain_->GetInflightCount());
	lightingDescriptorSets_.resize(swapchain_->GetInflightCount());
	guiDescriptorSets_.resize(swapchain_->GetInflightCount());
	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		geometryDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ colorBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment }
			}
		);

		lightingDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ colorBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ geometryFrameBuffer_->GetAttachmentImage(0, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ geometryFrameBuffer_->GetAttachmentImage(1, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ geometryFrameBuffer_->GetAttachmentImage(2, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			}
		);

		guiDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ lightingFrameBuffer_->GetAttachmentImage(0, i), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment },
			}
		);
	}

	geometryPipeline_.reset();
	lightingPipeline_.reset();

	geometryPipeline_ = device_.CreatePipeline(geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_, geometryDescriptorSets_[0]);
	lightingPipeline_ = device_.CreatePipeline(lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_, lightingDescriptorSets_[0]);
}

void App::OnTerminate()
{
	device_.WaitIdle(QueueContextType::General);
}