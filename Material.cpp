#include "Material.hpp"

using namespace std;

Material::Material(const sqrp::Device& device, std::string modelDir, std::string modelName)
	: pDevice_(&device), modelDir_(modelDir)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	string err, warn;

	bool success = loader.LoadASCIIFromFile(&model, &err, &warn, (modelDir_ + modelName).c_str());
	if (!success) {
		throw std::runtime_error("failed to load gltf");
	}

    for (const auto& mat : model.materials) {
        name_ = mat.name;

        // PBR Metallic-Roughness
        if (mat.pbrMetallicRoughness.baseColorFactor.size() == 4) {
            for (int i = 0; i < 4; i++) {
                baseColorFactor_[i] = static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[i]);
            }
        }
        if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
			int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
			baseColorTexture_ = LoadTexture(model, texIndex, 4, vk::Format::eR8G8B8A8Srgb);
        }

        // PBR Metallic-Roughness
        metallicFactor_ = static_cast<float>(mat.pbrMetallicRoughness.metallicFactor);
        roughnessFactor_ = static_cast<float>(mat.pbrMetallicRoughness.roughnessFactor);
        if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
            int texIndex = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
			metallicRoughnessTexture_ = LoadTexture(model, texIndex, 4, vk::Format::eR8G8B8A8Unorm); // NOTE : GB channels are used for metallic and roughness
        }

        if (mat.normalTexture.index >= 0)
        {
            int texIndex = mat.normalTexture.index;
            normalTexture_ = LoadTexture(model, texIndex, 4, vk::Format::eR8G8B8A8Unorm); // NOTE : RGB channels are used for normal map
        }

        if (mat.occlusionTexture.index >= 0)
        {
            int texIndex = mat.occlusionTexture.index;
            occlusionTexture_ = LoadTexture(model, texIndex, 1, vk::Format::eR8Unorm); // NOTE : Only R channel is used for occlusion
        }

        if (mat.emissiveFactor.size() == 3) {
            for (int i = 0; i < 3; i++)
            {
                emissiveFactor_[i] = static_cast<float>(mat.emissiveFactor[i]);
            }
        }
        if (mat.emissiveTexture.index >= 0)
        {
            int texIndex = mat.emissiveTexture.index;
			emissiveTexture_ = LoadTexture(model, texIndex, 4, vk::Format::eR8G8B8A8Srgb); // NOTE : RGB channels are used for emissive
        }

        for (int i = 0; i < 4; i++) {
            factors_.baseColorFactor[i] = baseColorFactor_[i];
        }
        for (int i = 0; i < 3; i++) {
            factors_.emissiveFactor[i] = emissiveFactor_[i];
		}
		factors_.metallicFactor = metallicFactor_;
		factors_.roughnessFactor = roughnessFactor_;

        //// ���̑�
        //m.doubleSided = mat.doubleSided;
        //m.alphaMode = mat.alphaMode;
        //m.alphaCutoff = static_cast<float>(mat.alphaCutoff);

        //outMaterials.push_back(m);
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

	sqrp::BufferHandle stagingBuffer = pDevice_->CreateBuffer(width * height * useChannelNum * sizeof(unsigned char), vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);
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

sqrp::ImageHandle Material::GetBaseColorTexture() const
{
    return baseColorTexture_;
}

sqrp::ImageHandle Material::GetMetallicRoughnessTexture() const
{
    return metallicRoughnessTexture_;
}
sqrp::ImageHandle Material::GetNormalTexture() const
{
    return normalTexture_;
}
sqrp::ImageHandle Material::GetOcclusionTexture() const
{
    return occlusionTexture_;
}
sqrp::ImageHandle Material::GetEmissiveTexture() const
{
    return emissiveTexture_;
}

Factors* Material::GetPFactors()
{
	return &factors_;
}