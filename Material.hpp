#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

struct Factors
{
    float baseColorFactor[4];
	float emissiveFactor[3];
	float padding; // 16バイトアラインメント
    float metallicFactor;
    float roughnessFactor;
};

class Material {
private:
	const sqrp::Device* pDevice_ = nullptr;

	std::string modelDir_;
    std::string name_;
    
	Factors factors_{};

    // PBR Metallic-Roughness
    std::array<float, 4> baseColorFactor_ = { 1.f, 1.f, 1.f, 1.f };
    sqrp::ImageHandle baseColorTexture_;

    float metallicFactor_ = 1.f;
    float roughnessFactor_ = 1.f;
    sqrp::ImageHandle metallicRoughnessTexture_;

    // ノーマル、AO、発光
    sqrp::ImageHandle normalTexture_;
    sqrp::ImageHandle occlusionTexture_;
    std::array<float, 3> emissiveFactor_ = { 0.f, 0.f, 0.f };
    sqrp::ImageHandle emissiveTexture_;

    // 描画設定
    bool doubleSided = false;
    std::string alphaMode = "OPAQUE";
    float alphaCutoff = 0.5f;

public:
    Material(const sqrp::Device& device, std::string modelDir, std::string modelName);
	~Material() = default;

    sqrp::ImageHandle LoadTexture(const tinygltf::Model& model, int texIndex, int useChannelNum, vk::Format format);

    sqrp::ImageHandle GetBaseColorTexture() const;
    sqrp::ImageHandle GetMetallicRoughnessTexture() const;
    sqrp::ImageHandle GetNormalTexture() const;
    sqrp::ImageHandle GetOcclusionTexture() const;
    sqrp::ImageHandle GetEmissiveTexture() const;
	Factors* GetPFactors();
};

using MaterialHandle = std::shared_ptr<Material>;