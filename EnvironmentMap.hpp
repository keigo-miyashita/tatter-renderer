#pragma once

#include <pch.hpp>
#include <sqrap.hpp>

struct alignas(16) Sizes
{
    alignas(16) glm::ivec2 hdrSizes;
    alignas(16) glm::ivec2 envMapSizes;
};

struct Push
{
    int face;
    int size = 512;
    int sampleCount = 100;
	float roughness = 0.0f;
};

class EnvironmentMap {
private:
    const sqrp::Device* pDevice_ = nullptr;

    std::string dir_;

    int width_, height_;

    sqrp::BufferHandle stagingBuffer_;

    sqrp::ImageHandle envTexture_;
    sqrp::ImageHandle envMap_;

	sqrp::ImageHandle irradianceMap_;
	sqrp::ImageHandle prefilterMap_;
    int prefileterMipCounts_ = 10;
	sqrp::ImageHandle brdfLUT_;
   

public:
    EnvironmentMap(const sqrp::Device& device, std::string dir, std::string name, sqrp::ShaderHandle envmap, sqrp::ShaderHandle irradiance, sqrp::ShaderHandle prefilter, sqrp::ShaderHandle brdfLUT);
    ~EnvironmentMap() = default;

    sqrp::ImageHandle GetEnvMap() const;
	sqrp::ImageHandle GetIrradianceMap() const;
    sqrp::ImageHandle GetPrefilterMap() const;
	sqrp::ImageHandle GetBrdfLUT() const;
};

using EnvMapHandle = std::shared_ptr<EnvironmentMap>;