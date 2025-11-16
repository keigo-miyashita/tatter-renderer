#include "Material.hpp"

using namespace std;

Material::Material(const sqrp::Device& device, std::string modelPath)
	: pDevice_(&device)
{
	std::filesystem::path path(modelPath);
	modelDir_ = path.parent_path().string() + "/";
	cout << modelDir_ << endl;

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	string err, warn;

	bool success = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath.c_str());
	if (!success) {
		throw std::runtime_error("failed to load gltf");
	}

    for (const auto& mat : model.materials) {
        SubMaterialInfo subMaterialInfo = {};
        subMaterialInfo.name = mat.name;

        if (mat.pbrMetallicRoughness.baseColorFactor.size() == 4) {
            for (int i = 0; i < 4; i++) {
                subMaterialInfo.factors.baseColorFactor[i] = static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[i]);
            }
        }
        if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
			int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
            subMaterialInfo.baseColorTexture = LoadTexture(model, texIndex, 4, vk::Format::eR8G8B8A8Srgb);
        }
        else {
            subMaterialInfo.baseColorTexture = CreateDummyTexture("BaseColor", vk::Format::eR8G8B8A8Srgb, { 1.0f, 1.0f, 1.0f, 1.0f });
        }

        // PBR Metallic-Roughness
        subMaterialInfo.factors.metallicFactor = static_cast<float>(mat.pbrMetallicRoughness.metallicFactor);
        subMaterialInfo.factors.roughnessFactor = static_cast<float>(mat.pbrMetallicRoughness.roughnessFactor);
        if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
            int texIndex = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
            subMaterialInfo.metallicRoughnessTexture = LoadTexture(model, texIndex, 4, vk::Format::eR8G8B8A8Unorm); // NOTE : GB channels are used for metallic and roughness
        }
        else {
            subMaterialInfo.metallicRoughnessTexture = CreateDummyTexture("MetallicRoughness", vk::Format::eR8G8B8A8Unorm, { 1.0f, 1.0f, 1.0f, 1.0f });
        }

        if (mat.normalTexture.index >= 0)
        {
            int texIndex = mat.normalTexture.index;
            subMaterialInfo.normalTexture = LoadTexture(model, texIndex, 4, vk::Format::eR8G8B8A8Unorm); // NOTE : RGB channels are used for normal map
        }
        else {
			// NOTE : (0.5, 0.5, 1.0) is normal vector in tangent space
            subMaterialInfo.normalTexture = CreateDummyTexture("Normal", vk::Format::eR8G8B8A8Unorm, {0.5, 0.5, 1.0, 1.0});
        }

        if (mat.occlusionTexture.index >= 0)
        {
            int texIndex = mat.occlusionTexture.index;
            subMaterialInfo.occlusionTexture = LoadTexture(model, texIndex, 1, vk::Format::eR8Unorm); // NOTE : Only R channel is used for occlusion
        }
        else {
            subMaterialInfo.occlusionTexture = CreateDummyTexture("AO", vk::Format::eR8Unorm, {1.0f});
        }

        if (mat.emissiveFactor.size() == 3) {
            for (int i = 0; i < 3; i++)
            {
                subMaterialInfo.factors.emissiveFactor[i] = static_cast<float>(mat.emissiveFactor[i]);
            }
        }
        if (mat.emissiveTexture.index >= 0)
        {
            int texIndex = mat.emissiveTexture.index;
            subMaterialInfo.emissiveTexture = LoadTexture(model, texIndex, 4, vk::Format::eR8G8B8A8Srgb); // NOTE : RGB channels are used for emissive
        }
        else {
            subMaterialInfo.emissiveTexture = CreateDummyTexture("Emissive", vk::Format::eR8G8B8A8Srgb, {0.0f, 0.0f, 0.0f, 1.0f});
        }

        subMaterialInfo.doubleSided = mat.doubleSided;
        subMaterialInfo.alphaMode = mat.alphaMode;
        subMaterialInfo.alphaCutoff = static_cast<float>(mat.alphaCutoff);

		subMaterialInfos_.push_back(subMaterialInfo);
    }
}

