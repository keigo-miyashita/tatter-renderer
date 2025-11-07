#include <sqrap.hpp>

#include "Material.hpp"

class ModelData
{
private:
	int numInstance = 0;
	sqrp::GLTFMeshHandle mesh_ = nullptr;
	MaterialHandle material_ = nullptr;

public:
	// NOTE ; To make sure default constructor is needed
	ModelData(sqrp::GLTFMeshHandle mesh, MaterialHandle material);
	ModelData(const ModelData&) = delete;
	ModelData& operator=(const ModelData&) = delete;
	ModelData(ModelData&&) noexcept = default; // Allow move constructor
	ModelData& operator=(ModelData&&) noexcept = default;

	void IncrementInstance();
	void DecrementInstance();

	int GetNumInstance() const;
	sqrp::GLTFMeshHandle GetMesh();
	MaterialHandle GetMaterial();
};

class ObjectData
{
private:
	std::shared_ptr<ModelData> model_;
	std::string name_ = "ObjectData";
	glm::vec4 position_;
	glm::quat quatRotation_;
	glm::vec3 scale_;
	sqrp::BufferHandle objectBuffer_ = nullptr;

public:
	ObjectData(
		const sqrp::Device& device,
		std::shared_ptr<ModelData> model,
		glm::vec4 position = { 0.0f, 0.0f, 0.0f, 1.0f },
		glm::quat quatRotation = { 0.0f, 0.0f, 0.0f, 0.0f },
		float scale = 1.0f
	);
	~ObjectData();

	void UpdateTransform(glm::mat4 model);
	std::string GetName() const;
	glm::mat4x4 GetModel();
	glm::mat4x4 GetInvTransModel();
	sqrp::TransformMatrix GetTransform();
	glm::vec3 GetPosition();
	glm::vec3 GetRotation();
	glm::vec3 GetScale();
	std::shared_ptr<ModelData> GetPModelData();
	sqrp::BufferHandle GetObjectBuffer() const;

	void SetPosition(glm::vec3 position);
	void SetRotation(glm::quat quatRotation);
	void SetScale(glm::vec3 scale);
};