#include "Scene.hpp"

using namespace std;

ModelData::ModelData(sqrp::GLTFMeshHandle mesh, MaterialHandle material)
	: mesh_(mesh), material_(material)
{

}

void ModelData::IncrementInstance()
{
	numInstance++;
}

void ModelData::DecrementInstance()
{
	numInstance--;
}

int ModelData::GetNumInstance() const
{
	return numInstance;
}

sqrp::GLTFMeshHandle ModelData::GetMesh()
{
	return mesh_;
}

MaterialHandle ModelData::GetMaterial()
{
	return material_;
}

ObjectData::ObjectData(
	const sqrp::Device& device,
	std::shared_ptr<ModelData> model,
	glm::vec4 position,
	glm::quat quatRotation,
	float scale)
	: model_(model), position_(position), quatRotation_(quatRotation), scale_(glm::vec3(scale))
{
	name_ = model_->GetMesh()->GetName() + to_string(model_->GetNumInstance());
	model_->IncrementInstance();

	objectBuffer_ = device.CreateBuffer(sizeof(sqrp::TransformMatrix), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	//objectBuffer_->Write(object_);
	objectBuffer_->Write(GetTransform());
}

ObjectData::~ObjectData()
{
	//model_->DecrementInstance();
}

std::string ObjectData::GetName() const
{
	return name_;
}

void ObjectData::UpdateTransform(glm::mat4 model)
{
	glm::vec3 translation, scale, skew;
	glm::vec4 perspective;
	glm::quat quat;
	glm::decompose(model, scale, quat, translation, skew, perspective);

	position_ = glm::vec4(translation, 1.0f);
	quatRotation_ = quat;
	scale_ = scale;
}

glm::mat4x4 ObjectData::GetModel()
{
	return glm::translate(glm::vec3(position_)) * glm::toMat4(quatRotation_) * glm::scale(scale_);
}

glm::mat4x4 ObjectData::GetInvTransModel()
{
	// NOTE : Shoud return mat3x3
	glm::mat3x3 modelMat3x3 = glm::mat3x3(GetModel());
	glm::mat4x4 invTransMat = glm::mat4x4(glm::inverse(glm::transpose(modelMat3x3)));
	return invTransMat;
	//return glm::inverse(GetModel());
}

sqrp::TransformMatrix ObjectData::GetTransform()
{
	sqrp::TransformMatrix transformMatrix;
	transformMatrix.model = GetModel();
	transformMatrix.invTransModel = GetInvTransModel();
	return transformMatrix;
}

glm::vec3 ObjectData::GetPosition()
{
	return glm::vec3(position_);
}

glm::vec3 ObjectData::GetRotation()
{
	return glm::eulerAngles(quatRotation_);
}

glm::vec3 ObjectData::GetScale()
{
	return scale_;
}

std::shared_ptr<ModelData> ObjectData::GetPModelData()
{
	return model_;
}

sqrp::BufferHandle ObjectData::GetObjectBuffer() const
{
	return objectBuffer_;
}

void ObjectData::SetPosition(glm::vec3 position)
{
	position_ = glm::vec4(position, 1.0f);
}

void ObjectData::SetRotation(glm::quat quatRotation)
{
	quatRotation_ = quatRotation;
}

void ObjectData::SetScale(glm::vec3 scale)
{
	scale_ = scale;
}