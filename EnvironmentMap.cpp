#include "EnvironmentMap.hpp"

using namespace std;

EnvironmentMap::EnvironmentMap(const sqrp::Device& device, std::string dir, std::string name, sqrp::ShaderHandle envmap, sqrp::ShaderHandle irradiance, sqrp::ShaderHandle prefilter, sqrp::ShaderHandle brdfLUT)
    : pDevice_(&device), dir_(dir)
{
	std::filesystem::path path = dir + name;
	name_ = path.stem().string();

    int w, h, c;
    //stbi_ldr_to_hdr_gamma(1.0f);
    float* hdr = stbi_loadf((dir + name).c_str(), &w, &h, &c, 4);

	width_ = w;
	height_ = h;
    std::vector<float> data(hdr, hdr + width_ * height_ * 4); // RGBE
    stbi_image_free(hdr);

	cout << "Loaded HDR image: " << name << " (" << w << "x" << h << ", channels: " << c << ")" << endl;


    stagingBuffer_ = pDevice_->CreateBuffer(width_ * height_ * 4 * sizeof(float), vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);
    stagingBuffer_->Write(data.data(), width_ * height_ * 4 * sizeof(float));
	
    vk::ImageCreateInfo envTextureImageInfo = {};
    envTextureImageInfo
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eR32G32B32A32Sfloat)
        .setExtent(vk::Extent3D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 })
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::eUndefined);
    vk::SamplerCreateInfo envTextureSamplerInfo = {};
    envTextureSamplerInfo
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setMaxLod(1.0f)
        .setMinLod(0.0f)
        .setBorderColor(vk::BorderColor::eFloatOpaqueBlack);
    envTexture_ = pDevice_->CreateImage(
        name + "Decoded",
        envTextureImageInfo,
        vk::ImageAspectFlagBits::eColor,
        envTextureSamplerInfo
	);

    pDevice_->OneTimeSubmit([&](sqrp::CommandBufferHandle pCommandBuffer) {
        pCommandBuffer->TransitionLayout(envTexture_, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        pCommandBuffer->CopyBufferToImage(stagingBuffer_, envTexture_);
        pCommandBuffer->TransitionLayout(envTexture_, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
    });

    // NOTE : Cube map size is often (hdr.width / 4, hdr.width / 4)
    vk::ImageCreateInfo imageInfo = {};
    imageInfo
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eR16G16B16A16Sfloat)
        .setExtent(vk::Extent3D{ static_cast<uint32_t>(w / 4), static_cast<uint32_t>(w / 4), 1 })
        .setMipLevels(1)
        .setArrayLayers(6)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
        .setSharingMode(vk::SharingMode::eExclusive)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
    vk::SamplerCreateInfo samplerInfo = {};
	samplerInfo
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
	    .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
	    .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
	    .setMagFilter(vk::Filter::eLinear)
	    .setMinFilter(vk::Filter::eLinear)
	    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
	    .setMaxLod(1.0f)
	    .setMinLod(0.0f)
	    .setBorderColor(vk::BorderColor::eFloatOpaqueBlack);
    envMap_ = pDevice_->CreateImage(
		name + "EnvMap",
        imageInfo,
		vk::ImageAspectFlagBits::eColor,
        samplerInfo
    );

	Sizes sizes = {};
	sizes.hdrSizes.x = w;
	sizes.hdrSizes.y = h;
	sizes.envMapSizes.x = w / 4;
	sizes.envMapSizes.y = w / 4;

	cout << "sizeof(Sizes): " << sizeof(Sizes) << endl;
    sqrp::BufferHandle sizesBuffer = pDevice_->CreateBuffer(
        sizeof(Sizes),
        vk::BufferUsageFlagBits::eUniformBuffer,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST
    );
	sizesBuffer->Write(sizes);

    sqrp::DescriptorSetHandle envMapDescSet = pDevice_->CreateDescriptorSet({
        { envTexture_, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute },
        { envMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute},
        { sizesBuffer, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute},
	});
    sqrp::ComputePipelineHandle envMapPipeline = pDevice_->CreateComputePipeline(
        envmap, envMapDescSet,
        vk::PushConstantRange{
        vk::ShaderStageFlagBits::eCompute,
        0,
        sizeof(int)
	});

    pDevice_->OneTimeSubmit([&](sqrp::CommandBufferHandle pCommandBuffer) {
        pCommandBuffer->TransitionLayout(
            envTexture_,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
        pCommandBuffer->TransitionLayout(
            envMap_,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral
        );
        pCommandBuffer->BindPipeline(envMapPipeline, vk::PipelineBindPoint::eCompute);
        pCommandBuffer->BindDescriptorSet(envMapPipeline, envMapDescSet, vk::PipelineBindPoint::eCompute);
        for (int i = 0; i < 6; i++) {
			int face = i;
            pCommandBuffer->PushConstants(envMapPipeline, vk::ShaderStageFlagBits::eCompute, sizeof(int), &face);
            pCommandBuffer->Dispatch((uint32_t)ceil(sizes.envMapSizes[0] / 8), (uint32_t)ceil(sizes.envMapSizes[1] / 8), 1);
		}
        pCommandBuffer->ImageBarrier(
            envMap_,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead
        );
    });

    vk::ImageCreateInfo irradianceMapImageInfo = {};
    irradianceMapImageInfo
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eR16G16B16A16Sfloat)
        .setExtent(vk::Extent3D{ 64, 64, 1 })
        .setMipLevels(1)
        .setArrayLayers(6)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
    vk::SamplerCreateInfo irradianceMapSamplerInfo = {};
    irradianceMapSamplerInfo
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setMaxLod(1.0f)
        .setMinLod(0.0f)
        .setBorderColor(vk::BorderColor::eFloatOpaqueBlack);
    irradianceMap_ = pDevice_->CreateImage(
        name + "IrradianceMap",
        irradianceMapImageInfo,
        vk::ImageAspectFlagBits::eColor,
        irradianceMapSamplerInfo
    );

    vk::ImageCreateInfo prefilterImageInfo = {};
    prefilterImageInfo
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eR16G16B16A16Sfloat)
        .setExtent(vk::Extent3D{ 512, 512, 1 })
        .setMipLevels(prefileterMipCounts_)
        .setArrayLayers(6)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
    vk::SamplerCreateInfo prefilterSamplerInfo = {};
    prefilterSamplerInfo
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setMaxLod(1.0f)
        .setMinLod(0.0f)
        .setBorderColor(vk::BorderColor::eFloatOpaqueBlack);
    prefilterMap_ = pDevice_->CreateImage(
        name + "PrefileterMap",
        prefilterImageInfo,
        vk::ImageAspectFlagBits::eColor,
        prefilterSamplerInfo
    );

    vk::ImageCreateInfo brdfLUTImageInfo = {};
    brdfLUTImageInfo
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eR16G16Sfloat)
        .setExtent(vk::Extent3D{ 512, 512, 1 })
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::eUndefined);
    vk::SamplerCreateInfo brdfLUTSamplerInfo = {};
    brdfLUTSamplerInfo
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setMaxLod(1.0f)
        .setMinLod(0.0f)
        .setBorderColor(vk::BorderColor::eFloatOpaqueBlack);
    brdfLUT_ = pDevice_->CreateImage(
        name + "brdfLUT",
        brdfLUTImageInfo,
        vk::ImageAspectFlagBits::eColor,
        brdfLUTSamplerInfo
    );

    sqrp::DescriptorSetHandle irradianceDescSet = pDevice_->CreateDescriptorSet({
        { envMap_, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute },
        { irradianceMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute },
    });
    sqrp::ComputePipelineHandle irradiancePipeline = pDevice_->CreateComputePipeline(
        irradiance, irradianceDescSet,
        vk::PushConstantRange{
        vk::ShaderStageFlagBits::eCompute,
        0,
        sizeof(Push)
    });

    sqrp::DescriptorSetHandle prefilterDescSet = pDevice_->CreateDescriptorSet({
        { envMap_, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute },
        { prefilterMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 0 },
        { prefilterMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 1 },
        { prefilterMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 2 },
        { prefilterMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 3 },
        { prefilterMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 4 },
        { prefilterMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 5 },
        { prefilterMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 6 },
        { prefilterMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 7 },
        { prefilterMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 8 },
        { prefilterMap_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, 9 },
        });
    sqrp::ComputePipelineHandle prefilterPipeline = pDevice_->CreateComputePipeline(
        prefilter, prefilterDescSet,
        vk::PushConstantRange{
        vk::ShaderStageFlagBits::eCompute,
        0,
        sizeof(Push)
    });

    sqrp::DescriptorSetHandle brdfLUTDescSet = pDevice_->CreateDescriptorSet({
        { brdfLUT_, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute },
    });
    sqrp::ComputePipelineHandle brdfLUTPipeline = pDevice_->CreateComputePipeline(
        brdfLUT, brdfLUTDescSet
    );

    pDevice_->OneTimeSubmit([&](sqrp::CommandBufferHandle pCommandBuffer) {
        pCommandBuffer->TransitionLayout(
            irradianceMap_,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral
        );
        pCommandBuffer->TransitionLayout(
            prefilterMap_,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral
        );
        pCommandBuffer->TransitionLayout(
            brdfLUT_,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral
        );

        pCommandBuffer->BindPipeline(irradiancePipeline, vk::PipelineBindPoint::eCompute);
        pCommandBuffer->BindDescriptorSet(irradiancePipeline, irradianceDescSet, vk::PipelineBindPoint::eCompute);
        for (int i = 0; i < 6; i++) {
			Push push = {i, 512, 100};
            pCommandBuffer->PushConstants(irradiancePipeline, vk::ShaderStageFlagBits::eCompute, sizeof(Push), &push);
            pCommandBuffer->Dispatch((uint32_t)ceil(512 / 8), (uint32_t)ceil(512 / 8), 1);
        }

        pCommandBuffer->BindPipeline(prefilterPipeline, vk::PipelineBindPoint::eCompute);
        pCommandBuffer->BindDescriptorSet(prefilterPipeline, prefilterDescSet, vk::PipelineBindPoint::eCompute);
        for (int mip = 0; mip < prefileterMipCounts_; mip++) {
            float roughness = (float)mip / (float)(prefileterMipCounts_ - 1);
            int size = 512 >> mip;
            for (int i = 0; i < 6; i++) {
                Push push = { i, size, 100, roughness };
                pCommandBuffer->PushConstants(prefilterPipeline, vk::ShaderStageFlagBits::eCompute, sizeof(Push), &push);
                pCommandBuffer->Dispatch((uint32_t)ceil(size / 8), (uint32_t)ceil(size / 8), 1);
            }
		}

        pCommandBuffer->BindPipeline(brdfLUTPipeline, vk::PipelineBindPoint::eCompute);
        pCommandBuffer->BindDescriptorSet(brdfLUTPipeline, brdfLUTDescSet, vk::PipelineBindPoint::eCompute);
        pCommandBuffer->Dispatch((uint32_t)ceil(512 / 8), (uint32_t)ceil(512 / 8), 1);

        pCommandBuffer->ImageBarrier(
            envMap_,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead
        );

        pCommandBuffer->ImageBarrier(
            irradianceMap_,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead
        );
        pCommandBuffer->ImageBarrier(
            prefilterMap_,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead
        );
        pCommandBuffer->ImageBarrier(
            brdfLUT_,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead
        );
    });
}

std::string EnvironmentMap::GetName() const
{
    return name_;
}

sqrp::ImageHandle EnvironmentMap::GetEnvMap() const
{
    return envMap_;
}

sqrp::ImageHandle EnvironmentMap::GetIrradianceMap() const
{
	return irradianceMap_;
}

sqrp::ImageHandle EnvironmentMap::GetPrefilterMap() const
{
	return prefilterMap_;
}

sqrp::ImageHandle EnvironmentMap::GetBrdfLUT() const
{
	return brdfLUT_;
}