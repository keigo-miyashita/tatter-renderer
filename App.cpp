#include "App.hpp"

using namespace std;
using namespace sqrp;

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

	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		baseColorMetallnessImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		normalRoughnessImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		emissiveAOImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		depthImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		renderImages_[i]->Recreate(sceneWidth_, sceneHeight_);
		toneMappedImages_[i]->Recreate(sceneWidth_, sceneHeight_);
	}

	swapchain_->Recreate(windowWidth_, windowHeight_);
	forwardFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	geometryFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	lightingFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	skyboxFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	toneMapFrameBuffer_->Recreate(sceneWidth_, sceneHeight_);
	presentFrameBuffer_->Recreate(windowWidth_, windowHeight_);

	forwardDescriptorSets_.clear();
	geomDescriptorSets_.clear();
	lightDescriptorSets_.clear();
	for (auto& [name, objectData] : objectData_) {
		forwardDescriptorSets_[name].resize(swapchain_->GetInflightCount());
		geomDescriptorSets_[name].resize(swapchain_->GetInflightCount());
		lightDescriptorSets_[name].resize(swapchain_->GetInflightCount());
		for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
			forwardDescriptorSets_[name][i] = device_.CreateDescriptorSet({
				{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectData->GetObjectBuffer(), vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectData->GetPModelData()->GetMaterial()->GetBaseColorTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetMetallicRoughnessTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetNormalTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetOcclusionTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetEmissiveTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
				}
			);
			geomDescriptorSets_[name][i] = device_.CreateDescriptorSet({
				{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectData->GetObjectBuffer(), vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectData->GetPModelData()->GetMaterial()->GetBaseColorTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetMetallicRoughnessTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetNormalTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetOcclusionTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetEmissiveTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				}
				);

			lightDescriptorSets_[name][i] = device_.CreateDescriptorSet({
				{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ baseColorMetallnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ normalRoughnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ emissiveAOImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ depthImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
				}
			);
		}
	}

	skyboxDescriptorSets_.clear();
	toneMapDescriptorSets_.clear();
	guiDescriptorSets_.clear();
	skyboxDescriptorSets_.resize(swapchain_->GetInflightCount());
	toneMapDescriptorSets_.resize(swapchain_->GetInflightCount());
	guiDescriptorSets_.resize(swapchain_->GetInflightCount());
	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		guiDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ toneMappedImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment},
			}
		);

		skyboxDescriptorSets_[i] = device_.CreateDescriptorSet({
		{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
		{ envMaps_[selectedEnvMapName_]->GetEnvMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			}
		);

		toneMapDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ renderImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			}
		);
	}

	forwardPipeline_.reset();
	geomPipeline_.reset();
	lightPipeline_.reset();

	forwardPipeline_ = device_.CreateGraphicsPipeline(
		forwardRenderPass_, swapchain_, forwardVertShader_, forwardPixelShader_,
		forwardDescriptorSets_.begin()->second[0],
		vk::PushConstantRange()
		.setOffset(0)
		.setSize(sizeof(Factors))
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
	);
	geomPipeline_ = device_.CreateGraphicsPipeline(
		geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_,
		geomDescriptorSets_.begin()->second[0],
		vk::PushConstantRange()
		.setOffset(0)
		.setSize(sizeof(Factors))
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
	);
	lightPipeline_ = device_.CreateGraphicsPipeline(
		lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_,
		lightDescriptorSets_.begin()->second[0]
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

App::App(std::string appName, unsigned int windowWidth, unsigned int windowHeight)
	: Application(appName, windowWidth, windowHeight)
{

}

void App::OnStart()
{
	NFD::Guard nfdGuard;

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
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
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
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	forwardDepthAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	forwardAttachmentNameToInfo["Depth"] = forwardDepthAttachment;

	SubPassInfo forwardSubpass;
	forwardSubpass.attachmentInfos = { "RenderingResult", "Depth"};

	forwardRenderPass_ = device_.CreateRenderPass(
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

	forwardFrameBuffer_ = device_.CreateFrameBuffer(
		forwardRenderPass_,
		{
			{ renderImages_ },
			{ depthImages_ }
		},
		sceneWidth_,
		sceneHeight_,
		swapchain_->GetInflightCount()
	);

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

	MeshHandle mesh = device_.CreateMesh(string(MODEL_DIR) + "DamagedHelmet.gltf");
	MaterialHandle material = std::make_shared<Material>(device_, string(MODEL_DIR), "DamagedHelmet.gltf");
	//cout << "model.numInstances: " << modelData.GetNumInstance() << endl;
	models_.emplace(
		mesh->GetName(),
		std::make_shared<ModelData>(mesh, material)
	);
	/*models_["DamagedHelmet"].GetMesh() = device_.CreateMesh(string(MODEL_DIR) + "DamagedHelmet.gltf");
	models_["DamagedHelmet"].GetMaterial() = std::make_shared<Material>(device_, string(MODEL_DIR), "DamagedHelmet.gltf");*/
	/*mesh_ = device_.CreateMesh(string(MODEL_DIR) + "DamagedHelmet.gltf");
	material_ = std::make_shared<Material>(device_, string(MODEL_DIR), "DamagedHelmet.gltf");*/


	// Camera
	camera_.Init((float)sceneWidth_ / (float)sceneHeight_, glm::vec3(0.0f, 0.0f, 5.0f)); // Note sign

	// Light
	light0_.pos = glm::vec4(10.0f, 10.0f, 5.0f, 1.0f);
	light0_.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Sphere0
	//object_ = {};

	cameraBuffer_ = device_.CreateBuffer(sizeof(CameraMatrix), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });

	//objects_.push_back(sqrp::Object(
	//	models_["DamagedHelmet"].GetMesh(),
	//	glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
	//	glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)),
	//	1.0f
	//));
	//objectBuffer_ = device_.CreateBuffer(sizeof(TransformMatrix), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	////objectBuffer_->Write(object_);
	//objectBuffer_->Write(objects_[0].GetTransform());

	std::shared_ptr<ObjectData> objData = std::make_shared<ObjectData>(
		device_, models_.at("DamagedHelmet"),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
		glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)),
		1.0f
	);
	//cout << "objData.GetModelData().GetNumInstance() = " << objData.GetPModelData()->GetNumInstance() << endl;
	/*objectData_[objData.GetName()] = objData;*/
	objectData_.emplace(
		objData->GetName(),
		objData
	);
	//objectData_.insert({ objData.GetName(), objData});
	objectNames_.push_back(objData->GetName());
	selectedObjectName_ = objectNames_[0];

	lightBuffer_ = device_.CreateBuffer(sizeof(Light), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	lightBuffer_->Write(light0_);

	colorBuffer_ = device_.CreateBuffer(sizeof(glm::vec4), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	colorBuffer_->Write(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	detailCameraBuffer_ = device_.CreateBuffer(sizeof(DetailCamera), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	detailCameraBuffer_->Write(DetailCamera{ camera_.GetView(), camera_.GetProj(), camera_.GetInvView(), camera_.GetInvProj(), camera_.GetPos(), camera_.GetNearClip(), camera_.GetFarClip()});

	/*descriptorSet_ = device_.CreateDescriptorSet({
		{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
		{ objectData_.at("DamagedHelmet").GetObjectBuffer(), vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
		{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
		{ colorBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment }
		}
	);*/

	vertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "Lambert.shader", sqrp::ShaderType::Vertex);
	pixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "Lambert.shader", sqrp::ShaderType::Pixel);

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
	selectedEnvMapName_ = envMapNames_[0];

	//pipeline_ = device_.CreateGraphicsPipeline(renderPass_, swapchain_, vertShader_, pixelShader_, descriptorSet_);
	
	for (auto& [name, objectData] : objectData_) {
		forwardDescriptorSets_[name].resize(swapchain_->GetInflightCount());
		geomDescriptorSets_[name].resize(swapchain_->GetInflightCount());
		lightDescriptorSets_[name].resize(swapchain_->GetInflightCount());
		for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
			forwardDescriptorSets_[name][i] = device_.CreateDescriptorSet({
				{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectData->GetObjectBuffer(), vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectData->GetPModelData()->GetMaterial()->GetBaseColorTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetMetallicRoughnessTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetNormalTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetOcclusionTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetEmissiveTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
				}
			);
			geomDescriptorSets_[name][i] = device_.CreateDescriptorSet({
				{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectData->GetObjectBuffer(), vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ objectData->GetPModelData()->GetMaterial()->GetBaseColorTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetMetallicRoughnessTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetNormalTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetOcclusionTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ objectData->GetPModelData()->GetMaterial()->GetEmissiveTexture(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				}
			);

			lightDescriptorSets_[name][i] = device_.CreateDescriptorSet({
				{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ baseColorMetallnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ normalRoughnessImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ emissiveAOImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
				{ depthImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetIrradianceMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetPrefilterMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
				{ envMaps_[selectedEnvMapName_]->GetBrdfLUT(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}
				}
			);
		}
	}

	guiDescriptorSets_.resize(swapchain_->GetInflightCount());
	skyboxDescriptorSets_.resize(swapchain_->GetInflightCount());
	toneMapDescriptorSets_.resize(swapchain_->GetInflightCount());
	for (int i = 0; i < swapchain_->GetInflightCount(); i++) {
		guiDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ toneMappedImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment},
			}
		);

		skyboxDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ detailCameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			{ envMaps_[selectedEnvMapName_]->GetEnvMap(), vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
			}
		);

		toneMapDescriptorSets_[i] = device_.CreateDescriptorSet({
			{ renderImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
			}
		);
	}


	forwardPipeline_ = device_.CreateGraphicsPipeline(
		forwardRenderPass_, swapchain_, forwardVertShader_, forwardPixelShader_, 
		forwardDescriptorSets_.begin()->second[0],
		vk::PushConstantRange()
		.setOffset(0)
		.setSize(sizeof(Factors))
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
	);
	geomPipeline_ = device_.CreateGraphicsPipeline(
		geometryRenderPass_, swapchain_, geomVertShader_, geomPixelShader_, 
		geomDescriptorSets_.begin()->second[0],
		vk::PushConstantRange()
		.setOffset(0)
		.setSize(sizeof(Factors))
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
	);
	lightPipeline_ = device_.CreateGraphicsPipeline(
		lightingRenderPass_, swapchain_, lightVertShader_, lightPixelShader_, 
		lightDescriptorSets_.begin()->second[0]
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

	if (isChangedSceneSize || isNeedRecreate_) {

		Recreate();
	}

	if (isNeedReloadModel_) {
		device_.WaitIdle(sqrp::QueueContextType::General);
		MeshHandle mesh = device_.CreateMesh(string(MODEL_DIR) + newModelPath_);
		MaterialHandle material = std::make_shared<Material>(device_, string(MODEL_DIR), newModelPath_);
		//cout << "model.numInstances: " << modelData.GetNumInstance() << endl;
		models_.emplace(
			mesh->GetName(),
			std::make_shared<ModelData>(mesh, material)
		);
	}

	if (isNeedReloadEnvMap_) {
		device_.WaitIdle(sqrp::QueueContextType::General);
		EnvironmentMapHandle envMap = std::make_shared<EnvironmentMap>(device_, string(HDR_DIR), newEnvMapPath_, envMapCompShader_, irradianceCompShader_, prefilterCompShader_, brdfLUTCompShader_);
		envMaps_.insert({ envMap->GetName(), envMap });
		envMapNames_.push_back(envMap->GetName());
	}

	if (isChangedEnvMap_) {
		Recreate();
		isChangedEnvMap_ = false;
	}

	camera_.Update(sceneWidth_, sceneHeight_);
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });
	detailCameraBuffer_->Write(DetailCamera{ camera_.GetView(), camera_.GetProj(), camera_.GetInvView(), camera_.GetInvProj(), camera_.GetPos(), camera_.GetNearClip(), camera_.GetFarClip() });
	for (auto& [name, objectData] : objectData_) {
		objectData->GetObjectBuffer()->Write(objectData->GetTransform());
	}
	//objectData_.at("DamagedHelmet0").GetObjectBuffer()->Write(objectData_.at("DamagedHelmet0").GetTransform());

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

	if (renderMode_ == 1) {
		commandBuffer->BeginRenderPass(geometryRenderPass_, geometryFrameBuffer_, infligtIndex);
		commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
		commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
		commandBuffer->BindPipeline(geomPipeline_, vk::PipelineBindPoint::eGraphics);
		for (auto& [objectName, objectData] : objectData_) {
			std::string meshName = objectData->GetPModelData()->GetMesh()->GetName();
			commandBuffer->BindDescriptorSet(geomPipeline_, geomDescriptorSets_[objectName][infligtIndex], vk::PipelineBindPoint::eGraphics);
			commandBuffer->PushConstants(geomPipeline_, vk::ShaderStageFlagBits::eFragment, sizeof(Factors), models_.at(meshName)->GetMaterial()->GetPFactors());
			commandBuffer->BindMeshBuffer(models_.at(meshName)->GetMesh());
			commandBuffer->DrawMesh(models_.at(meshName)->GetMesh());
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
		commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
		commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
		commandBuffer->BindPipeline(lightPipeline_, vk::PipelineBindPoint::eGraphics);
		for (auto& [objectName, objectData] : objectData_) {
			std::string meshName = objectData->GetPModelData()->GetMesh()->GetName();
			commandBuffer->BindDescriptorSet(lightPipeline_, lightDescriptorSets_[objectName][infligtIndex], vk::PipelineBindPoint::eGraphics);
			commandBuffer->Draw(3, 1); // Fullscreen Triangle
		}
		commandBuffer->EndRenderPass();
	}
	else if (renderMode_ == 0) {
		commandBuffer->BeginRenderPass(forwardRenderPass_, forwardFrameBuffer_, infligtIndex);
		commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
		commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
		commandBuffer->BindPipeline(forwardPipeline_, vk::PipelineBindPoint::eGraphics);
		for (auto& [objectName, objectData] : objectData_) {
			std::string meshName = objectData->GetPModelData()->GetMesh()->GetName();
			commandBuffer->BindDescriptorSet(forwardPipeline_, forwardDescriptorSets_[objectName][infligtIndex], vk::PipelineBindPoint::eGraphics);
			commandBuffer->PushConstants(forwardPipeline_, vk::ShaderStageFlagBits::eFragment, sizeof(Factors), models_.at(meshName)->GetMaterial()->GetPFactors());
			commandBuffer->BindMeshBuffer(models_.at(meshName)->GetMesh());
			commandBuffer->DrawMesh(models_.at(meshName)->GetMesh());
		}
		commandBuffer->EndRenderPass();
	}

	//for (auto& [objectName, objectData] : objectData_) {
	//	std::string meshName = objectData->GetPModelData()->GetMesh()->GetName();
	//	if (renderMode_ == 1) {
	//		commandBuffer->BeginRenderPass(geometryRenderPass_, geometryFrameBuffer_, infligtIndex);
	//		commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
	//		commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
	//		commandBuffer->BindPipeline(geomPipeline_, vk::PipelineBindPoint::eGraphics);
	//		commandBuffer->BindDescriptorSet(geomPipeline_, geomDescriptorSets_[objectName][infligtIndex], vk::PipelineBindPoint::eGraphics);
	//		commandBuffer->PushConstants(geomPipeline_, vk::ShaderStageFlagBits::eFragment, sizeof(Factors), models_.at(meshName)->GetMaterial()->GetPFactors());
	//		commandBuffer->BindMeshBuffer(models_.at(meshName)->GetMesh());
	//		commandBuffer->DrawMesh(models_.at(meshName)->GetMesh());
	//		commandBuffer->EndRenderPass();

	//		commandBuffer->ImageBarrier(
	//			baseColorMetallnessImages_[infligtIndex],
	//			vk::ImageLayout::eShaderReadOnlyOptimal,
	//			vk::ImageLayout::eShaderReadOnlyOptimal,
	//			vk::PipelineStageFlagBits::eColorAttachmentOutput,
	//			vk::PipelineStageFlagBits::eFragmentShader,
	//			vk::AccessFlagBits::eColorAttachmentWrite,
	//			vk::AccessFlagBits::eShaderRead
	//		);
	//		commandBuffer->ImageBarrier(
	//			normalRoughnessImages_[infligtIndex],
	//			vk::ImageLayout::eShaderReadOnlyOptimal,
	//			vk::ImageLayout::eShaderReadOnlyOptimal,
	//			vk::PipelineStageFlagBits::eColorAttachmentOutput,
	//			vk::PipelineStageFlagBits::eFragmentShader,
	//			vk::AccessFlagBits::eColorAttachmentWrite,
	//			vk::AccessFlagBits::eShaderRead
	//		);
	//		commandBuffer->ImageBarrier(
	//			emissiveAOImages_[infligtIndex],
	//			vk::ImageLayout::eShaderReadOnlyOptimal,
	//			vk::ImageLayout::eShaderReadOnlyOptimal,
	//			vk::PipelineStageFlagBits::eColorAttachmentOutput,
	//			vk::PipelineStageFlagBits::eFragmentShader,
	//			vk::AccessFlagBits::eColorAttachmentWrite,
	//			vk::AccessFlagBits::eShaderRead
	//		);
	//		commandBuffer->ImageBarrier(
	//			depthImages_[infligtIndex],
	//			vk::ImageLayout::eShaderReadOnlyOptimal,
	//			vk::ImageLayout::eShaderReadOnlyOptimal,
	//			vk::PipelineStageFlagBits::eLateFragmentTests,
	//			vk::PipelineStageFlagBits::eFragmentShader,
	//			vk::AccessFlagBits::eDepthStencilAttachmentWrite,
	//			vk::AccessFlagBits::eShaderRead
	//		);

	//		commandBuffer->BeginRenderPass(lightingRenderPass_, lightingFrameBuffer_, infligtIndex);
	//		commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
	//		commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
	//		commandBuffer->BindPipeline(lightPipeline_, vk::PipelineBindPoint::eGraphics);
	//		commandBuffer->BindDescriptorSet(lightPipeline_, lightDescriptorSets_[objectName][infligtIndex], vk::PipelineBindPoint::eGraphics);
	//		commandBuffer->Draw(3, 1); // Fullscreen Triangle
	//		commandBuffer->EndRenderPass();
	//	}
	//	else if (renderMode_ == 0) {
	//		commandBuffer->BeginRenderPass(forwardRenderPass_, forwardFrameBuffer_, infligtIndex);
	//		commandBuffer->SetViewport(sceneWidth_, sceneHeight_);
	//		commandBuffer->SetScissor(sceneWidth_, sceneHeight_);
	//		commandBuffer->BindPipeline(forwardPipeline_, vk::PipelineBindPoint::eGraphics);
	//		commandBuffer->BindDescriptorSet(forwardPipeline_, forwardDescriptorSets_[objectName][infligtIndex], vk::PipelineBindPoint::eGraphics);
	//		commandBuffer->PushConstants(forwardPipeline_, vk::ShaderStageFlagBits::eFragment, sizeof(Factors), models_.at(meshName)->GetMaterial()->GetPFactors());
	//		commandBuffer->BindMeshBuffer(models_.at(meshName)->GetMesh());
	//		commandBuffer->DrawMesh(models_.at(meshName)->GetMesh());
	//		commandBuffer->EndRenderPass();
	//	}
	//}

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

	ImGuiWindowFlags sceneWindowFlag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar;
	if (catchSceneDir_ & GuiDir::Left || catchSceneDir_ & GuiDir::Up || catchSceneDir_ & (GuiDir::Down | GuiDir::Left) || catchSceneDir_ & (GuiDir::Down | GuiDir::Right))
	{
		sceneWindowFlag |= ImGuiWindowFlags_NoResize;
	}
	else
	{
		sceneWindowFlag &= ~ImGuiWindowFlags_NoResize;
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(sceneWidth_, sceneHeight_));
	ImGui::Begin("Scene", nullptr, sceneWindowFlag);
	ImVec2 size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
		Input::SetCatchInput(true);
	}

	ImVec2 pos = ImGui::GetWindowPos();
	ImVec2 mouse = ImGui::GetIO().MousePos;

	bool onSceneLeft = fabs(mouse.x - pos.x) < edgeThreshold_;
	bool onSceneRight = fabs(mouse.x - (pos.x + size.x)) < edgeThreshold_;
	bool onSceneTop = fabs(mouse.y - pos.y) < edgeThreshold_;
	bool onSceneBottom = fabs(mouse.y - (pos.y + size.y)) < edgeThreshold_;

	catchSceneDir_ = GuiDir::None;
	if (ImGui::IsMouseDown(0)) {
		if (onSceneLeft)					catchSceneDir_ |= GuiDir::Left;
		if (onSceneRight)					catchSceneDir_ |= GuiDir::Right;
		if (onSceneTop)						catchSceneDir_ |= GuiDir::Up;
		if (onSceneBottom)					catchSceneDir_ |= GuiDir::Down;
	}
	else {
		catchSceneDir_ = GuiDir::None;
	}
	changedSceneWidth_ = size.x;
	changedSceneHeight_ = size.y;
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	ImTextureID texId = (ImTextureID)((VkDescriptorSet)guiDescriptorSets_[infligtIndex]->GetDescriptorSet());
	ImGui::Image(texId, contentSize); // シーンテクスチャ表示

	ImGui::SetCursorPos(ImVec2(0, 0));
	std::string selectedFile = "";
	if (ImGui::Button("File")) {
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
				isNeedReloadModel_ = true;
			}
			else if (filePath.ends_with(".hdr") || filePath.ends_with(".HDR")) {
				// HDR テクスチャ読み込み処理
				// 例: stb_image などで読み込む
				std::filesystem::path pathObj(filePath);
				newEnvMapPath_ = pathObj.filename().string();
				isNeedReloadEnvMap_ = true;
			}
		}
		else if (result == NFD_CANCEL) {
			std::cout << "User pressed cancel." << std::endl;
		}
		else {
			std::cout << "Error: " << NFD::GetError() << std::endl;
		}
	}
	else {
		isNeedReloadModel_ = false;
		isNeedReloadEnvMap_ = false;
	}
	
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	float sceneTitleBarHeight = ImGui::GetFrameHeight();
	ImGui::SetCursorPos(ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())); // Image 内の座標
	if (ImGui::Button("Translate")) gizmoOperation_ = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::Button("Rotate"))    gizmoOperation_ = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::Button("Scale"))     gizmoOperation_ = ImGuizmo::SCALE;

	glm::mat4 view = camera_.GetView();
	glm::mat4 proj = camera_.GetProj();
	glm::mat4 indentityMatrix = glm::mat4(1.0f);
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
	//ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(proj), glm::value_ptr(indentityMatrix), 100.f);
	ImGui::End();
	ImGuizmo::ViewManipulate(glm::value_ptr(view), 1.0f, ImVec2(sceneWidth_ / 6.0f * 5, sceneTitleBarHeight), ImVec2(sceneWidth_ / 6.0f, sceneHeight_ / 6.0f), 0x10101010);

	glm::quat qCamera = glm::quat_cast(glm::transpose(glm::mat3(view)));
	// Using ImGui::GetWindowWidth() returns default value, (400, 400), when no resize
	//ImVec2 gizmoPos = ImVec2(ImGui::GetWindowPos().x + sceneWidth_ - sceneWidth_ / 6.0f, ImGui::GetWindowPos().y);
	ImVec2 gizmoPos = ImVec2(sceneWidth_ - sceneWidth_ / 6.0f, 0.0f);
	ImVec2 gizmoSize = ImVec2(sceneWidth_ / 6.0f, sceneHeight_ / 6.0f);
	bool isMouseOverViewCube =
		(mouse.x >= gizmoPos.x && mouse.x <= gizmoPos.x + gizmoSize.x) &&
		(mouse.y >= gizmoPos.y && mouse.y <= gizmoPos.y + gizmoSize.y);
	if (isMouseOverViewCube) {
		Input::SetCatchInput(false);
	}
	glm::mat4 model = objectData_.at(selectedObjectName_)->GetModel();
	if (isShowGuizmo_) {
		ImGuizmo::Manipulate(
			glm::value_ptr(view), glm::value_ptr(proj), gizmoOperation_,
			ImGuizmo::MODE::LOCAL, glm::value_ptr(model)
		);
		if (ImGuizmo::IsUsing()) {
			Input::SetCatchInput(false);
		}
	}
	objectData_.at(selectedObjectName_)->UpdateTransform(model);

	ImGui::PopStyleVar();

	// 2. ツールパネル描画（横に並べる）
	glm::vec3 position = objectData_.at(selectedObjectName_)->GetPosition();
	glm::vec3 displayRotation = glm::degrees(objectData_.at(selectedObjectName_)->GetRotation());
	if (fabs(displayRotation.x) < 0.01f) displayRotation.x = 0.0f;
	if (fabs(displayRotation.y) < 0.01f) displayRotation.y = 0.0f;
	if (fabs(displayRotation.z) < 0.01f) displayRotation.z = 0.0f;
	glm::vec3 scale = objectData_.at(selectedObjectName_)->GetScale();
	auto NormalizeAngle = [](float angle) {
		while (angle > 180.0f) angle -= 360.0f;
		while (angle < -180.0f) angle += 360.0f;
		// ±180 の境界を丸める
		if (fabs(angle - 180.0f) < 0.01f || fabs(angle + 180.0f) < 0.01f)
			angle = 180.0f;
		return angle;
	};
	displayRotation.x = NormalizeAngle(displayRotation.x);
	displayRotation.y = NormalizeAngle(displayRotation.y);
	displayRotation.z = NormalizeAngle(displayRotation.z);
	ImGuiWindowFlags panelWindowFlag = ImGuiWindowFlags_NoCollapse;
	if ((catchPanelDir_ & GuiDir::Right) || (catchPanelDir_ & GuiDir::Up) || (catchPanelDir_ & GuiDir::Down))
	{
		panelWindowFlag |= ImGuiWindowFlags_NoResize;
	}
	else
	{
		panelWindowFlag &= ~ImGuiWindowFlags_NoResize;
	}
	ImGui::SetNextWindowPos(ImVec2(sceneWidth_, 0));
	ImGui::SetNextWindowSize(ImVec2(panelWidth_, windowHeight_));
	ImGui::Begin("Inspector", nullptr, panelWindowFlag);
	size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
		Input::SetCatchInput(false);
	}
	pos = ImGui::GetWindowPos();
	mouse = ImGui::GetIO().MousePos;

	bool onPanelLeft = fabs(mouse.x - pos.x) < edgeThreshold_;
	bool onPanelRight = fabs(mouse.x - (pos.x + size.x)) < edgeThreshold_;
	bool onPanelTop = fabs(mouse.y - pos.y) < edgeThreshold_;
	bool onPanelBottom = fabs(mouse.y - (pos.y + size.y)) < edgeThreshold_;

	catchPanelDir_ = GuiDir::None;
	if (ImGui::IsMouseDown(0)) {
		if (onPanelLeft)					catchPanelDir_ |= GuiDir::Left;
		if (onPanelRight)					catchPanelDir_ |= GuiDir::Right;
		if (onPanelTop)						catchPanelDir_ |= GuiDir::Up;
		if (onPanelBottom)					catchPanelDir_ |= GuiDir::Down;
	}
	else {
		catchPanelDir_ = GuiDir::None;
	}
	changedPanelWidth_ = size.x;
	changedPanelHeight_ = size.y;
	if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 子ウィンドウ開始：縦方向に自動リサイズしたい
		ImGui::BeginChild("Rendering", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);

		if (ImGui::Selectable("Forward", renderMode_ == 0)) renderMode_ = 0;
		if (ImGui::Selectable("Differed", renderMode_ == 1))    renderMode_ = 1;

		ImGui::EndChild();
	}
	if (ImGui::CollapsingHeader("Object", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 子ウィンドウ開始：縦方向に自動リサイズしたい
		ImGui::BeginChild("TransformChild", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);
		std::vector<const char*> itemPtrs;
		for (const auto& objectName : objectNames_)
			itemPtrs.push_back(objectName.c_str());
		if (ImGui::Combo("Select Model", &selectedObjectIndex_, itemPtrs.data(), static_cast<int>(itemPtrs.size())))
		{
			selectedObjectName_ = objectNames_[selectedObjectIndex_];
		}
		ImGui::Checkbox("Show Guizmo", &isShowGuizmo_);
		if (ImGui::InputFloat3("Position", glm::value_ptr(position))) {
			objectData_.at(selectedObjectName_)->SetPosition(position);
		}
		ImGui::InputFloat3("Rotation", glm::value_ptr(displayRotation));
		if (!ImGui::IsItemActive() && isModifiedRotation_)
		{
			glm::vec3 normalizedRotation;
			normalizedRotation.x = NormalizeAngle(displayRotation.x);
			normalizedRotation.y = NormalizeAngle(displayRotation.y);
			normalizedRotation.z = NormalizeAngle(displayRotation.z);
			objectData_.at(selectedObjectName_)->SetRotation(glm::quat(glm::radians(normalizedRotation)));
			isModifiedRotation_ = false;
		}
		if (ImGui::IsItemActive())
		{
			isModifiedRotation_ = true;
		}
		if (ImGui::InputFloat3("Scale", glm::value_ptr(scale)))
		{
			objectData_.at(selectedObjectName_)->SetScale(scale);
		}

		ImGui::EndChild();
	}
	if (ImGui::CollapsingHeader("EnvironmentMap", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 子ウィンドウ開始：縦方向に自動リサイズしたい
		ImGui::BeginChild("EnvironmentMap", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);
		std::vector<const char*> itemPtrs;
		for (const auto& envMapName : envMapNames_)
			itemPtrs.push_back(envMapName.c_str());
		int beforeEnvMapIndex = selectedEnvMapIndex_;
		if (ImGui::Combo("Select EnvMap", &selectedEnvMapIndex_, itemPtrs.data(), static_cast<int>(itemPtrs.size())))
		{
			if (beforeEnvMapIndex != selectedEnvMapIndex_) {
				isChangedEnvMap_ = true;
			}
			else {
				isChangedEnvMap_ = false;
			}
			selectedEnvMapName_ = envMapNames_[selectedEnvMapIndex_];
		}
		ImGui::EndChild();
	}
	ImGui::End();

	// 3. ファイルツールパネル描画（縦に並べる）
	ImGuiWindowFlags filePanelWindowFlag = ImGuiWindowFlags_NoCollapse;
	if (catchFilePanelDir_ & GuiDir::Left || catchFilePanelDir_ & GuiDir::Down)
	{
		cout << "No Resizing File Panel" << endl;
		filePanelWindowFlag |= ImGuiWindowFlags_NoResize;
	}
	else
	{
		filePanelWindowFlag &= ~ImGuiWindowFlags_NoResize;
	}
	ImGui::SetNextWindowPos(ImVec2(0, sceneHeight_));
	ImGui::SetNextWindowSize(ImVec2(filePanelWidth_, filePanelHeight_));
	ImGui::Begin("Loaded Data", nullptr, filePanelWindowFlag);
	size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
		Input::SetCatchInput(false);
	}

	pos = ImGui::GetWindowPos();
	mouse = ImGui::GetIO().MousePos;

	bool onFilePanelLeft = fabs(mouse.x - pos.x) < edgeThreshold_;
	bool onFilePanelRight = fabs(mouse.x - (pos.x + size.x)) < edgeThreshold_;
	bool onFilePanelTop = fabs(mouse.y - pos.y) < edgeThreshold_;
	bool onFilePanelBottom = fabs(mouse.y - (pos.y + size.y)) < edgeThreshold_;

	catchFilePanelDir_ = GuiDir::None;
	if (ImGui::IsMouseDown(0)) {
		if (onFilePanelLeft)					catchFilePanelDir_ |= GuiDir::Left;
		if (onFilePanelRight)					catchFilePanelDir_ |= GuiDir::Right;
		if (onFilePanelTop)						catchFilePanelDir_ |= GuiDir::Up;
		if (onFilePanelBottom)					catchFilePanelDir_ |= GuiDir::Down;
	}
	else {
		catchFilePanelDir_ = GuiDir::None;
	}
	changedFilePanelWidth_ = size.x;
	changedFilePanelHeight_ = size.y;
	ImGui::Columns(2, nullptr, false);
	if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 子ウィンドウ開始：縦方向に自動リサイズしたい
		ImGui::BeginChild("ModelChild", ImVec2(size.x / 2, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);

		for (const auto& [modelName, modelData] : models_) {
			ImGui::Text(modelName.c_str());
			ImGui::SameLine();
			ImVec2 childWindowSize = ImGui::GetWindowSize();
			ImGui::SetCursorPosX(childWindowSize.x - 50);
			if (ImGui::Button(("Add##" + modelName).c_str()))
			{
				//device_.WaitIdle(QueueContextType::General);
				std::shared_ptr<ObjectData> objData = std::make_shared<ObjectData>(
					device_, models_.at(modelName),
					glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
					glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)),
					1.0f
				);
				cout << "objData.GetModelData().GetNumInstance() = " << objData->GetPModelData()->GetNumInstance() << endl;
				/*objectData_[objData.GetName()] = objData;*/
				objectData_.emplace(
					objData->GetName(),
					objData
				);
				cout << objData->GetPModelData()->GetNumInstance() << endl;
				cout << objData->GetName() << endl;
				objectNames_.push_back(objData->GetName());

				isNeedRecreate_ = true;

				//Recreate();
			}
			else {
				isNeedRecreate_ = false;
			}
			/*if (ImGui::TreeNode(modelName.c_str())) {
				ImGui::Text("Mesh: %s", modelData.GetMesh()->GetName().c_str());
				ImGui::Text("Vertices: %d", modelData.GetMesh()->GetVertexCount());
				ImGui::Text("Indices: %d", modelData.GetMesh()->GetIndexCount());
				ImGui::Separator();
				if (ImGui::TreeNode("Material")) {
					ImGui::Text("BaseColorFactor: %.2f, %.2f, %.2f, %.2f",
						modelData.GetMaterial()->GetPFactors().baseColorFactor.r,
						modelData.GetMaterial()->GetPFactors().baseColorFactor.g,
						modelData.GetMaterial()->GetPFactors().baseColorFactor.b,
						modelData.GetMaterial()->GetPFactors().baseColorFactor.a
					);
					ImGui::Text("MetallicFactor: %.2f", modelData.GetMaterial()->GetPFactors().metallicFactor);
					ImGui::Text("RoughnessFactor: %.2f", modelData.GetMaterial()->GetPFactors().roughnessFactor);
					ImGui::Separator();
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}*/
		}

		ImGui::EndChild();
	}
	ImGui::NextColumn();
	if (ImGui::CollapsingHeader("EnvironmentMap", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("EnvironmentMapChild", ImVec2(size.x / 2, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);
		for (const auto& envMapName: envMapNames_) {
			ImGui::Text(envMapName.c_str());
		}

		ImGui::EndChild();
	}
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

	Recreate();
}

void App::OnTerminate()
{
	device_.WaitIdle(QueueContextType::General);
}