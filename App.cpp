#include "App.hpp"

using namespace std;
using namespace sqrp;

App::App(std::string appName, unsigned int windowWidth, unsigned int windowHeight)
	: Application(appName, windowWidth, windowHeight)
{

}

void App::OnStart()
{
	device_.Init(*this);

	swapchain_ = device_.CreateSwapchain(windowWidth_, windowHeight_);

	renderPass_ = device_.CreateRenderPass(swapchain_);
	frameBuffer_ = device_.CreateFrameBuffer(swapchain_, renderPass_, 1);

	mesh_ = device_.CreateMesh(string(MODEL_DIR) + "Suzanne.gltf");

	// Camera
	camera_.Init((float)GetWindowWidth() / (float)GetWindowHeight(), glm::vec3(0.0f, 0.0f, 5.0f)); // Note sign

	// Light
	light0_.pos = glm::vec4(10.0f, 10.0f, -5.0f, 1.0f);
	light0_.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Sphere0
	object_ = {};
	/*XMMATRIX modelMat = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(180)), XMMatrixIdentity());
	sphere0_.model = modelMat;
	sphere0_.invTransModel = XMMatrixTranspose(XMMatrixInverse(nullptr, sphere0_.model));*/

	cameraBuffer_ = device_.CreateBuffer(sizeof(CameraMatrix), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });
	
	CameraMatrix* data = (CameraMatrix*)cameraBuffer_->Map();
	cout << data->view[0][0] << ", " << data->view[0][1] << ", " << data->view[0][2] << ", " << data->view[0][3] << endl;
	cout << data->view[1][0] << ", " << data->view[1][1] << ", " << data->view[1][2] << ", " << data->view[1][3] << endl;
	cout << data->view[2][0] << ", " << data->view[2][1] << ", " << data->view[2][2] << ", " << data->view[2][3] << endl;
	cout << data->view[3][0] << ", " << data->view[3][1] << ", " << data->view[3][2] << ", " << data->view[3][3] << endl;
	cameraBuffer_->Unmap();

	objectBuffer_ = device_.CreateBuffer(sizeof(TransformMatrix), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	objectBuffer_->Write(object_);

	TransformMatrix* objectData = (TransformMatrix*)objectBuffer_->Map();
	cout << objectData->model[0][0] << ", " << objectData->model[0][1] << ", " << objectData->model[0][2] << ", " << objectData->model[0][3] << endl;
	cout << objectData->model[1][0] << ", " << objectData->model[1][1] << ", " << objectData->model[1][2] << ", " << objectData->model[1][3] << endl;
	cout << objectData->model[2][0] << ", " << objectData->model[2][1] << ", " << objectData->model[2][2] << ", " << objectData->model[2][3] << endl;
	cout << objectData->model[3][0] << ", " << objectData->model[3][1] << ", " << objectData->model[3][2] << ", " << objectData->model[3][3] << endl;
	objectBuffer_->Unmap();

	lightBuffer_ = device_.CreateBuffer(sizeof(Light), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	lightBuffer_->Write(light0_);
	Light* lightData = (Light*)lightBuffer_->Map();
	cout << lightData->pos.x << ", " << lightData->pos.y << ", " << lightData->pos.z << ", " << lightData->pos.w << endl;
	lightBuffer_->Unmap();
	cout << "lightBuffer_->GetSize() = " << lightBuffer_->GetSize() << endl;

	colorBuffer_ = device_.CreateBuffer(sizeof(glm::vec4), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);
	colorBuffer_->Write(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	glm::vec4* colorData = (glm::vec4*)colorBuffer_->Map();
	cout << colorData->x << ", " << colorData->y << ", " << colorData->z << ", " << colorData->w << endl;
	colorBuffer_->Unmap();
	cout << "colorBuffer_->GetSize() = " << colorBuffer_->GetSize() << endl;

	descriptorSet_ = device_.CreateDescriptorSet({
		{ cameraBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
		{ objectBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
		{ lightBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
		{ colorBuffer_, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment }
		}
	);

	vertShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "Lambert.shader", sqrp::ShaderType::Vertex);
	pixelShader_ = device_.CreateShader(compiler_, string(SHADER_DIR) + "Lambert.shader", sqrp::ShaderType::Pixel);

	pipeline_ = device_.CreatePipeline(renderPass_, swapchain_, vertShader_, pixelShader_, descriptorSet_);
}

void App::OnUpdate()
{
	camera_.Update();
	cameraBuffer_->Write(CameraMatrix{ camera_.GetView(), camera_.GetProj() });

	swapchain_->WaitFrame();

	//cout << "test" << endl;

	auto& commandBuffer = swapchain_->GetCurrentCommandBuffer();

	commandBuffer->Begin();

	commandBuffer->BeginRenderPass(renderPass_, frameBuffer_);

	commandBuffer->SetViewport(swapchain_->GetWidth(), swapchain_->GetHeight());
	commandBuffer->SetScissor(swapchain_->GetWidth(), swapchain_->GetHeight());
	commandBuffer->BindPipeline(pipeline_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindDescriptorSet(pipeline_, descriptorSet_, vk::PipelineBindPoint::eGraphics);
	commandBuffer->BindMeshBuffer(mesh_);
	commandBuffer->DrawMesh(mesh_);

	commandBuffer->EndRenderPass();

	commandBuffer->End();

	device_.Submit(
		QueueContextType::General, commandBuffer, vk::PipelineStageFlagBits::eColorAttachmentOutput,
		swapchain_->GetImageAcquireSemaphore(), swapchain_->GetRenderCompleteSemaphore(), swapchain_->GetCurrentFence()
	);

	swapchain_->Present();
}

void App::OnTerminate()
{
	device_.WaitIdle(QueueContextType::General);
}