sqrp::ImageHandle Material::LoadTexture(const tinygltf::Model& model, int texIndex, int useChannelNum, vk::Format format)
{
    const tinygltf::Texture& tex = model.textures[texIndex];

    const tinygltf::Image& image = model.images[tex.source];

    std::string imageName = image.uri;

    int width, height, channels;

    const unsigned char* pixels = stbi_load((modelDir_ + imageName).c_str(), &width, &height, &channels, 4);

    if (pixels == nullptr) {
        throw std::runtime_error("Failed to load texture image: " + imageName);
    }

    std::vector<unsigned char> packedPixels(width * height * useChannelNum);
    for (int i = 0; i < width * height; i++) {
        for (int c = 0; c < useChannelNum; c++) {
            packedPixels[i * useChannelNum + c] = pixels[i * 4 + c];
        }
    }

	sqrp::BufferHandle stagingBuffer = pDevice_->CreateBuffer("materialStaging", width * height * useChannelNum * sizeof(unsigned char), vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);
	stagingBuffer->Write(packedPixels.data(), packedPixels.size());
	cout << "Loaded texture image: " << imageName << " (" << width << "x" << height << ", channels: " << channels << ")" << endl;

	stbi_image_free((unsigned char*)pixels);

    sqrp::ImageHandle texture = pDevice_->CreateImage(
        imageName,
        vk::Extent3D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
        vk::ImageType::e2D,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		format,
        vk::ImageLayout::eUndefined,
        vk::ImageAspectFlagBits::eColor
	);

    pDevice_->OneTimeSubmit([&](sqrp::CommandBufferHandle pCommandBuffer) {
        pCommandBuffer->TransitionLayout(texture, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        pCommandBuffer->CopyBufferToImage(stagingBuffer, texture);
        pCommandBuffer->TransitionLayout(texture, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    });
	return texture;
}

sqrp::ImageHandle Material::CreateDummyTexture(std::string name, vk::Format format, std::vector<float> initialValue)
{
    std::vector<float> data = initialValue;

    sqrp::BufferHandle stagingBuffer = pDevice_->CreateBuffer("materialStaging", data.size() * sizeof(float), vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);
    stagingBuffer->Write(data.data(), data.size());

    // NOTE : dummy texture
    sqrp::ImageHandle texture = pDevice_->CreateImage(
        name + "_dummy",
        vk::Extent3D{ static_cast<uint32_t>(1), static_cast<uint32_t>(1), 1 },
        vk::ImageType::e2D,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        format,
        vk::ImageLayout::eUndefined,
        vk::ImageAspectFlagBits::eColor
    );

    pDevice_->OneTimeSubmit([&](sqrp::CommandBufferHandle pCommandBuffer) {
        pCommandBuffer->TransitionLayout(texture, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        pCommandBuffer->CopyBufferToImage(stagingBuffer, texture);
        pCommandBuffer->TransitionLayout(texture, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        });

    return texture;
}

sqrp::ImageHandle Material::GetBaseColorTexture(int materialIndex) const
{
    return subMaterialInfos_[materialIndex].baseColorTexture;
}

sqrp::ImageHandle Material::GetMetallicRoughnessTexture(int materialIndex) const
{
    return subMaterialInfos_[materialIndex].metallicRoughnessTexture;
}
sqrp::ImageHandle Material::GetNormalTexture(int materialIndex) const
{
    return subMaterialInfos_[materialIndex].normalTexture;
}
sqrp::ImageHandle Material::GetOcclusionTexture(int materialIndex) const
{
    return subMaterialInfos_[materialIndex].occlusionTexture;
}
sqrp::ImageHandle Material::GetEmissiveTexture(int materialIndex) const
{
    return subMaterialInfos_[materialIndex].emissiveTexture;
}

Factors* Material::GetPFactors(int materialIndex)
{
	return &(subMaterialInfos_[materialIndex].factors);
}