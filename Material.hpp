#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

struct Factors
{
    float baseColorFactor[4];
	float emissiveFactor[3];
	float padding; // 16 byte align
    float metallicFactor;
    float roughnessFactor;
    float padding1;
	float padding2;
    glm::mat4 model;
    glm::mat4 invTransModel;
};

struct SubMaterialInfo
{
	std::string name;

    Factors factors{};

    // PBR Metallic-Roughness
    sqrp::ImageHandle baseColorTexture;
    sqrp::ImageHandle metallicRoughnessTexture;
    sqrp::ImageHandle normalTexture;
    sqrp::ImageHandle occlusionTexture;
    sqrp::ImageHandle emissiveTexture;

    bool doubleSided = false;
    std::string alphaMode = "OPAQUE";
    float alphaCutoff = 0.5f;
};

class Material {
private:
	const sqrp::Device* pDevice_ = nullptr;

	std::string modelDir_;
	std::vector<SubMaterialInfo> subMaterialInfos_;

public:
    Material(const sqrp::Device& device, std::string modelPath);
	~Material() = default;

    sqrp::ImageHandle LoadTexture(const tinygltf::Model& model, int texIndex, int useChannelNum, vk::Format format);

    sqrp::ImageHandle GetBaseColorTexture(int materialIndex/* = 0*/) const;
    sqrp::ImageHandle GetMetallicRoughnessTexture(int materialIndex/* = 0*/) const;
    sqrp::ImageHandle GetNormalTexture(int materialIndex/* = 0*/) const;
    sqrp::ImageHandle GetOcclusionTexture(int materialIndex = 0) const;
    sqrp::ImageHandle GetEmissiveTexture(int materialIndex/* = 0*/) const;
	Factors* GetPFactors(int materialIndex/* = 0*/);
};

using MaterialHandle = std::shared_ptr<Material>;