#include "App.hpp"

using namespace std;
using namespace sqrp;

sqrp::ImageHandle App::CreateIcon(std::string path)
{
	int texWidth = 0, texHeight = 0, texChannels = 0;
	stbi_uc* pixels = stbi_load(path.c_str(), & texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		fprintf(stderr, "Failed to load image %s\n", path);
		throw std::runtime_error("Failed to load image");
	}

	std::vector<uint8_t> data(pixels, pixels + texWidth * texHeight * 4); // RGBA
	stbi_image_free(pixels);


	sqrp::BufferHandle stagingBuffer = device_.CreateBuffer("iconStaging", texWidth * texHeight * 4 * sizeof(uint8_t), vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);
	stagingBuffer->Write(data.data(), texWidth * texHeight * 4 * sizeof(uint8_t));

	vk::ImageCreateInfo iconImageInfo = {};
	iconImageInfo
		.setImageType(vk::ImageType::e2D)
		.setFormat(vk::Format::eR8G8B8A8Srgb)
		.setExtent(vk::Extent3D{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 })
		.setMipLevels(1)
		.setArrayLayers(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setInitialLayout(vk::ImageLayout::eUndefined);
	vk::SamplerCreateInfo iconSamplerInfo = {};
	iconSamplerInfo
		.setAddressModeU(vk::SamplerAddressMode::eRepeat)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMaxLod(1.0f)
		.setMinLod(0.0f)
		.setBorderColor(vk::BorderColor::eFloatOpaqueBlack);
	sqrp::ImageHandle iconTexture = device_.CreateImage(
		path,
		iconImageInfo,
		vk::ImageAspectFlagBits::eColor,
		iconSamplerInfo
	);

	device_.OneTimeSubmit([&](sqrp::CommandBufferHandle pCommandBuffer) {
		pCommandBuffer->TransitionLayout(iconTexture, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		pCommandBuffer->CopyBufferToImage(stagingBuffer, iconTexture);
		pCommandBuffer->TransitionLayout(iconTexture, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	});

	return iconTexture;
}

void App::Recreate()
{
	device_.WaitIdle(QueueContextType::General);

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

	GuiWindowSize sceneViewSize = guiManager_->GetSceneViewSize();
	GuiWindowSize inspectorViewSize = guiManager_->GetInspectorViewSize();
	GuiWindowSize filePanelSize = guiManager_->GetAssetViewSize();
	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		baseColorMetallnessImages_[i]->Recreate(sceneViewSize.width, sceneViewSize.height);
		normalRoughnessImages_[i]->Recreate(sceneViewSize.width, sceneViewSize.height);
		emissiveAOImages_[i]->Recreate(sceneViewSize.width, sceneViewSize.height);
		depthImages_[i]->Recreate(sceneViewSize.width, sceneViewSize.height);
		renderImages_[i]->Recreate(sceneViewSize.width, sceneViewSize.height);
		toneMappedImages_[i]->Recreate(sceneViewSize.width, sceneViewSize.height);
	}

	swapchain_->Recreate(windowWidth_, windowHeight_);
	forwardFrameBuffer_->Recreate(sceneViewSize.width, sceneViewSize.height);
	geometryFrameBuffer_->Recreate(sceneViewSize.width, sceneViewSize.height);
	lightingFrameBuffer_->Recreate(sceneViewSize.width, sceneViewSize.height);
	skyboxFrameBuffer_->Recreate(sceneViewSize.width, sceneViewSize.height);
	toneMapFrameBuffer_->Recreate(sceneViewSize.width, sceneViewSize.height);
	presentFrameBuffer_->Recreate(windowWidth_, windowHeight_);

	forwardDescriptorSets_.clear();
	geomDescriptorSets_.clear();
	lightDescriptorSets_.clear();
	for (const auto& [objectName, objectData] : objectData_) {
		const auto& mesh = objectData->GetPModelData()->GetMesh();
		int meshNum = mesh->GetMeshNum();
		forwardDescriptorSets_[objectName].resize(meshNum);
		geomDescriptorSets_[objectName].resize(meshNum);
		lightDescriptorSets_[objectName].resize(meshNum);
		for (const auto& subMeshinfo : mesh->GetSubMeshInfos()) {
			sqrp::TransformMatrix mat = subMeshinfo.mat;
			int meshID = subMeshinfo.meshIndex;
			int primitiveNum = mesh->GetPrimitiveNumPerMesh(meshID);
			forwardDescriptorSets_[objectName][meshID].resize(primitiveNum);
			geomDescriptorSets_[objectName][meshID].resize(primitiveNum);
			lightDescriptorSets_[objectName][meshID].resize(primitiveNum);
			for (int primitiveID = 0; primitiveID < mesh->GetPrimitiveNumPerMesh(meshID); primitiveID++) {
				int materialIndex = mesh->GetMaterialIndex(primitiveID);
				forwardDescriptorSets_[objectName][meshID][primitiveID].resize(swapchain_->GetInflightCount());
				geomDescriptorSets_[objectName][meshID][primitiveID].resize(swapchain_->GetInflightCount());
				lightDescriptorSets_[objectName][meshID][primitiveID].resize(swapchain_->GetInflightCount());
				for (int frameID = 0; frameID < swapchain_->GetInflightCount(); frameID++) {
					forwardDescriptorSets_[objectName][meshID][primitiveID][frameID] = device_.CreateDescriptorSet(
						"forward" + std::to_string(frameID),
						{
						{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ objectData->GetObjectBuffer(), vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ objectData->GetPModelData()->GetMaterial()->GetBaseColorTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetMetallicRoughnessTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetNormalTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetOcclusionTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetEmissiveTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
						}
					);
					geomDescriptorSets_[objectName][meshID][primitiveID][frameID] = device_.CreateDescriptorSet(
						"geometry" + std::to_string(frameID),
						{
						{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ objectData->GetObjectBuffer(), vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ objectData->GetPModelData()->GetMaterial()->GetBaseColorTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetMetallicRoughnessTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetNormalTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetOcclusionTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetEmissiveTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						}
						);

					lightDescriptorSets_[objectName][meshID][primitiveID][frameID] = device_.CreateDescriptorSet(
						"lighting" + std::to_string(frameID),
						{
						{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ baseColorMetallnessImages_[frameID], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ normalRoughnessImages_[frameID], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ emissiveAOImages_[frameID], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ depthImages_[frameID], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
						}
					);
				}
			}
		}
	}

	skyboxDescriptorSets_.clear();
	toneMapDescriptorSets_.clear();
	skyboxDescriptorSets_.resize(swapchain_->GetInflightCount());
	toneMapDescriptorSets_.resize(swapchain_->GetInflightCount());
	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {

		skyboxDescriptorSets_[i] = device_.CreateDescriptorSet(
			"skybox" + std::to_string(i),
			{
			{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetEnvMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			}
		);

		toneMapDescriptorSets_[i] = device_.CreateDescriptorSet(
			"tonemap" + std::to_string(i),
			{
			{ renderImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			}
		);
	}

	forwardPipeline_.reset();
	geomPipeline_.reset();
	lightPipeline_.reset();

	if (objectData_.size() != 0) {
		forwardPipeline_ = device_.CreateGraphicsPipeline(
			"foward",
			forwardRenderPass_, swapchain_, forwardVertShader_, forwardPixelShader_,
			forwardDescriptorSets_.begin()->second[0][0][0],
			vk::PushConstantRange()
			.setOffset(0)
			.setSize(sizeof(Factors))
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		);
		geomPipeline_ = device_.CreateGraphicsPipeline(
			"geometry",
			geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_,
			geomDescriptorSets_.begin()->second[0][0][0],
			vk::PushConstantRange()
			.setOffset(0)
			.setSize(sizeof(Factors))
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		);
		lightPipeline_ = device_.CreateGraphicsPipeline(
			"lighting",
			lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_,
			lightDescriptorSets_.begin()->second[0][0][0], {}, true, false
		);
	}

	skyboxPipeline_ = device_.CreateGraphicsPipeline(
		"skybox",
		skyboxRenderPass_, swapchain_, skyboxVertShader_, skyboxPixelShader_,
		skyboxDescriptorSets_[0], {}, false, false
	);

	toneMapPipeline_ = device_.CreateGraphicsPipeline(
		"tonemap",
		toneMapRenderPass_, swapchain_, toneMapVertShader_, toneMapPixelShader_,
		toneMapDescriptorSets_[0], {}, true, false
	);
}

App::App(std::string appName, unsigned int windowWidth, unsigned int windowHeight)
	: Application(appName, windowWidth, windowHeight)
{

}

void App::OnStart()
{
	NFD::Guard nfdGuard;

	device_.Init(*this);

	swapchain_ = device_.CreateSwapchain(windowWidth_, windowHeight_);

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

	uint32_t sceneViewWidth = windowWidth_ * 0.8f;
	uint32_t sceneViewHeight = windowHeight_ * 0.7f;
	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		baseColorMetallnessImages_[i] = device_.CreateImage(
			"BaseColorMetallness" + to_string(i),
			vk::Extent3D{ sceneViewWidth, sceneViewHeight, 1 },
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
			vk::Extent3D{ sceneViewWidth, sceneViewHeight, 1 },
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
			vk::Extent3D{ sceneViewWidth, sceneViewHeight, 1 },
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
			vk::Extent3D{ sceneViewWidth, sceneViewHeight, 1 },
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
			vk::Extent3D{ sceneViewWidth, sceneViewHeight, 1 },
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
			vk::Extent3D{ sceneViewWidth, sceneViewHeight, 1 },
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

	std::map<string, AttachmentInfo> forwardAttachmentNameToInfo;
	AttachmentInfo forwardRenderingResultAttachment;
	forwardRenderingResultAttachment.attachmentDesc
		.setFormat(vk::Format::eR16G16B16A16Sfloat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
	forwardRenderingResultAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	forwardAttachmentNameToInfo["RenderingResult"] = forwardRenderingResultAttachment;

	AttachmentInfo forwardDepthAttachment;
	forwardDepthAttachment.attachmentDesc
		.setFormat(vk::Format::eD32Sfloat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	forwardDepthAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	forwardAttachmentNameToInfo["Depth"] = forwardDepthAttachment;

	SubPassInfo forwardSubpass;
	forwardSubpass.attachmentInfos = { "RenderingResult", "Depth"};

	forwardRenderPass_ = device_.CreateRenderPass(
		"forward",
		{ forwardSubpass }, forwardAttachmentNameToInfo
	);

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
		"geometry",
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
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
	renderingResultAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
	lightingAttachmentNameToInfo["RenderingResult"] = renderingResultAttachment;

	SubPassInfo lightingSubpass;
	lightingSubpass.attachmentInfos = { "RenderingResult"};

	lightingRenderPass_ = device_.CreateRenderPass(
		"lighting",
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
		.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	depthAttachment.attachmentDesc
		.setFormat(vk::Format::eD32Sfloat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eLoad)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	skyboxAttachmentNameToInfo["RenderingResult"] = renderingResultAttachment;
	skyboxAttachmentNameToInfo["Depth"] = depthAttachment;

	SubPassInfo skyboxSubpass;
	skyboxSubpass.attachmentInfos = { "RenderingResult", "Depth" };

	skyboxRenderPass_ = device_.CreateRenderPass(
		"skybox",
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
		"tonemap",
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
		"present",
		{ presentSubpass }, presentAttachmentNameToInfo
	);

	forwardFrameBuffer_ = device_.CreateFrameBuffer(
		"forward",
		forwardRenderPass_,
		{
			{ renderImages_ },
			{ depthImages_ }
		},
		sceneViewWidth,
		sceneViewHeight,
		swapchain_->GetInflightCount()
	);

	geometryFrameBuffer_ = device_.CreateFrameBuffer(
		"geometry",
		geometryRenderPass_,
		{
			{ baseColorMetallnessImages_ },
			{ normalRoughnessImages_},
			{ emissiveAOImages_},
			{ depthImages_}
		},
		sceneViewWidth,
		sceneViewHeight,
		swapchain_->GetInflightCount()
	);

	lightingFrameBuffer_ = device_.CreateFrameBuffer(
		"lighting",
		lightingRenderPass_,
		{
			{ renderImages_ },
		},
		sceneViewWidth,
		sceneViewHeight,
		swapchain_->GetInflightCount()
	);

	skyboxFrameBuffer_ = device_.CreateFrameBuffer(
		"skybox",
		skyboxRenderPass_,
		{
			{ renderImages_ },
			{ depthImages_ }
		},
		sceneViewWidth,
		sceneViewHeight,
		swapchain_->GetInflightCount()
	);

	toneMapFrameBuffer_ = device_.CreateFrameBuffer(
		"tonemap",
		toneMapRenderPass_,
		{
			{ toneMappedImages_ }
		},
		sceneViewWidth,
		sceneViewHeight,
		swapchain_->GetInflightCount()
	);

	presentFrameBuffer_ = device_.CreateFrameBuffer(
		"present",
		presentRenderPass_,
		swapchain_,
		{}
	);

	GLTFMeshHandle mesh = device_.CreateGLTFMesh(string(MODEL_DIR) + "DamagedHelmet.gltf");
	MaterialHandle material = std::make_shared<Material>(device_, string(MODEL_DIR) + "DamagedHelmet.gltf");
	models_.emplace(
		mesh->GetName(),
		std::make_shared<ModelData>(mesh, material)
	);


	camera_.Init((float)sceneViewWidth / (float)sceneViewHeight, glm::vec3(0.0f, 0.0f, 5.0f)); // Note sign

	light0_.pos = glm::vec4(10.0f, 10.0f, 5.0f, 1.0f);
	light0_.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	cameraBuffer_ = device_.CreateBuffer("camera", sizeof(CameraMatrix), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });

	lightBuffer_ = device_.CreateBuffer("light", sizeof(Light), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	lightBuffer_->Write(light0_);

	colorBuffer_ = device_.CreateBuffer("color", sizeof(glm::vec4), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	colorBuffer_->Write(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	detailCameraBuffer_ = device_.CreateBuffer("detailCamera", sizeof(DetailCamera), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	detailCameraBuffer_->Write(DetailCamera{ camera_.GetView(), camera_.GetProj(), camera_.GetInvView(), camera_.GetInvProj(), camera_.GetPos(), camera_.GetNearClip(), camera_.GetFarClip()});

	forwardVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "forward.shader", sqrp::ShaderType::Vertex);
	forwardPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "forward.shader", sqrp::ShaderType::Pixel);
	geomVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "geometrypass.shader", sqrp::ShaderType::Vertex);
	geomPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "geometrypass.shader", sqrp::ShaderType::Pixel);
	lightVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "lightingpass.shader", sqrp::ShaderType::Vertex);
	lightPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "lightingpass.shader", sqrp::ShaderType::Pixel);

	skyboxVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "skybox.shader", sqrp::ShaderType::Vertex);
	skyboxPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "skybox.shader", sqrp::ShaderType::Pixel);
	toneMapVertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "tonemap.shader", sqrp::ShaderType::Vertex);
	toneMapPixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "tonemap.shader", sqrp::ShaderType::Pixel);

	envMapCompShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "envmap.shader", sqrp::ShaderType::Compute);
	irradianceCompShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "irradiancemap.shader", sqrp::ShaderType::Compute);
	prefilterCompShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "specularprefiltering.shader", sqrp::ShaderType::Compute);
	brdfLUTCompShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "brdfLUT.shader", sqrp::ShaderType::Compute);

	EnvironmentMapHandle envMap = std::make_shared<EnvironmentMap>(device_, string(HDR_DIR), "warm_restaurant_4k.hdr", envMapCompShader_, irradianceCompShader_, prefilterCompShader_, brdfLUTCompShader_);
	envMaps_.insert({ envMap->GetName(), envMap });
	envMapNames_.push_back(envMap->GetName());
	guiManager_ = std::make_shared<GuiManager>(this);
	//guiManager_->SetSelectedObjectName(objectNames_[0]);
	guiManager_->SetSelectedEnvMapName(envMapNames_[0]);

	for (const auto& [objectName, objectData] : objectData_) {
		const auto& mesh = objectData->GetPModelData()->GetMesh();
		int meshNum = mesh->GetMeshNum();
		forwardDescriptorSets_[objectName].resize(meshNum);
		geomDescriptorSets_[objectName].resize(meshNum);
		lightDescriptorSets_[objectName].resize(meshNum);
		for (const auto& subMeshinfo : mesh->GetSubMeshInfos()) {
			sqrp::TransformMatrix mat = subMeshinfo.mat;
			int meshID = subMeshinfo.meshIndex;
			int primitiveNum = mesh->GetPrimitiveNumPerMesh(meshID);
			forwardDescriptorSets_[objectName][meshID].resize(primitiveNum);
			geomDescriptorSets_[objectName][meshID].resize(primitiveNum);
			lightDescriptorSets_[objectName][meshID].resize(primitiveNum);
			for (int primitiveID = 0; primitiveID < mesh->GetPrimitiveNumPerMesh(meshID); primitiveID++) {
				int materialIndex = mesh->GetMaterialIndex(primitiveID);
				forwardDescriptorSets_[objectName][meshID][primitiveID].resize(swapchain_->GetInflightCount());
				geomDescriptorSets_[objectName][meshID][primitiveID].resize(swapchain_->GetInflightCount());
				lightDescriptorSets_[objectName][meshID][primitiveID].resize(swapchain_->GetInflightCount());
				for (int frameID = 0; frameID < swapchain_->GetInflightCount(); frameID++) {
					forwardDescriptorSets_[objectName][meshID][primitiveID][frameID] = device_.CreateDescriptorSet(
						"forward" + to_string(frameID),
						{
						{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ objectData->GetObjectBuffer(), vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ objectData->GetPModelData()->GetMaterial()->GetBaseColorTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetMetallicRoughnessTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetNormalTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetOcclusionTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetEmissiveTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
						}
					);
					geomDescriptorSets_[objectName][meshID][primitiveID][frameID] = device_.CreateDescriptorSet(
						"geometry" + to_string(frameID),
						{
						{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ objectData->GetObjectBuffer(), vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ objectData->GetPModelData()->GetMaterial()->GetBaseColorTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetMetallicRoughnessTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetNormalTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetOcclusionTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ objectData->GetPModelData()->GetMaterial()->GetEmissiveTexture(materialIndex), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						}
						);

					lightDescriptorSets_[objectName][meshID][primitiveID][frameID] = device_.CreateDescriptorSet(
						"lighting" + to_string(frameID),
						{
						{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ baseColorMetallnessImages_[frameID], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ normalRoughnessImages_[frameID], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ emissiveAOImages_[frameID], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
						{ depthImages_[frameID], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
						{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
						}
					);
				}
			}
		}
	}

	skyboxDescriptorSets_.resize(swapchain_->GetInflightCount());
	toneMapDescriptorSets_.resize(swapchain_->GetInflightCount());
	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		skyboxDescriptorSets_[i] = device_.CreateDescriptorSet(
			"skybox" + to_string(i),
			{
			{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ envMaps_[guiManager_->GetSelectedEnvMapName()]->GetEnvMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			}
		);

		toneMapDescriptorSets_[i] = device_.CreateDescriptorSet(
			"tonemap" + to_string(i),
			{
			{ renderImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			}
		);
	}


	if (objectData_.size() != 0) {
		forwardPipeline_ = device_.CreateGraphicsPipeline(
			"forward",
			forwardRenderPass_, swapchain_, forwardVertShader_, forwardPixelShader_,
			forwardDescriptorSets_.begin()->second[0][0][0],
			vk::PushConstantRange()
			.setOffset(0)
			.setSize(sizeof(Factors))
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		);
		geomPipeline_ = device_.CreateGraphicsPipeline(
			"geometry",
			geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_,
			geomDescriptorSets_.begin()->second[0][0][0],
			vk::PushConstantRange()
			.setOffset(0)
			.setSize(sizeof(Factors))
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		);
		lightPipeline_ = device_.CreateGraphicsPipeline(
			"lighting",
			lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_,
			lightDescriptorSets_.begin()->second[0][0][0], {}, true, false
		);
	}

	skyboxPipeline_ = device_.CreateGraphicsPipeline(
		"skybox",
		skyboxRenderPass_, swapchain_, skyboxVertShader_, skyboxPixelShader_,
		skyboxDescriptorSets_[0], {}, false, false
	);

	toneMapPipeline_ = device_.CreateGraphicsPipeline(
		"tonemap",
		toneMapRenderPass_, swapchain_, toneMapVertShader_, toneMapPixelShader_,
		toneMapDescriptorSets_[0], {}, true, false
	);
}

void App::OnUpdate()
{	
	if (guiManager_->GetIsNeedRecreate()) {
		device_.WaitIdle(QueueContextType::General);

		if (guiManager_->GetDeletedModelName() != "") {
			objectData_.erase(guiManager_->GetDeletedModelName());
			for (int i = 0; i < objectNames_.size(); i++) {
				if (objectNames_[i] == guiManager_->GetDeletedModelName()) {
					objectNames_.erase(objectNames_.begin() + i);
					break;
				}
			}
			guiManager_->SetSelectedObjectName((objectNames_.size() != 0) ? objectNames_[0] : "");
		}

		if (guiManager_->GetAddedModelName() != "") {
			std::shared_ptr<ObjectData> objData = std::make_shared<ObjectData>(
				device_, models_.at(guiManager_->GetAddedModelName()),
				glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
				glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)),
				1.0f
			);
			objectData_.emplace(
				objData->GetName(),
				objData
			);
			objectNames_.push_back(objData->GetName());
			guiManager_->SetSelectedObjectName(objData->GetName());
		}

		Recreate();
		guiManager_->Recreate();
	}

	newModelPath_ = "";
	newEnvMapPath_ = "";
	if (guiManager_->IsOpenFile()) {
		device_.WaitIdle(sqrp::QueueContextType::General);
		// auto-freeing memory
		NFD::UniquePath outPath;

		// prepare filters for the dialog
		nfdfilteritem_t filterItem[2] = { {"glTF file", "gltf"}, {"hdr file", "hdr"} };

		// show the dialog
		nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 2);
		if (result == NFD_OKAY) {
			std::cout << "Success!" << std::endl << outPath.get() << std::endl;
			std::string filePath = outPath.get();
			if (filePath.ends_with(".gltf") || filePath.ends_with(".GLTF")) {
				std::filesystem::path pathObj(filePath);
				newModelPath_ = pathObj.filename().string();
			}
			else if (filePath.ends_with(".hdr") || filePath.ends_with(".HDR")) {
				std::filesystem::path pathObj(filePath);
				newEnvMapPath_ = pathObj.filename().string();
			}
			else if (result == NFD_CANCEL) {
				std::cout << "User pressed cancel." << std::endl;
			}
			else {
				std::cout << "Error: " << NFD::GetError() << std::endl;
			}
		}

		if (newModelPath_ != "") {
			GLTFMeshHandle mesh = device_.CreateGLTFMesh(string(MODEL_DIR) + newModelPath_);
			MaterialHandle material = std::make_shared<Material>(device_, string(MODEL_DIR) + newModelPath_);
			models_.emplace(
				mesh->GetName(),
				std::make_shared<ModelData>(mesh, material)
			);
		}

		if (newEnvMapPath_ != "") {
			EnvironmentMapHandle envMap = std::make_shared<EnvironmentMap>(device_, string(HDR_DIR), newEnvMapPath_, envMapCompShader_, irradianceCompShader_, prefilterCompShader_, brdfLUTCompShader_);
			envMaps_.insert({ envMap->GetName(), envMap });
			envMapNames_.push_back(envMap->GetName());
		}
	}

	camera_.Update(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });
	detailCameraBuffer_->Write(DetailCamera{ camera_.GetView(), camera_.GetProj(), camera_.GetInvView(), camera_.GetInvProj(), camera_.GetPos(), camera_.GetNearClip(), camera_.GetFarClip() });
	for (auto& [name, objectData] : objectData_) {
		objectData->GetObjectBuffer()->Write(objectData->GetTransform());
	}

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

	if (guiManager_->GetRenderMode() == 1) {
		commandBuffer->BeginRenderPass(geometryRenderPass_, geometryFrameBuffer_, infligtIndex);
		commandBuffer->SetViewport(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
		commandBuffer->SetScissor(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
		for (const auto& [objectName, objectData] : objectData_) {
			const auto& mesh = objectData->GetPModelData()->GetMesh();
			for (const auto& subMeshinfo : mesh->GetSubMeshInfos()) {
				sqrp::TransformMatrix mat = subMeshinfo.mat;
				int meshID = subMeshinfo.meshIndex;
				int primitiveNum = mesh->GetPrimitiveNumPerMesh(meshID);
				for (int primitiveID = 0; primitiveID < mesh->GetPrimitiveNumPerMesh(meshID); primitiveID++) {
					int materialID = mesh->GetMaterialIndex(primitiveID);
					Factors* factors = models_.at(mesh->GetName())->GetMaterial()->GetPFactors(materialID);
					factors->model = mat.model;
					factors->invTransModel = mat.invTransModel;
					int materialIndex = mesh->GetMaterialIndex(primitiveID);
					commandBuffer->BindPipeline(geomPipeline_, vk::PipelineBindPoint::eGraphics); // When there are no objects, pipeline is binded
					commandBuffer->BindDescriptorSet(geomPipeline_, geomDescriptorSets_[objectName][meshID][primitiveID][infligtIndex], vk::PipelineBindPoint::eGraphics);
					commandBuffer->PushConstants(geomPipeline_, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(Factors), factors);
					commandBuffer->BindMeshBuffer(mesh, sizeof(sqrp::Vertex) * mesh->GetVertexRange(primitiveID).offset, sizeof(uint32_t) * mesh->GetIndexRange(primitiveID).offset);
					commandBuffer->DrawMesh(mesh, mesh->GetNumIndices(primitiveID));
				}
			}
		}
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
		commandBuffer->SetViewport(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
		commandBuffer->SetScissor(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
		for (const auto& [objectName, objectData] : objectData_) {
			const auto& mesh = objectData->GetPModelData()->GetMesh();
			for (const auto& subMeshinfo : mesh->GetSubMeshInfos()) {
				sqrp::TransformMatrix mat = subMeshinfo.mat;
				int meshID = subMeshinfo.meshIndex;
				int primitiveNum = mesh->GetPrimitiveNumPerMesh(meshID);
				for (int primitiveID = 0; primitiveID < mesh->GetPrimitiveNumPerMesh(meshID); primitiveID++) {
					commandBuffer->BindPipeline(lightPipeline_, vk::PipelineBindPoint::eGraphics);
					std::string meshName = objectData->GetPModelData()->GetMesh()->GetName();
					commandBuffer->BindDescriptorSet(lightPipeline_, lightDescriptorSets_[objectName][meshID][primitiveID][infligtIndex], vk::PipelineBindPoint::eGraphics);
					commandBuffer->Draw(3, 1); // Fullscreen Triangle
				}
			}
		}
		commandBuffer->EndRenderPass();

		commandBuffer->ImageBarrier(
			depthImages_[infligtIndex],
			vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::PipelineStageFlagBits::eEarlyFragmentTests,
			vk::AccessFlagBits::eShaderRead,
			vk::AccessFlagBits::eDepthStencilAttachmentRead
		);
	}
	else if (guiManager_->GetRenderMode() == 0) {
		commandBuffer->BeginRenderPass(forwardRenderPass_, forwardFrameBuffer_, infligtIndex);
		commandBuffer->SetViewport(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
		commandBuffer->SetScissor(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
		for (const auto& [objectName, objectData] : objectData_) {
			const auto& mesh = objectData->GetPModelData()->GetMesh();
			for (const auto& subMeshinfo : mesh->GetSubMeshInfos()) {
				sqrp::TransformMatrix mat = subMeshinfo.mat;
				int meshID = subMeshinfo.meshIndex;
				int primitiveNum = mesh->GetPrimitiveNumPerMesh(meshID);
				for (int primitiveID = 0; primitiveID < mesh->GetPrimitiveNumPerMesh(meshID); primitiveID++) {
					int materialID = mesh->GetMaterialIndex(primitiveID);
					Factors* factors = models_.at(mesh->GetName())->GetMaterial()->GetPFactors(materialID);
					factors->model = mat.model;
					factors->invTransModel = mat.invTransModel;
					int materialIndex = mesh->GetMaterialIndex(primitiveID);
					commandBuffer->BindPipeline(forwardPipeline_, vk::PipelineBindPoint::eGraphics);
					commandBuffer->BindDescriptorSet(forwardPipeline_, forwardDescriptorSets_[objectName][meshID][primitiveID][infligtIndex], vk::PipelineBindPoint::eGraphics);
					commandBuffer->PushConstants(forwardPipeline_, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(Factors), factors);
					commandBuffer->BindMeshBuffer(mesh, sizeof(sqrp::Vertex)* mesh->GetVertexRange(primitiveID).offset, sizeof(uint32_t)* mesh->GetIndexRange(primitiveID).offset);
					commandBuffer->DrawMesh(mesh, mesh->GetNumIndices(primitiveID));
				}
			}
		}
		commandBuffer->EndRenderPass();
	}

	commandBuffer->BeginRenderPass(skyboxRenderPass_, skyboxFrameBuffer_, infligtIndex);
	commandBuffer->SetViewport(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
	commandBuffer->SetScissor(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
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
	commandBuffer->SetViewport(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
	commandBuffer->SetScissor(guiManager_->GetSceneViewSize().width, guiManager_->GetSceneViewSize().height);
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

	guiManager_->DrawGui();
	commandBuffer->DrawGui(*(guiManager_->GetGui()));
	commandBuffer->EndRenderPass();

	commandBuffer->End();

	device_.Submit(
		QueueContextType::General, commandBuffer, vk::PipelineStageFlagBits::eColorAttachmentOutput,
		swapchain_->GetImageAcquireSemaphore()->GetSemaphore(), swapchain_->GetRenderCompleteSemaphore()->GetSemaphore(), swapchain_->GetCurrentFence()
	);

	swapchain_->Present();
}

void App::OnResize(unsigned int width, unsigned int height)
{
	if (width == 0 || height == 0) return;
	windowWidth_ = width;
	windowHeight_ = height;

	guiManager_->UpdateGUISize();
	Recreate();
	guiManager_->Recreate();
}

void App::OnTerminate()
{
	device_.WaitIdle(QueueContextType::General);
}