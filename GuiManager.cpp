#include "GuiManager.hpp"

#include "App.hpp"

using namespace std;
using namespace sqrp;

sqrp::ImageHandle GuiManager::CreateIcon(std::string path)
{
	int texWidth = 0, texHeight = 0, texChannels = 0;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		throw std::runtime_error("Failed to load image" + path);
	}

	std::vector<uint8_t> data(pixels, pixels + texWidth * texHeight * 4); // RGBA
	stbi_image_free(pixels);


	sqrp::BufferHandle stagingBuffer = app_->device_.CreateBuffer("iconStaging", texWidth * texHeight * 4 * sizeof(uint8_t), vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);
	stagingBuffer->Write(data.data(), texWidth * texHeight * 4 * sizeof(uint8_t));

	vk::ImageCreateInfo iconImageInfo = {};
	iconImageInfo
		.setImageType(vk::ImageType::e2D)
		.setFormat(vk::Format::eR8G8B8A8Srgb)
		.setExtent(vk::Extent3D{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 })
		.setMipLevels(1)
		.setArrayLayers(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setInitialLayout(vk::ImageLayout::eUndefined);
	vk::SamplerCreateInfo iconSamplerInfo = {};
	iconSamplerInfo
		.setAddressModeU(vk::SamplerAddressMode::eRepeat)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMaxLod(1.0f)
		.setMinLod(0.0f)
		.setBorderColor(vk::BorderColor::eFloatOpaqueBlack);
	sqrp::ImageHandle iconTexture = app_->device_.CreateImage(
		path,
		iconImageInfo,
		vk::ImageAspectFlagBits::eColor,
		iconSamplerInfo
	);

	app_->device_.OneTimeSubmit([&](sqrp::CommandBufferHandle pCommandBuffer) {
		pCommandBuffer->TransitionLayout(iconTexture, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		pCommandBuffer->CopyBufferToImage(stagingBuffer, iconTexture);
		pCommandBuffer->TransitionLayout(iconTexture, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
		});

	return iconTexture;
}

void GuiManager::DefineGUIStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();

	// Window background
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.0f);
	// Title bar background
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // black
	// Active title bar background
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // black
	style.Colors[ImGuiCol_Header] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);

	// Button
	style.Colors[ImGuiCol_Button] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1f, 0.3f, 0.5f, 1.0f);

	// Selectable
	style.Colors[ImGuiCol_Header] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.1f, 0.3f, 0.5f, 1.0f);

	// Checkbox
	//style.Colors[ImGuiCol_CheckMark] = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

	// InputFloat3
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	//style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

GuiManager::GuiManager(App* app)
	: app_(app)
{
	sceneViewSize_ = {
		(uint32_t)(app_->windowWidth_ * sceneViewScaleX_),
		(uint32_t)(app_->windowHeight_ * sceneViewScaleY_),
		(uint32_t)(app_->windowWidth_ * sceneViewScaleX_),
		(uint32_t)(app_->windowHeight_ * sceneViewScaleY_)
	};
	inspectorViewSize_ = {
		(uint32_t)(app_->windowWidth_ - (sceneViewSize_.width)),
		(uint32_t)(app_->windowHeight_),
		(uint32_t)(app_->windowWidth_ - (sceneViewSize_.width)),
		(uint32_t)(app_->windowHeight_)
	};
	assetViewSize_ = {
		(uint32_t)(sceneViewSize_.width),
		(uint32_t)(app_->windowHeight_ - (sceneViewSize_.height)),
		(uint32_t)(sceneViewSize_.width),
		(uint32_t)(app_->windowHeight_ - (sceneViewSize_.height))
	};

	uint32_t inflightCount = app_->swapchain_->GetInflightCount();

	guiDescriptorSets_.resize(inflightCount);
	for (int i = 0; i < inflightCount; i++) {
		guiDescriptorSets_[i] = app_->device_.CreateDescriptorSet(
			"gui" + std::to_string(i),
			{
			{ app_->toneMappedImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment},
			}
		);
	}

	translateIconImage_ = CreateIcon("C:\\projects\\Vulkan\\tatter-renderer\\icon\\translate.png");
	rotationIconImage_ = CreateIcon("C:\\projects\\Vulkan\\tatter-renderer\\icon\\rotation.png");
	scaleIconImage_ = CreateIcon("C:\\projects\\Vulkan\\tatter-renderer\\icon\\scale.png");

	translateIconDescSet_ = app_->device_.CreateDescriptorSet(
		"translateIcon",
		{
		{ translateIconImage_, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment }
		}
	);

	rotationIconDescSet_ = app_->device_.CreateDescriptorSet(
		"rotationIcon",
		{
		{ rotationIconImage_, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment }
		}
	);

	scaleIconDescSet_ = app_->device_.CreateDescriptorSet(
		"scaleIcon",
		{
		{ scaleIconImage_, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment }
		}
	);

	gui_ = app_->device_.CreateGUI(app_->pWindow_, app_->swapchain_, app_->presentRenderPass_);

	DefineGUIStyle();
}

