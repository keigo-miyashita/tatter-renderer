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

	renderPass_ = device_.CreateRenderPass(swapchain_, false);

	std::array<vk::Format, 3> gbufferFormats = {
		vk::Format::eR8G8B8A8Unorm,     // BaseColor + Metalness
		vk::Format::eR8G8B8A8Unorm, // Normal + Roughness
		vk::Format::eR8G8B8A8Unorm      // Emissive + AO
	};

	baseColorMetallnessImages_.resize(swapchain_->GetInflightCount());
	normalRoughnessImages_.resize(swapchain_->GetInflightCount());
	emissiveAOImages_.resize(swapchain_->GetInflightCount());
	depthImages_.resize(swapchain_->GetInflightCount());
	renderImages_.resize(swapchain_->GetInflightCount());
	toneMappedImages_.resize(swapchain_->GetInflightCount());
	vk::SamplerCreateInfo colorSamplerInfo;
	colorSamplerInfo
		.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setMipmapMode(vk::SamplerMipmapMode::eNearest)
		.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
		.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
		.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
		.setMipLodBias(0.0f)
		.setAnisotropyEnable(VK_FALSE)
		.setMaxAnisotropy(1.0f)
		.setCompareEnable(VK_FALSE)
		.setCompareOp(vk::CompareOp::eAlways)
		.setMinLod(0.0f)
		.setMaxLod(0.0f)
		.setBorderColor(vk::BorderColor::eIntOpaqueWhite)
		.setUnnormalizedCoordinates(VK_FALSE);
	vk::SamplerCreateInfo depthSamplerInfo;
	depthSamplerInfo
		.setMagFilter(vk::Filter::eNearest)
		.setMinFilter(vk::Filter::eNearest)
		.setMipmapMode(vk::SamplerMipmapMode::eNearest)
		.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
		.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
		.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
		.setMipLodBias(0.0f)
		.setAnisotropyEnable(VK_FALSE)
		.setMaxAnisotropy(1.0f)
		.setCompareEnable(VK_FALSE)
		.setCompareOp(vk::CompareOp::eAlways)
		.setMinLod(0.0f)
		.setMaxLod(0.0f)
		.setBorderColor(vk::BorderColor::eIntOpaqueWhite)
		.setUnnormalizedCoordinates(VK_FALSE);

	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		baseColorMetallnessImages_[i] = device_.CreateImage(
			"BaseColorMetallness" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eColor,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			colorSamplerInfo
		);

		normalRoughnessImages_[i] = device_.CreateImage(
			"NormalRoughness" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eColor,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			colorSamplerInfo
		);

		emissiveAOImages_[i] = device_.CreateImage(
			"EmissiveAO" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eColor,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			colorSamplerInfo
		);

		depthImages_[i] = device_.CreateImage(
			"Depth" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
			vk::Format::eD32Sfloat,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eDepth,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			depthSamplerInfo
		);

		renderImages_[i] = device_.CreateImage(
			"Render" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::Format::eR16G16B16A16Sfloat,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eColor,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			colorSamplerInfo
		);

		toneMappedImages_[i] = device_.CreateImage(
			"toneMapped" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			swapchain_->GetSurfaceFormat(),
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eColor,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			colorSamplerInfo
		);
	}

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
		.setStoreOp(vk::AttachmentStoreOp::eStore)
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
		.setFormat(vk::Format::eR16G16B16A16Sfloat)
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

	std::map<string, AttachmentInfo> skyboxAttachmentNameToInfo;
	renderingResultAttachment.attachmentDesc
		.setFormat(vk::Format::eR16G16B16A16Sfloat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eLoad)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	depthAttachment.attachmentDesc
		.setFormat(vk::Format::eD32Sfloat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eLoad)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	skyboxAttachmentNameToInfo["RenderingResult"] = renderingResultAttachment;
	skyboxAttachmentNameToInfo["Depth"] = depthAttachment;

	SubPassInfo skyboxSubpass;
	skyboxSubpass.attachmentInfos = { "RenderingResult", "Depth" };

	skyboxRenderPass_ = device_.CreateRenderPass(
		{ skyboxSubpass }, skyboxAttachmentNameToInfo
	);

	std::map<string, AttachmentInfo> toneMapAttachmentNameToInfo;
	AttachmentInfo toneMapAttachment;
	toneMapAttachment.attachmentDesc
		.setFormat(swapchain_->GetSurfaceFormat())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	toneMapAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	toneMapAttachmentNameToInfo["toneMapped"] = toneMapAttachment;

	SubPassInfo toneMapSubpass;
	toneMapSubpass.attachmentInfos = { "toneMapped" };

	toneMapRenderPass_ = device_.CreateRenderPass(
		{ toneMapSubpass }, toneMapAttachmentNameToInfo
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
			{ baseColorMetallnessImages_ },
			{ normalRoughnessImages_},
			{ emissiveAOImages_},
			{ depthImages_}
		},
		sceneWidth_,
		sceneHeight_,
		swapchain_->GetInflightCount()
	);

	lightingFrameBuffer_ = device_.CreateFrameBuffer(
		lightingRenderPass_,
		{
			{ renderImages_ },
		},
		sceneWidth_,
		sceneHeight_,
		swapchain_->GetInflightCount()
	);

	skyboxFrameBuffer_ = device_.CreateFrameBuffer(
		skyboxRenderPass_,
		{
			{ renderImages_ },
			{ depthImages_ }
		},
		sceneWidth_,
		sceneHeight_,
		swapchain_->GetInflightCount()
	);

	toneMapFrameBuffer_ = device_.CreateFrameBuffer(
		toneMapRenderPass_,
		{
			{ toneMappedImages_ }
		},
		sceneWidth_,
		sceneHeight_,
		swapchain_->GetInflightCount()
	);

	presentFrameBuffer_ = device_.CreateFrameBuffer(
		presentRenderPass_,
		swapchain_,
		{}
	);

	mesh_ = device_.CreateMesh(string(MODEL_DIR) + "DamagedHelmet.gltf");
	material_ = std::make_shared<Material>(device_, string(MODEL_DIR), "DamagedHelmet.gltf");

	// Camera
	camera_.Init((float)sceneWidth_ / (float)sceneHeight_, glm::vec3(0.0f, 0.0f, 5.0f)); // Note sign

	// Light
	light0_.pos = glm::vec4(10.0f, 10.0f, 5.0f, 1.0f);
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

	detailCameraBuffer_ = device_.CreateBuffer(sizeof(DetailCamera), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	detailCameraBuffer_->Write(DetailCamera{ camera_.GetView(), camera_.GetProj(), camera_.GetInvView(), camera_.GetInvProj(), camera_.GetPos(), camera_.GetNearClip(), camera_.GetFarClip()});

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

	gltfGeomVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "gltfgeometrypass.shader", sqrp::ShaderType::Vertex);
	gltfGeomPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "gltfgeometrypass.shader", sqrp::ShaderType::Pixel);
	gltfLightVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "gltflightingpass.shader", sqrp::ShaderType::Vertex);
	gltfLightPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "gltflightingpass.shader", sqrp::ShaderType::Pixel);

	skyboxVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "skybox.shader", sqrp::ShaderType::Vertex);
	skyboxPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "skybox.shader", sqrp::ShaderType::Pixel);

	decodeHDRCompShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "decodeHDR.shader", sqrp::ShaderType::Compute);
	envMapCompShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "envmap.shader", sqrp::ShaderType::Compute);

	toneMapVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "tonemap.shader", sqrp::ShaderType::Vertex);
	toneMapPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "tonemap.shader", sqrp::ShaderType::Pixel);

	irradianceCompShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "irradiancemap.shader", sqrp::ShaderType::Compute);
	prefilterCompShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "specularprefiltering.shader", sqrp::ShaderType::Compute);
	brdfLUTCompShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "brdfLUT.shader", sqrp::ShaderType::Compute);

	envMap_ = std::make_shared<EnvironmentMap>(device_, string(HDR_DIR), "warm_restaurant_4k.hdr", decodeHDRCompShader_, envMapCompShader_, irradianceCompShader_, prefilterCompShader_, brdfLUTCompShader_);

	pipeline_ = device_.CreateGraphicsPipeline(renderPass_, swapchain_, vertShader_, pixelShader_, descriptorSet_);
	
	geometryDescriptorSets_.resize(swapchain_->GetInflightCount());
	lightingDescriptorSets_.resize(swapchain_->GetInflightCount());
	gltfGeomDescriptorSets_.resize(swapchain_->GetInflightCount());
	gltfLightDescriptorSets_.resize(swapchain_->GetInflightCount());
	guiDescriptorSets_.resize(swapchain_->GetInflightCount());
	skyboxDescriptorSets_.resize(swapchain_->GetInflightCount());
	toneMapDescriptorSets_.resize(swapchain_->GetInflightCount());
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
			{ baseColorMetallnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ normalRoughnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ emissiveAOImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
			}
		);

		gltfGeomDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ material_->GetBaseColorTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ material_->GetMetallicRoughnessTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ material_->GetNormalTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ material_->GetOcclusionTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ material_->GetEmissiveTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			}
		);

		gltfLightDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ baseColorMetallnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ normalRoughnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ emissiveAOImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ depthImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ envMap_->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ envMap_->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ envMap_->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
			}
		);

		guiDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ toneMappedImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment},
			}
		);

		skyboxDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ envMap_->GetEnvMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			}
		);

		toneMapDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ renderImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			}
		);
	}

	geometryPipeline_ = device_.CreateGraphicsPipeline(geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_, geometryDescriptorSets_[0]);
	lightingPipeline_ = device_.CreateGraphicsPipeline(lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_, lightingDescriptorSets_[0]);

	gltfGeomPipeline_ = device_.CreateGraphicsPipeline(
		geometryRenderPass_, swapchain_, gltfGeomVertShader_, gltfGeomPixelShader_, 
		gltfGeomDescriptorSets_[0],
		vk::PushConstantRange()
		.setOffset(0)
		.setSize(sizeof(Factors))
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
	);
	gltfLightPipeline_ = device_.CreateGraphicsPipeline(
		lightingRenderPass_, swapchain_, gltfLightVertShader_, gltfLightPixelShader_, 
		gltfLightDescriptorSets_[0]
	);

	skyboxPipeline_ = device_.CreateGraphicsPipeline(
		skyboxRenderPass_, swapchain_, skyboxVertShader_, skyboxPixelShader_,
		skyboxDescriptorSets_[0], {}, false
	);

	toneMapPipeline_ = device_.CreateGraphicsPipeline(
		toneMapRenderPass_, swapchain_, toneMapVertShader_, toneMapPixelShader_,
		toneMapDescriptorSets_[0]
	);

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

		/*baseColorMetallnessImages_.clear();
		normalRoughnessImages_.clear();
		emissiveAOImages_.clear();
		depthImages_.clear();
		renderImages_.clear();
		toneMappedImages_.clear();

		baseColorMetallnessImages_.resize(swapchain_->GetInflightCount());
		normalRoughnessImages_.resize(swapchain_->GetInflightCount());
		emissiveAOImages_.resize(swapchain_->GetInflightCount());
		depthImages_.resize(swapchain_->GetInflightCount());
		renderImages_.resize(swapchain_->GetInflightCount());
		toneMappedImages_.resize(swapchain_->GetInflightCount());*/
		vk::SamplerCreateInfo colorSamplerInfo;
		colorSamplerInfo
			.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
			.setMipLodBias(0.0f)
			.setAnisotropyEnable(VK_FALSE)
			.setMaxAnisotropy(1.0f)
			.setCompareEnable(VK_FALSE)
			.setCompareOp(vk::CompareOp::eAlways)
			.setMinLod(0.0f)
			.setMaxLod(0.0f)
			.setBorderColor(vk::BorderColor::eIntOpaqueWhite)
			.setUnnormalizedCoordinates(VK_FALSE);
		vk::SamplerCreateInfo depthSamplerInfo;
		depthSamplerInfo
			.setMagFilter(vk::Filter::eNearest)
			.setMinFilter(vk::Filter::eNearest)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
			.setMipLodBias(0.0f)
			.setAnisotropyEnable(VK_FALSE)
			.setMaxAnisotropy(1.0f)
			.setCompareEnable(VK_FALSE)
			.setCompareOp(vk::CompareOp::eAlways)
			.setMinLod(0.0f)
			.setMaxLod(0.0f)
			.setBorderColor(vk::BorderColor::eIntOpaqueWhite)
			.setUnnormalizedCoordinates(VK_FALSE);

		for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
			baseColorMetallnessImages_[i]->Recreate(sceneWidth_, sceneHeight_);
			normalRoughnessImages_[i]->Recreate(sceneWidth_, sceneHeight_);
			emissiveAOImages_[i]->Recreate(sceneWidth_, sceneHeight_);
			depthImages_[i]->Recreate(sceneWidth_, sceneHeight_);
			renderImages_[i]->Recreate(sceneWidth_, sceneHeight_);
			toneMappedImages_[i]->Recreate(sceneWidth_, sceneHeight_);
			/*baseColorMetallnessImages_[i] = device_.CreateImage(
				"BaseColorMetallness" + to_string(i),
				vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
				vk::ImageType::e2D,
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
				vk::Format::eR8G8B8A8Unorm,
				vk::ImageLayout::eUndefined,
				vk::ImageAspectFlagBits::eColor,
				1,
				1,
				vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
				colorSamplerInfo
			);

			normalRoughnessImages_[i] = device_.CreateImage(
				"NormalRoughness" + to_string(i),
				vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
				vk::ImageType::e2D,
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
				vk::Format::eR8G8B8A8Unorm,
				vk::ImageLayout::eUndefined,
				vk::ImageAspectFlagBits::eColor,
				1,
				1,
				vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
				colorSamplerInfo
			);

			emissiveAOImages_[i] = device_.CreateImage(
				"EmissiveAO" + to_string(i),
				vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
				vk::ImageType::e2D,
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
				vk::Format::eR8G8B8A8Unorm,
				vk::ImageLayout::eUndefined,
				vk::ImageAspectFlagBits::eColor,
				1,
				1,
				vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
				colorSamplerInfo
			);

			depthImages_[i] = device_.CreateImage(
				"Depth" + to_string(i),
				vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
				vk::ImageType::e2D,
				vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
				vk::Format::eD32Sfloat,
				vk::ImageLayout::eUndefined,
				vk::ImageAspectFlagBits::eDepth,
				1,
				1,
				vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
				depthSamplerInfo
			);

			renderImages_[i] = device_.CreateImage(
				"Render" + to_string(i),
				vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
				vk::ImageType::e2D,
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
				vk::Format::eR16G16B16A16Sfloat,
				vk::ImageLayout::eUndefined,
				vk::ImageAspectFlagBits::eColor,
				1,
				1,
				vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
				colorSamplerInfo
			);

			toneMappedImages_[i] = device_.CreateImage(
				"toneMapped" + to_string(i),
				vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
				vk::ImageType::e2D,
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
				vk::Format::eR16G16B16A16Sfloat,
				vk::ImageLayout::eUndefined,
				vk::ImageAspectFlagBits::eColor,
				1,
				1,
				vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
				colorSamplerInfo
			);*/
		}

		swapchain_->Recreate(windowWidth_, windowHeight_);
		geometryFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
		lightingFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
		skyboxFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
		toneMapFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
		presentFrameBuffer_->Recreate(windowWidth_, windowHeight_);

		geometryDescriptorSets_.clear();
		lightingDescriptorSets_.clear();
		gltfGeomDescriptorSets_.clear();
		gltfLightDescriptorSets_.clear();
		skyboxDescriptorSets_.clear();
		toneMapDescriptorSets_.clear();
		guiDescriptorSets_.clear();
		geometryDescriptorSets_.resize(swapchain_->GetInflightCount());
		lightingDescriptorSets_.resize(swapchain_->GetInflightCount());
		gltfGeomDescriptorSets_.resize(swapchain_->GetInflightCount());
		gltfLightDescriptorSets_.resize(swapchain_->GetInflightCount());
		skyboxDescriptorSets_.resize(swapchain_->GetInflightCount());
		toneMapDescriptorSets_.resize(swapchain_->GetInflightCount());
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
				{ baseColorMetallnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ normalRoughnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ emissiveAOImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				}
				);

			gltfGeomDescriptorSets_[i] = device_.CreateDescriptorSet({
				{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ material_->GetBaseColorTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ material_->GetMetallicRoughnessTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ material_->GetNormalTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ material_->GetOcclusionTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ material_->GetEmissiveTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				}
				);

			gltfLightDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ baseColorMetallnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ normalRoughnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ emissiveAOImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ depthImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ envMap_->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ envMap_->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ envMap_->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
				}
			);

			guiDescriptorSets_[i] = device_.CreateDescriptorSet({
				{ toneMappedImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment},
				}
				);

			skyboxDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ envMap_->GetEnvMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				}
				);

			toneMapDescriptorSets_[i] = device_.CreateDescriptorSet({
				{ renderImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				}
				);
		}

		geometryPipeline_.reset();
		lightingPipeline_.reset();
		gltfGeomPipeline_.reset();
		gltfLightPipeline_.reset();

		geometryPipeline_ = device_.CreateGraphicsPipeline(geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_, geometryDescriptorSets_[0]);
		lightingPipeline_ = device_.CreateGraphicsPipeline(lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_, lightingDescriptorSets_[0]);
		gltfGeomPipeline_ = device_.CreateGraphicsPipeline(
			geometryRenderPass_, swapchain_, gltfGeomVertShader_, gltfGeomPixelShader_,
			gltfGeomDescriptorSets_[0],
			vk::PushConstantRange()
			.setOffset(0)
			.setSize(sizeof(Factors))
			.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		);
		gltfLightPipeline_ = device_.CreateGraphicsPipeline(
			lightingRenderPass_, swapchain_, gltfLightVertShader_, gltfLightPixelShader_,
			gltfLightDescriptorSets_[0]
		);

		skyboxPipeline_ = device_.CreateGraphicsPipeline(
			skyboxRenderPass_, swapchain_, skyboxVertShader_, skyboxPixelShader_,
			skyboxDescriptorSets_[0], {}, false
		);

		toneMapPipeline_ = device_.CreateGraphicsPipeline(
			toneMapRenderPass_, swapchain_, toneMapVertShader_, toneMapPixelShader_,
			toneMapDescriptorSets_[0]
		);
	}

	camera_.Update(sceneWidth_, sceneHeight_);
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });
	detailCameraBuffer_->Write(DetailCamera{ camera_.GetView(), camera_.GetProj(), camera_.GetInvView(), camera_.GetInvProj(), camera_.GetPos(), camera_.GetNearClip(), camera_.GetFarClip() });

	swapchain_->WaitFrame();

	uint32_t infligtIndex = swapchain_->GetCurrentInflightIndex();

	auto& commandBuffer = swapchain_->GetCurrentCommandBuffer();

	commandBuffer->Begin();
	commandBuffer->ImageBarrier(
		depthImages_[infligtIndex],
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthStencilAttachmentOptimal,
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eAllCommands,
		{},
		vk::AccessFlagBits::eDepthStencilAttachmentWrite
	);

	/*commandBuffer->ImageBarrier(
		geometryFrameBuffer_->GetAttachmentImage(3, infligtIndex),
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal,
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		{},
		vk::AccessFlagBits::eTransferWrite
	);

	vk::ClearDepthStencilValue clearValue(1.0f, 0);
	vk::ImageSubresourceRange range(
		vk::ImageAspectFlagBits::eDepth,
		0, 1, 0, 1
	);

	commandBuffer->GetCommandBuffer().clearDepthStencilImage(
		geometryFrameBuffer_->GetAttachmentImage(3, infligtIndex)->GetImage(),
		vk::ImageLayout::eTransferDstOptimal,
		clearValue,
		range
	);*/

	/*commandBuffer->ImageBarrier(
		geometryFrameBuffer_->GetAttachmentImage(3, infligtIndex),
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eDepthStencilAttachmentOptimal,
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eEarlyFragmentTests,
		{},
		vk::AccessFlagBits::eDepthStencilAttachmentWrite
	);*/


	commandBuffer->BeginRenderPass(geometryRenderPass_, geometryFrameBuffer_, infligtIndex);
	commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
	commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
	commandBuffer->BindPipeline(gltfGeomPipeline_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindDescriptorSet(gltfGeomPipeline_, gltfGeomDescriptorSets_[infligtIndex], vk::PipelineBindPoint::eGraphics);
	commandBuffer->PushConstants(gltfGeomPipeline_, vk::ShaderStageFlagBits::eFragment, sizeof(Factors), material_->GetPFactors());
	commandBuffer->BindMeshBuffer(mesh_);
	commandBuffer->DrawMesh(mesh_);
	commandBuffer->EndRenderPass();

	commandBuffer->ImageBarrier(
		baseColorMetallnessImages_[infligtIndex],
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::AccessFlagBits::eShaderRead
	);
	commandBuffer->ImageBarrier(
		normalRoughnessImages_[infligtIndex],
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::AccessFlagBits::eShaderRead
	);
	commandBuffer->ImageBarrier(
		emissiveAOImages_[infligtIndex],
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::AccessFlagBits::eShaderRead
	);
	commandBuffer->ImageBarrier(
		depthImages_[infligtIndex],
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::PipelineStageFlagBits::eLateFragmentTests,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::AccessFlagBits::eDepthStencilAttachmentWrite,
		vk::AccessFlagBits::eShaderRead
	);

	commandBuffer->BeginRenderPass(lightingRenderPass_, lightingFrameBuffer_, infligtIndex);
	commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
	commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
	commandBuffer->BindPipeline(gltfLightPipeline_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindDescriptorSet(gltfLightPipeline_, gltfLightDescriptorSets_[infligtIndex], vk::PipelineBindPoint::eGraphics);
	commandBuffer->Draw(3, 1); // Fullscreen Triangle
	commandBuffer->EndRenderPass();

	commandBuffer->BeginRenderPass(skyboxRenderPass_, skyboxFrameBuffer_, infligtIndex);
	commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
	commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
	commandBuffer->BindPipeline(skyboxPipeline_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindDescriptorSet(skyboxPipeline_, skyboxDescriptorSets_[infligtIndex], vk::PipelineBindPoint::eGraphics);
	commandBuffer->Draw(3, 1); // Fullscreen Triangle
	commandBuffer->EndRenderPass();

	commandBuffer->ImageBarrier(
		renderImages_[infligtIndex],
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::AccessFlagBits::eShaderRead
	);

	commandBuffer->BeginRenderPass(toneMapRenderPass_, toneMapFrameBuffer_, infligtIndex);
	commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
	commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
	commandBuffer->BindPipeline(toneMapPipeline_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindDescriptorSet(toneMapPipeline_, toneMapDescriptorSets_[infligtIndex], vk::PipelineBindPoint::eGraphics);
	commandBuffer->Draw(3, 1); // Fullscreen Triangle
	commandBuffer->EndRenderPass();

	commandBuffer->ImageBarrier(
		toneMappedImages_[infligtIndex],
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::AccessFlagBits::eShaderRead
	);

	commandBuffer->BeginRenderPass(presentRenderPass_, presentFrameBuffer_, infligtIndex);
	commandBuffer->SetViewport(swapchain_->GetWidth(), swapchain_->GetHeight());
	commandBuffer->SetScissor(swapchain_->GetWidth(), swapchain_->GetHeight());

	gui_->NewFrame();

	ImGuiIO& io = ImGui::GetIO();

	ImGuizmo::BeginFrame();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(sceneWidth_, sceneHeight_));
	ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImVec2 size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
		Input::SetCatchInput(true);
	}
	changedSceneWidth_ = size.x;
	changedSceneHeight_ = size.y;
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	ImTextureID texId = (ImTextureID)((VkDescriptorSet)guiDescriptorSets_[infligtIndex]->GetDescriptorSet());
	ImGui::Image(texId, contentSize); // シーンテクスチャ表示
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	float sceneTitleBarHeight = ImGui::GetFrameHeight();
	glm::mat4 view = camera_.GetView();
	glm::mat4 proj = camera_.GetProj();
	glm::mat4 indentityMatrix = glm::mat4(1.0f);
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
	ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(proj), glm::value_ptr(indentityMatrix), 100.f);
	ImGui::End();
	ImGuizmo::ViewManipulate(glm::value_ptr(view), 1.0f, ImVec2(sceneWidth_ / 6.0f * 5, sceneTitleBarHeight), ImVec2(sceneWidth_ / 6.0f, sceneHeight_ / 6.0f), 0x10101010);
	glm::quat qCamera = glm::quat_cast(glm::transpose(glm::mat3(view)));
	/*camera_.SetRotation(glm::vec3(glm::eulerAngles(qCamera)));*/
	ImGui::PopStyleVar();

	// 2. ツールパネル描画（横に並べる）
	float position[] = {0.2f, 0.2f, 0.2f};
	float rotation[] = {0.2f, 0.2f, 0.2f};
	ImGui::SetNextWindowPos(ImVec2(sceneWidth_, 0));
	ImGui::SetNextWindowSize(ImVec2(panelWidth_, windowHeight_));
	ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse);
	size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
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
	ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoCollapse);
	size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
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

	/*baseColorMetallnessImages_.clear();
	normalRoughnessImages_.clear();
	emissiveAOImages_.clear();
	depthImages_.clear();
	renderImages_.clear();
	toneMappedImages_.clear();

	baseColorMetallnessImages_.resize(swapchain_->GetInflightCount());
	normalRoughnessImages_.resize(swapchain_->GetInflightCount());
	emissiveAOImages_.resize(swapchain_->GetInflightCount());
	depthImages_.resize(swapchain_->GetInflightCount());
	renderImages_.resize(swapchain_->GetInflightCount());
	toneMappedImages_.resize(swapchain_->GetInflightCount());*/
	vk::SamplerCreateInfo colorSamplerInfo;
	colorSamplerInfo
		.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setMipmapMode(vk::SamplerMipmapMode::eNearest)
		.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
		.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
		.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
		.setMipLodBias(0.0f)
		.setAnisotropyEnable(VK_FALSE)
		.setMaxAnisotropy(1.0f)
		.setCompareEnable(VK_FALSE)
		.setCompareOp(vk::CompareOp::eAlways)
		.setMinLod(0.0f)
		.setMaxLod(0.0f)
		.setBorderColor(vk::BorderColor::eIntOpaqueWhite)
		.setUnnormalizedCoordinates(VK_FALSE);
	vk::SamplerCreateInfo depthSamplerInfo;
	depthSamplerInfo
		.setMagFilter(vk::Filter::eNearest)
		.setMinFilter(vk::Filter::eNearest)
		.setMipmapMode(vk::SamplerMipmapMode::eNearest)
		.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
		.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
		.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
		.setMipLodBias(0.0f)
		.setAnisotropyEnable(VK_FALSE)
		.setMaxAnisotropy(1.0f)
		.setCompareEnable(VK_FALSE)
		.setCompareOp(vk::CompareOp::eAlways)
		.setMinLod(0.0f)
		.setMaxLod(0.0f)
		.setBorderColor(vk::BorderColor::eIntOpaqueWhite)
		.setUnnormalizedCoordinates(VK_FALSE);

	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		baseColorMetallnessImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		normalRoughnessImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		emissiveAOImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		depthImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		renderImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		toneMappedImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		/*baseColorMetallnessImages_[i] = device_.CreateImage(
			"BaseColorMetallness" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eColor,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			colorSamplerInfo
		);

		normalRoughnessImages_[i] = device_.CreateImage(
			"NormalRoughness" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eColor,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			colorSamplerInfo
		);

		emissiveAOImages_[i] = device_.CreateImage(
			"EmissiveAO" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::Format::eR8G8B8A8Unorm,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eColor,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			colorSamplerInfo
		);

		depthImages_[i] = device_.CreateImage(
			"Depth" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
			vk::Format::eD32Sfloat,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eDepth,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			depthSamplerInfo
		);

		renderImages_[i] = device_.CreateImage(
			"Render" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::Format::eR16G16B16A16Sfloat,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eColor,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			colorSamplerInfo
		);

		toneMappedImages_[i] = device_.CreateImage(
			"toneMapped" + to_string(i),
			vk::Extent3D{ sceneWidth_, sceneHeight_, 1 },
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::Format::eR16G16B16A16Sfloat,
			vk::ImageLayout::eUndefined,
			vk::ImageAspectFlagBits::eColor,
			1,
			1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			colorSamplerInfo
		);*/
	}

	swapchain_->Recreate(width, height);
	geometryFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	lightingFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	skyboxFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	toneMapFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	presentFrameBuffer_->Recreate(width, height);

	geometryDescriptorSets_.clear();
	lightingDescriptorSets_.clear();
	gltfGeomDescriptorSets_.clear();
	gltfLightDescriptorSets_.clear();
	skyboxDescriptorSets_.clear();
	toneMapDescriptorSets_.clear();
	guiDescriptorSets_.clear();
	geometryDescriptorSets_.resize(swapchain_->GetInflightCount());
	lightingDescriptorSets_.resize(swapchain_->GetInflightCount());
	gltfGeomDescriptorSets_.resize(swapchain_->GetInflightCount());
	gltfLightDescriptorSets_.resize(swapchain_->GetInflightCount());
	skyboxDescriptorSets_.resize(swapchain_->GetInflightCount());
	toneMapDescriptorSets_.resize(swapchain_->GetInflightCount());
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
			{ baseColorMetallnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ normalRoughnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ emissiveAOImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			}
			);

		gltfGeomDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ material_->GetBaseColorTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ material_->GetMetallicRoughnessTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ material_->GetNormalTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ material_->GetOcclusionTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ material_->GetEmissiveTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			}
			);

		gltfLightDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ baseColorMetallnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ normalRoughnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ emissiveAOImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ depthImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ envMap_->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ envMap_->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			{ envMap_->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
			}
		);

		guiDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ toneMappedImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment},
			}
			);

		skyboxDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ envMap_->GetEnvMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			}
			);

		toneMapDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ renderImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			}
			);
	}

	geometryPipeline_.reset();
	lightingPipeline_.reset();
	gltfGeomPipeline_.reset();
	gltfLightPipeline_.reset();

	geometryPipeline_ = device_.CreateGraphicsPipeline(geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_, geometryDescriptorSets_[0]);
	lightingPipeline_ = device_.CreateGraphicsPipeline(lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_, lightingDescriptorSets_[0]);

	gltfGeomPipeline_ = device_.CreateGraphicsPipeline(
		geometryRenderPass_, swapchain_, gltfGeomVertShader_, gltfGeomPixelShader_,
		gltfGeomDescriptorSets_[0],
		vk::PushConstantRange()
		.setOffset(0)
		.setSize(sizeof(Factors))
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
	);
	gltfLightPipeline_ = device_.CreateGraphicsPipeline(
		lightingRenderPass_, swapchain_, gltfLightVertShader_, gltfLightPixelShader_,
		gltfLightDescriptorSets_[0]
	);

	skyboxPipeline_ = device_.CreateGraphicsPipeline(
		skyboxRenderPass_, swapchain_, skyboxVertShader_, skyboxPixelShader_,
		skyboxDescriptorSets_[0], {}, false
	);

	toneMapPipeline_ = device_.CreateGraphicsPipeline(
		toneMapRenderPass_, swapchain_, toneMapVertShader_, toneMapPixelShader_,
		toneMapDescriptorSets_[0]
	);
}

void App::OnTerminate()
{
	device_.WaitIdle(QueueContextType::General);
}