void GuiManager::DrawGui()
{
	isNeedRecreate_ = false;
	gui_->NewFrame();

	ImGuiIO& io = ImGui::GetIO();

	ImGuizmo::BeginFrame();

	ImVec2 mouse = ImGui::GetIO().MousePos;

	std::shared_ptr<ObjectData> selectedObject;
	auto it = app_->objectData_.find(selectedObjectName_);
	if (it == app_->objectData_.end()) {
		selectedObject = std::make_shared<ObjectData>(app_->device_);
	}
	else {
		selectedObject = it->second;
	}

	// 1. Scene View
	ImGuiWindowFlags sceneWindowFlag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar & (~ImGuiWindowFlags_NoResize);
	if (catchedSceneEdge_ & (GuiEdge::Left | GuiEdge::Up) || (catchedSceneEdge_ & (GuiEdge::Down | GuiEdge::Right)) == (GuiEdge::Down | GuiEdge::Right))
	{
		sceneWindowFlag |= ImGuiWindowFlags_NoResize;
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(sceneViewSize_.width, sceneViewSize_.height));
	ImGui::Begin("Scene", nullptr, sceneWindowFlag);
	ImVec2 size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
		Input::SetCatchInput(true);
	}

	ImVec2 sceneWindowPos = ImGui::GetWindowPos();
	bool onSceneLeft = fabs(mouse.x - sceneWindowPos.x) < edgeThreshold_;
	bool onSceneRight = fabs(mouse.x - (sceneWindowPos.x + size.x)) < edgeThreshold_;
	bool onSceneTop = fabs(mouse.y - sceneWindowPos.y) < edgeThreshold_;
	bool onSceneBottom = fabs(mouse.y - (sceneWindowPos.y + size.y)) < edgeThreshold_;

	catchedSceneEdge_ = GuiEdge::None;
	if (ImGui::IsMouseDown(0)) {
		if (onSceneLeft)					catchedSceneEdge_ |= GuiEdge::Left;
		if (onSceneRight)					catchedSceneEdge_ |= GuiEdge::Right;
		if (onSceneTop)						catchedSceneEdge_ |= GuiEdge::Up;
		if (onSceneBottom)					catchedSceneEdge_ |= GuiEdge::Down;
	}
	sceneViewSize_.changedWidth = size.x;
	sceneViewSize_.changedHeight = size.y;
	
	// Scene Image
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	ImTextureID texId = (ImTextureID)((VkDescriptorSet)guiDescriptorSets_[app_->swapchain_->GetCurrentInflightIndex()]->GetDescriptorSet());
	ImGui::Image(texId, contentSize);

	// File Open Button
	ImGui::SetCursorPos(ImVec2(0, 0));
	isOpenFile_ = false;
	if (ImGui::Button("File")) {
		isOpenFile_ = true;
	}
	
	// Icons
	ImGui::SetCursorPos(ImVec2(0, 30)); // Coordinates of Image
	ImTextureID translateIconTexId = (ImTextureID)((VkDescriptorSet)translateIconDescSet_->GetDescriptorSet());
	if (ImGui::ImageButton("Translate", translateIconTexId, ImVec2(32, 32))) gizmoOperation_ = ImGuizmo::TRANSLATE;
	ImTextureID rotationIconTexId = (ImTextureID)((VkDescriptorSet)rotationIconDescSet_->GetDescriptorSet());
	if (ImGui::ImageButton("Rotate", rotationIconTexId, ImVec2(32, 32)))    gizmoOperation_ = ImGuizmo::ROTATE;
	ImTextureID scaleIconTexId = (ImTextureID)((VkDescriptorSet)scaleIconDescSet_->GetDescriptorSet());
	if (ImGui::ImageButton("Scale", scaleIconTexId, ImVec2(32, 32)))     gizmoOperation_ = ImGuizmo::SCALE;

	// View Manipulate
	ImVec2 sceneWindowSize = ImGui::GetWindowSize();
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(sceneWindowPos.x, sceneWindowPos.y, sceneWindowSize.x, sceneWindowSize.y);
	float sceneTitleBarHeight = ImGui::GetFrameHeight();
	glm::mat4 view = app_->camera_.GetView();
	glm::mat4 proj = app_->camera_.GetProj();
	glm::mat4 indentityMatrix = glm::mat4(1.0f);
	ImGuizmo::ViewManipulate(glm::value_ptr(view), 1.0f, ImVec2(sceneViewSize_.width / 6.0f * 5, sceneTitleBarHeight), ImVec2(sceneViewSize_.width / 6.0f, sceneViewSize_.height / 6.0f), 0x10101010);
	// Using ImGui::GetWindowWidth() returns default value, (400, 400), when no resize
	ImVec2 gizmoPos = ImVec2(sceneViewSize_.width - sceneViewSize_.width / 6.0f, 0.0f);
	ImVec2 gizmoSize = ImVec2(sceneViewSize_.width / 6.0f, sceneViewSize_.height / 6.0f);
	MousePosition pushedMousePos = Input::GetPushedPos();
	bool isMouseOverViewCube =
		(pushedMousePos.x >= gizmoPos.x && pushedMousePos.x <= gizmoPos.x + gizmoSize.x) &&
		(pushedMousePos.y >= gizmoPos.y && pushedMousePos.y <= gizmoPos.y + gizmoSize.y);
	if (isMouseOverViewCube) {
		Input::SetCatchInput(false);
	}
	glm::mat4 invView = glm::inverse(view);
	glm::vec3 newCameraPosition = glm::vec3(invView[3]);
	glm::quat newCameraQuatRotation = glm::quat_cast(glm::mat3(invView));
	app_->camera_.SetPosition(newCameraPosition);
	app_->camera_.SetRotation(newCameraQuatRotation);

	if (app_->objectData_.size() != 0) {
		glm::mat4 model = selectedObject->GetModel();
		if (isShowGuizmo_) {
			ImGuizmo::Manipulate(
				glm::value_ptr(view), glm::value_ptr(proj), gizmoOperation_,
				ImGuizmo::MODE::LOCAL, glm::value_ptr(model)
			);
			if (ImGuizmo::IsUsing()) {
				Input::SetCatchInput(false);
			}
		}
		selectedObject->UpdateTransform(model);
	}
	ImGui::End();

	ImGui::PopStyleVar();

	// 2. Inspector view
	ImGuiWindowFlags inspectorWindowFlag = ImGuiWindowFlags_NoCollapse & (~ImGuiWindowFlags_NoResize);
	if (catchedInspectorDir_ & (GuiEdge::Right | GuiEdge::Up | GuiEdge::Down))
	{
		inspectorWindowFlag |= ImGuiWindowFlags_NoResize;
	}
	ImGui::SetNextWindowPos(ImVec2(sceneViewSize_.width, 0));
	ImGui::SetNextWindowSize(ImVec2(inspectorViewSize_.width, app_->windowHeight_));
	ImGui::Begin("Inspector", nullptr, inspectorWindowFlag);

	size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
		Input::SetCatchInput(false);
	}
	ImVec2 inspectorWindowPos = ImGui::GetWindowPos();

	bool onInspectorLeft = fabs(mouse.x - inspectorWindowPos.x) < edgeThreshold_;
	bool onInspectorRight = fabs(mouse.x - (inspectorWindowPos.x + size.x)) < edgeThreshold_;
	bool onInspectorTop = fabs(mouse.y - inspectorWindowPos.y) < edgeThreshold_;
	bool onInspectorBottom = fabs(mouse.y - (inspectorWindowPos.y + size.y)) < edgeThreshold_;

	catchedInspectorDir_ = GuiEdge::None;
	if (ImGui::IsMouseDown(0)) {
		if (onInspectorLeft)					catchedInspectorDir_ |= GuiEdge::Left;
		if (onInspectorRight)					catchedInspectorDir_ |= GuiEdge::Right;
		if (onInspectorTop)						catchedInspectorDir_ |= GuiEdge::Up;
		if (onInspectorBottom)					catchedInspectorDir_ |= GuiEdge::Down;
	}
	inspectorViewSize_.changedWidth = size.x;
	inspectorViewSize_.changedHeight = size.y;

	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 displayRotation = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(0.0f);
	if (app_->objectData_.size() != 0) {
		position = selectedObject->GetPosition();
		displayRotation = glm::degrees(selectedObject->GetRotation());
		scale = selectedObject->GetScale();
	}
	auto NormalizeAngle = [](float angle) {
		if (fabs(angle) < 0.01f) angle = 0.0f;
		while (angle > 180.0f) angle -= 360.0f;
		while (angle < -180.0f) angle += 360.0f;
		// Rounding near 180 degrees
		if (fabs(angle - 180.0f) < 0.01f || fabs(angle + 180.0f) < 0.01f)
			angle = 180.0f;
		return angle;
		};
	displayRotation.x = NormalizeAngle(displayRotation.x);
	displayRotation.y = NormalizeAngle(displayRotation.y);
	displayRotation.z = NormalizeAngle(displayRotation.z);

	glm::vec3 displayCameraRotation = app_->camera_.GetRotation();
	displayCameraRotation.x = NormalizeAngle(displayCameraRotation.x);
	displayCameraRotation.y = NormalizeAngle(displayCameraRotation.y);
	displayCameraRotation.z = NormalizeAngle(displayCameraRotation.z);
	if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("Rendering", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);

		if (ImGui::Selectable("Forward", renderMode_ == 0, 0, ImVec2(inspectorViewSize_.changedWidth / 3, 0))) renderMode_ = 0;
		ImGui::SameLine();
		if (ImGui::Selectable("Deferred", renderMode_ == 1, 0, ImVec2(inspectorViewSize_.changedWidth / 3, 0)))    renderMode_ = 1;

		ImGui::EndChild();
	}
	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("CameraChild", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);
		glm::vec3 position = app_->camera_.GetPos();
		glm::vec3 rotation = app_->camera_.GetRotation();
		if (ImGui::InputFloat3("Position", glm::value_ptr(position))) {
			app_->camera_.SetPosition(position);
		}
		if (ImGui::InputFloat3("Rotation", glm::value_ptr(displayCameraRotation))) {
			if (!ImGui::IsItemActive())
			{
				glm::vec3 normalizedRotation;
				normalizedRotation.x = NormalizeAngle(displayCameraRotation.x);
				normalizedRotation.y = NormalizeAngle(displayCameraRotation.y);
				normalizedRotation.z = NormalizeAngle(displayCameraRotation.z);
				// Snapping 90 degrees
				auto Snap = [](float angle) {
					if (fabs(angle - 90.0f) < 0.01f) return 89.9f;
					if (fabs(angle + 90.0f) < 0.01f) return -89.9f;
					return angle;
					};
				normalizedRotation.x = Snap(normalizedRotation.x);
				normalizedRotation.y = Snap(normalizedRotation.y);
				normalizedRotation.z = Snap(normalizedRotation.z);

				glm::vec3 radiansRot = glm::radians(normalizedRotation);
				glm::vec3 up = app_->camera_.GetUp();
				glm::vec3 right = app_->camera_.GetRight();
				glm::vec3 front = app_->camera_.GetFront();
				glm::quat qYaw = glm::angleAxis(radiansRot.y, glm::vec3(0, 1, 0));
				glm::quat qPitch = glm::angleAxis(radiansRot.x, glm::vec3(1, 0, 0));
				glm::quat qRoll = glm::angleAxis(radiansRot.z, glm::vec3(0, 0, -1));

				//// NOTE : define the order of rotation here
				glm::quat q = qYaw * qPitch * qRoll; // Yaw -> Pitch -> Roll
				app_->camera_.SetRotation(q);
			}
		}
		ImGui::EndChild();
	}
	if (ImGui::CollapsingHeader("Object", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("ObjectChild", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);
		std::vector<const char*> itemPtrs;
		for (int i = 0; i < app_->objectNames_.size(); i++) {
			if (app_->objectNames_[i] == selectedObjectName_) {
				selectedObjectIndex_ = i;
			}
			itemPtrs.push_back(app_->objectNames_[i].c_str());
		}
		if (app_->objectNames_.size() == 0) {
			selectedObjectName_ = "";
		}
		if (ImGui::Combo("Select Model", &selectedObjectIndex_, itemPtrs.data(), static_cast<int>(itemPtrs.size())))
		{
			if (app_->objectNames_.size() != 0) {
				selectedObjectName_ = app_->objectNames_[selectedObjectIndex_];
			}
		}
		ImGui::Checkbox("Show Guizmo", &isShowGuizmo_);
		if (ImGui::InputFloat3("Position", glm::value_ptr(position))) {
			if (app_->objectNames_.size() != 0) {
				selectedObject->SetPosition(position);
			}
		}
		ImGui::InputFloat3("Rotation", glm::value_ptr(displayRotation));
		if (!ImGui::IsItemActive() && isModifiedRotation_)
		{
			if (app_->objectNames_.size() != 0) {
				glm::vec3 normalizedRotation;
				normalizedRotation.x = NormalizeAngle(displayRotation.x);
				normalizedRotation.y = NormalizeAngle(displayRotation.y);
				normalizedRotation.z = NormalizeAngle(displayRotation.z);
				selectedObject->SetRotation(glm::quat(glm::radians(normalizedRotation)));
				isModifiedRotation_ = false;
			}
		}
		if (ImGui::IsItemActive())
		{
			isModifiedRotation_ = true;
		}
		if (ImGui::InputFloat3("Scale", glm::value_ptr(scale)))
		{
			scale = glm::max(scale, glm::vec3(0.01f));
			if (app_->objectNames_.size() != 0) {
				selectedObject->SetScale(scale);
			}
		}
		deletedModelName_ = "";
		if (ImGui::Button("Delete")) {
			cout << "Delete Model: " << selectedObjectName_ << endl;
			deletedModelName_ = selectedObjectName_;
			isNeedRecreate_ = true;
		}

		ImGui::EndChild();
	}
	if (ImGui::CollapsingHeader("EnvironmentMap", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("EnvironmentMap", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);
		std::vector<const char*> itemPtrs;
		for (const auto& envMapName : app_->envMapNames_)
			itemPtrs.push_back(envMapName.c_str());
		int beforeEnvMapIndex = selectedEnvMapIndex_;
		isChangedEnvMap_ = false;
		if (ImGui::Combo("Select EnvMap", &selectedEnvMapIndex_, itemPtrs.data(), static_cast<int>(itemPtrs.size())))
		{
			if (beforeEnvMapIndex != selectedEnvMapIndex_) {
				isChangedEnvMap_ = true;
				isNeedRecreate_ = true;
			}
			selectedEnvMapName_ = "";
			if (app_->envMapNames_.size() != 0) {
				selectedEnvMapName_ = app_->envMapNames_[selectedEnvMapIndex_];
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();

	// 3. Asset view 
	ImGuiWindowFlags assetWindowFlag = ImGuiWindowFlags_NoCollapse & (~ImGuiWindowFlags_NoResize);
	if (catchedAssetDir_ & (GuiEdge::Left | GuiEdge::Down))
	{
		assetWindowFlag |= ImGuiWindowFlags_NoResize;
	}
	ImGui::SetNextWindowPos(ImVec2(0, sceneViewSize_.height));
	ImGui::SetNextWindowSize(ImVec2(assetViewSize_.width, assetViewSize_.height));
	ImGui::Begin("Asset", nullptr, assetWindowFlag);
	size = ImGui::GetWindowSize();
	if (ImGui::IsWindowHovered()) {
		Input::SetCatchInput(false);
	}

	ImVec2 assetWindowPos = ImGui::GetWindowPos();

	bool onAssetLeft = fabs(mouse.x - assetWindowPos.x) < edgeThreshold_;
	bool onAssetRight = fabs(mouse.x - (assetWindowPos.x + size.x)) < edgeThreshold_;
	bool onAssetTop = fabs(mouse.y - assetWindowPos.y) < edgeThreshold_;
	bool onAssetBottom = fabs(mouse.y - (assetWindowPos.y + size.y)) < edgeThreshold_;

	catchedAssetDir_ = GuiEdge::None;
	if (ImGui::IsMouseDown(0)) {
		if (onAssetLeft)					catchedAssetDir_ |= GuiEdge::Left;
		if (onAssetRight)					catchedAssetDir_ |= GuiEdge::Right;
		if (onAssetTop)						catchedAssetDir_ |= GuiEdge::Up;
		if (onAssetBottom)					catchedAssetDir_ |= GuiEdge::Down;
	}
	size = ImGui::GetWindowSize();
	assetViewSize_.changedWidth = size.x;
	assetViewSize_.changedHeight = size.y;
	ImGui::Columns(2, nullptr, false);
	if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("ModelChild", ImVec2(size.x / 2, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);

		addedModelName_ = "";
		for (const auto& [modelName, modelData] : app_->models_) {
			ImGui::Text(modelName.c_str());
			ImGui::SameLine();
			ImVec2 childWindowSize = ImGui::GetWindowSize();
			ImGui::SetCursorPosX(childWindowSize.x - 50);
			if (ImGui::Button(("Add##" + modelName).c_str()))
			{
				cout << "Add Model: " << modelName << endl;
				addedModelName_ = modelName;
				isNeedRecreate_ = true;
			}
		}

		ImGui::EndChild();
	}
	ImGui::NextColumn();
	if (ImGui::CollapsingHeader("EnvironmentMap", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("EnvironmentMapChild", ImVec2(size.x / 2, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);
		for (const auto& envMapName: app_->envMapNames_) {
			ImGui::Text(envMapName.c_str());
		}

		ImGui::EndChild();
	}
	ImGui::End();

	if (sceneViewSize_.changedWidth != sceneViewSize_.width) {
		sceneViewSize_.width = sceneViewSize_.changedWidth;
		inspectorViewSize_.width = app_->windowWidth_ - sceneViewSize_.width;
		assetViewSize_.width = sceneViewSize_.width;
		isNeedRecreate_ = true;
	}
	else if (sceneViewSize_.changedHeight != sceneViewSize_.height) {
		sceneViewSize_.height = sceneViewSize_.changedHeight;
		assetViewSize_.height = app_->windowHeight_ - sceneViewSize_.height;
		isNeedRecreate_ = true;
	}
	else if (inspectorViewSize_.changedWidth != inspectorViewSize_.width) {
		inspectorViewSize_.width = inspectorViewSize_.changedWidth;
		sceneViewSize_.width = app_->windowWidth_ - inspectorViewSize_.width;
		assetViewSize_.width = sceneViewSize_.width;
		isNeedRecreate_ = true;
	}
	else if (assetViewSize_.changedHeight != assetViewSize_.height) {
		assetViewSize_.height = assetViewSize_.changedHeight;
		sceneViewSize_.height = app_->windowHeight_ - assetViewSize_.height;
		sceneViewSize_.width = assetViewSize_.width;
		isNeedRecreate_ = true;
	}
	else if (assetViewSize_.changedWidth != assetViewSize_.width) {
		assetViewSize_.width = assetViewSize_.changedWidth;
		sceneViewSize_.width = assetViewSize_.width;
		inspectorViewSize_.width = app_->windowWidth_ - sceneViewSize_.width;
		isNeedRecreate_ = true;
	}
}

void GuiManager::UpdateGUISize()
{
	sceneViewSize_.width = app_->windowWidth_ * sceneViewScaleX_;
	sceneViewSize_.height = app_->windowHeight_ * sceneViewScaleY_;
	sceneViewSize_.changedWidth = sceneViewSize_.width;
	sceneViewSize_.changedHeight = sceneViewSize_.height;
	inspectorViewSize_.width = app_->windowWidth_ * (1.0f - sceneViewScaleX_);
	inspectorViewSize_.height = app_->windowHeight_;
	inspectorViewSize_.changedWidth = inspectorViewSize_.width;
	inspectorViewSize_.changedHeight = inspectorViewSize_.height;
	assetViewSize_.width = app_->windowWidth_ * sceneViewScaleX_;
	assetViewSize_.height = app_->windowHeight_ * (1.0f - sceneViewScaleY_);
	assetViewSize_.changedWidth = assetViewSize_.width;
	assetViewSize_.changedHeight = assetViewSize_.height;
}

void GuiManager::Recreate()
{
	guiDescriptorSets_.clear();
	guiDescriptorSets_.resize(app_->swapchain_->GetInflightCount());
	for (int i = 0; i < app_->swapchain_->GetInflightCount(); i++) {
		guiDescriptorSets_[i] = app_->device_.CreateDescriptorSet(
			"gui" + std::to_string(i),
			{
			{ app_->toneMappedImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment},
			}
		);
	}
}

GUIHandle GuiManager::GetGui()
{
	return gui_;
}

GuiWindowSize GuiManager::GetSceneViewSize()
{
	return sceneViewSize_;
}

GuiWindowSize GuiManager::GetInspectorViewSize()
{
	return inspectorViewSize_;
}

GuiWindowSize GuiManager::GetAssetViewSize()
{
	return assetViewSize_;
}

std::string GuiManager::GetSelectedObjectName()
{
	return selectedObjectName_;
}

std::string GuiManager::GetSelectedEnvMapName()
{
	return selectedEnvMapName_;
}

bool GuiManager::GetIsNeedRecreate()
{
	return isNeedRecreate_;
}

int GuiManager::GetRenderMode()
{
	return renderMode_;
}

std::string GuiManager::GetAddedModelName()
{
	return addedModelName_;
}

std::string GuiManager::GetDeletedModelName()
{
	return deletedModelName_;
}

bool GuiManager::IsChangedEnvMap()
{
	return isChangedEnvMap_;
}

bool GuiManager::IsOpenFile()
{
	return isOpenFile_;
}

void GuiManager::SetSelectedObjectName(std::string name)
{
	selectedObjectName_ = name;
}

void GuiManager::SetSelectedEnvMapName(std::string name)
{
	selectedEnvMapName_ = name;
}
