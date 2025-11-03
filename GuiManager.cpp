#include "GuiManager.hpp"

#include "App.hpp"

using namespace std;
using namespace sqrp;

sqrp::ImageHandle GuiManager::CreateIcon(std::string path)
{
	int texWidth = 0, texHeight = 0, texChannels = 0;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		fprintf(stderr, "Failed to load image %s\n", path);
		throw std::runtime_error("Failed to load image");
	}

	std::vector<uint8_t> data(pixels, pixels + texWidth * texHeight * 4); // RGBA
	/*for (int i = 0; i < texWidth * texHeight * 4; i++) {
		data[i] = pixels[i] / 255.0f;
	}*/
	stbi_image_free(pixels);


	sqrp::BufferHandle stagingBuffer = app_->device_.CreateBuffer(texWidth * texHeight * 4 * sizeof(uint8_t), vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);
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

	// 全体の色を取得して変更
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.0f);   // ウィンドウ背景
	// 通常のタイトルバー背景
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // 緑

	// アクティブなウィンドウのタイトルバー背景
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // 少し明るい緑
	style.Colors[ImGuiCol_Header] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);

	// ボタン
	style.Colors[ImGuiCol_Button] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);        // 通常
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f); // ホバー
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1f, 0.3f, 0.5f, 1.0f);  // 押下中

	// Selectable
	style.Colors[ImGuiCol_Header] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);           // 通常
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f);    // ホバー
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.1f, 0.3f, 0.5f, 1.0f);     // 選択中

	// Checkbox (チェックボックス)
	//style.Colors[ImGuiCol_CheckMark] = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);       // チェックマークの色
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);         // 背景
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);  // ホバー
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);   // 押下中

	// InputFloat3（入力欄）
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.05859375f, 0.1015625f, 0.26953125f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	//style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // テキスト色
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
	cout << "sceneViewSize_ initialize : " << sceneViewSize_.width << " " << sceneViewSize_.height << " " << sceneViewSize_.changedWidth << " " << sceneViewSize_.changedHeight << endl;
	inspectorViewSize_ = {
		(uint32_t)(app_->windowWidth_ - (sceneViewSize_.width)),
		(uint32_t)(app_->windowHeight_),
		(uint32_t)(app_->windowWidth_ * (sceneViewSize_.width)),
		(uint32_t)(app_->windowHeight_)
	};
	cout << "inspectorViewSize_ initialize : " << inspectorViewSize_.width << " " << inspectorViewSize_.height << " " << inspectorViewSize_.changedWidth << " " << inspectorViewSize_.changedHeight << endl;
	assetViewSize_ = {
		(uint32_t)(sceneViewSize_.width),
		(uint32_t)(app_->windowHeight_ - (sceneViewSize_.height)),
		(uint32_t)(sceneViewSize_.width),
		(uint32_t)(app_->windowHeight_ - (sceneViewSize_.height))
	};

	uint32_t inflightCount = app_->swapchain_->GetInflightCount();

	guiDescriptorSets_.resize(inflightCount);
	for (int i = 0; i < inflightCount; i++) {
		guiDescriptorSets_[i] = app_->device_.CreateDescriptorSet({
			{ app_->toneMappedImages_[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment},
			}
		);
	}

	translateIconImage_ = CreateIcon("C:\\projects\\Vulkan\\tatter-renderer\\icon\\translate.png");
	rotationIconImage_ = CreateIcon("C:\\projects\\Vulkan\\tatter-renderer\\icon\\rotation.png");
	scaleIconImage_ = CreateIcon("C:\\projects\\Vulkan\\tatter-renderer\\icon\\scale.png");

	translateIconDescSet_ = app_->device_.CreateDescriptorSet({
		{ translateIconImage_, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment }
		}
	);

	rotationIconDescSet_ = app_->device_.CreateDescriptorSet({
		{ rotationIconImage_, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment }
		}
	);

	scaleIconDescSet_ = app_->device_.CreateDescriptorSet({
		{ scaleIconImage_, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment }
		}
	);

	gui_ = app_->device_.CreateGUI(app_->pWindow_, app_->swapchain_, app_->presentRenderPass_);

	DefineGUIStyle();
}

void GuiManager::DrawGui()
{
	/*isChangedSceneSize_ = false;
	if (sceneViewSize_.changedWidth != sceneViewSize_.width) {
		sceneViewSize_.width = sceneViewSize_.changedWidth;
		inspectorViewSize_.width = app_->windowWidth_ - sceneViewSize_.width;
		filePanelSize_.width = sceneViewSize_.width;
		isChangedSceneSize_ = true;
	}
	if (sceneViewSize_.changedHeight != sceneViewSize_.height) {
		sceneViewSize_.height = sceneViewSize_.changedHeight;
		filePanelSize_.height = app_->windowHeight_ - sceneViewSize_.height;
		isChangedSceneSize_ = true;
	}
	if (inspectorViewSize_.changedWidth != inspectorViewSize_.width) {
		inspectorViewSize_.width = inspectorViewSize_.changedWidth;
		sceneViewSize_.width = app_->windowWidth_ - inspectorViewSize_.width;
		filePanelSize_.width = sceneViewSize_.width;
		filePanelSize_.height = app_->windowHeight_ - sceneViewSize_.height;
		isChangedSceneSize_ = true;
	}
	if (filePanelSize_.changedHeight != filePanelSize_.height) {
		filePanelSize_.height = filePanelSize_.changedHeight;
		sceneViewSize_.height = app_->windowHeight_ - filePanelSize_.height;
		sceneViewSize_.width = filePanelSize_.width;
		inspectorViewSize_.width = app_->windowWidth_ - sceneViewSize_.width;
		isChangedSceneSize_ = true;
	}
	if (filePanelSize_.changedWidth != filePanelSize_.width) {
		filePanelSize_.width = filePanelSize_.changedWidth;
		sceneViewSize_.width = filePanelSize_.width;
		inspectorViewSize_.width = app_->windowWidth_ - sceneViewSize_.width;
		isChangedSceneSize_ = true;
	}*/

	gui_->NewFrame();

	ImGuiIO& io = ImGui::GetIO();

	ImGuizmo::BeginFrame();

	ImVec2 mouse = ImGui::GetIO().MousePos;

	// 1. Scene View
	ImGuiWindowFlags sceneWindowFlag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar;
	/*if (catchSceneDir_ & GuiDir::Left || catchSceneDir_ & GuiDir::Up || catchSceneDir_ & (GuiDir::Down | GuiDir::Left) || catchSceneDir_ & (GuiDir::Down | GuiDir::Right))
	{
		sceneWindowFlag |= ImGuiWindowFlags_NoResize;
	}*/
	/*if (catchSceneDir_ & GuiDir::Right) {
		cout << "Catch scene right." << endl;
	}*/
	if (catchedSceneEdge_ & (GuiEdge::Left | GuiEdge::Up) || (catchedSceneEdge_ & (GuiEdge::Down | GuiEdge::Right)) == (GuiEdge::Down | GuiEdge::Right))
	{
		if (catchedSceneEdge_ & GuiEdge::Left)
		{
			cout << "Catch scene left." << endl;
		}
		if (catchedSceneEdge_ & GuiEdge::Right) {
			cout << "Catch scene right." << endl;
		}
		if (catchedSceneEdge_ & GuiEdge::Up) {
			cout << "Catch scene up." << endl;
		}
		if (catchedSceneEdge_ & GuiEdge::Down) {
			cout << "Catch scene down." << endl;
		}
		sceneWindowFlag |= ImGuiWindowFlags_NoResize;
		cout << "Scene No Resize." << endl;
	}
	else
	{
		sceneWindowFlag &= ~ImGuiWindowFlags_NoResize;
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
	cout << "sceneVIewSize_.width: " << sceneViewSize_.width << ", sceneViewSize_.height: " << sceneViewSize_.height << endl;
	cout << "sceneViewSize_.changedWidth: " << sceneViewSize_.changedWidth << ", sceneViewSize_.changedHeight: " << sceneViewSize_.changedHeight << endl;
	if (sceneViewSize_.width != sceneViewSize_.changedWidth || sceneViewSize_.height != sceneViewSize_.changedHeight) {
		//for (int testi = 0; testi < 20; testi++) {
			cout << "        Scene View Size: " << sceneViewSize_.width << ", " << sceneViewSize_.height << endl;
			cout << "Changed Scene View Size: " << sceneViewSize_.changedWidth << ", " << sceneViewSize_.changedHeight << endl;
		//}
	}
	
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	ImTextureID texId = (ImTextureID)((VkDescriptorSet)guiDescriptorSets_[app_->swapchain_->GetCurrentInflightIndex()]->GetDescriptorSet());
	ImGui::Image(texId, contentSize); // Draw scene image

	ImGui::SetCursorPos(ImVec2(0, 0));
	std::string selectedFile = "";
	if (ImGui::Button("File")) {
		// auto-freeing memory
		NFD::UniquePath outPath;

		// prepare filters for the dialog
		nfdfilteritem_t filterItem[2] = { {"glTF file", "gltf"}, {"hdr file", "hdr"} };

		// show the dialog
		nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 2);
		if (result == NFD_OKAY) {
			std::cout << "Success!" << std::endl << outPath.get() << std::endl;
			std::string filePath = outPath.get();
			if (filePath.ends_with(".gltf") || filePath.ends_with(".GLTF")) {
				std::filesystem::path pathObj(filePath);
				newModelPath_ = pathObj.filename().string();
				isNeedReloadModel_ = true;
			}
			else if (filePath.ends_with(".hdr") || filePath.ends_with(".HDR")) {
				// HDR テクスチャ読み込み処理
				// 例: stb_image などで読み込む
				std::filesystem::path pathObj(filePath);
				newEnvMapPath_ = pathObj.filename().string();
				isNeedReloadEnvMap_ = true;
			}
		}
		else if (result == NFD_CANCEL) {
			std::cout << "User pressed cancel." << std::endl;
		}
		else {
			std::cout << "Error: " << NFD::GetError() << std::endl;
		}
	}
	else {
		isNeedReloadModel_ = false;
		isNeedReloadEnvMap_ = false;
	}
	
	// Draw icons
	ImGui::SetCursorPos(ImVec2(0, 30)); // Image 内の座標
	ImTextureID translateIconTexId = (ImTextureID)((VkDescriptorSet)translateIconDescSet_->GetDescriptorSet());
	if (ImGui::ImageButton("Translate", translateIconTexId, ImVec2(32, 32))) gizmoOperation_ = ImGuizmo::TRANSLATE;
	ImTextureID rotationIconTexId = (ImTextureID)((VkDescriptorSet)rotationIconDescSet_->GetDescriptorSet());
	if (ImGui::ImageButton("Rotate", rotationIconTexId, ImVec2(32, 32)))    gizmoOperation_ = ImGuizmo::ROTATE;
	ImTextureID scaleIconTexId = (ImTextureID)((VkDescriptorSet)scaleIconDescSet_->GetDescriptorSet());
	if (ImGui::ImageButton("Scale", scaleIconTexId, ImVec2(32, 32)))     gizmoOperation_ = ImGuizmo::SCALE;

	ImVec2 sceneWindowSize = ImGui::GetWindowSize();
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(sceneWindowPos.x, sceneWindowPos.y, sceneWindowSize.x, sceneWindowSize.y);
	float sceneTitleBarHeight = ImGui::GetFrameHeight();
	glm::mat4 view = app_->camera_.GetView();
	glm::mat4 proj = app_->camera_.GetProj();
	glm::mat4 indentityMatrix = glm::mat4(1.0f);
	ImGuizmo::ViewManipulate(glm::value_ptr(view), 1.0f, ImVec2(sceneViewSize_.width / 6.0f * 5, sceneTitleBarHeight), ImVec2(sceneViewSize_.width / 6.0f, sceneViewSize_.height / 6.0f), 0x10101010);
	// Using ImGui::GetWindowWidth() returns default value, (400, 400), when no resize
	//ImVec2 gizmoPos = ImVec2(ImGui::GetWindowPos().x + sceneViewSize_.width - sceneViewSize_.width / 6.0f, ImGui::GetWindowPos().y);
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
		glm::mat4 model = app_->objectData_.at(selectedObjectName_)->GetModel();
		if (isShowGuizmo_) {
			ImGuizmo::Manipulate(
				glm::value_ptr(view), glm::value_ptr(proj), gizmoOperation_,
				ImGuizmo::MODE::LOCAL, glm::value_ptr(model)
			);
			if (ImGuizmo::IsUsing()) {
				Input::SetCatchInput(false);
			}
		}
		app_->objectData_.at(selectedObjectName_)->UpdateTransform(model);
	}
	ImGui::End();

	ImGui::PopStyleVar();

	// 2. Inspector view
	ImGuiWindowFlags inspectorWindowFlag = ImGuiWindowFlags_NoCollapse;
	/*if ((catchPanelDir_ & GuiDir::Right) || (catchPanelDir_ & GuiDir::Up) || (catchPanelDir_ & GuiDir::Down))
	{
		panelWindowFlag |= ImGuiWindowFlags_NoResize;
	}*/
	if (catchedInspectorDir_ & (GuiEdge::Right | GuiEdge::Up | GuiEdge::Down))
	{
		inspectorWindowFlag |= ImGuiWindowFlags_NoResize;
		cout << "Panel No Resize." << endl;
	}
	else
	{
		inspectorWindowFlag &= ~ImGuiWindowFlags_NoResize;
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
	if (inspectorViewSize_.width != inspectorViewSize_.changedWidth || inspectorViewSize_.height != inspectorViewSize_.changedHeight) {
		//for (int testi = 0; testi < 20; testi++) {
			cout << "        Inspector View Size: " << inspectorViewSize_.width << ", " << inspectorViewSize_.height << endl;
			cout << "Changed Inspector View Size: " << inspectorViewSize_.changedWidth << ", " << inspectorViewSize_.changedHeight << endl;
		//}
	}

	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 displayRotation = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(0.0f);
	if (app_->objectData_.size() != 0) {
		position = app_->objectData_.at(selectedObjectName_)->GetPosition();
		displayRotation = glm::degrees(app_->objectData_.at(selectedObjectName_)->GetRotation());
		scale = app_->objectData_.at(selectedObjectName_)->GetScale();
	}
	auto NormalizeAngle = [](float angle) {
		if (fabs(angle) < 0.01f) angle = 0.0f;
		while (angle > 180.0f) angle -= 360.0f;
		while (angle < -180.0f) angle += 360.0f;
		// ±180 の境界を丸める
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
		if (ImGui::Selectable("Differed", renderMode_ == 1, 0, ImVec2(inspectorViewSize_.changedWidth / 3, 0)))    renderMode_ = 1;

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
				// 90度や180度付近は明示的にスナップ
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

				//// 回転順序を定義（重要）
				glm::quat q = qYaw * qPitch * qRoll; // Yaw→Pitch→Roll の順
				app_->camera_.SetRotation(q);
				//app_->camera_.SetRotation(glm::quat(glm::radians(normalizedRotation)));
			}
		}
		ImGui::EndChild();
	}
	if (ImGui::CollapsingHeader("Object", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("ObjectChild", ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);
		std::vector<const char*> itemPtrs;
		for (const auto& objectName : app_->objectNames_)
			itemPtrs.push_back(objectName.c_str());
		if (app_->objectNames_.size() == 0) {
			selectedObjectIndex_ = -1;
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
				app_->objectData_.at(selectedObjectName_)->SetPosition(position);
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
				app_->objectData_.at(selectedObjectName_)->SetRotation(glm::quat(glm::radians(normalizedRotation)));
				isModifiedRotation_ = false;
			}
		}
		if (ImGui::IsItemActive())
		{
			isModifiedRotation_ = true;
		}
		if (ImGui::InputFloat3("Scale", glm::value_ptr(scale)))
		{
			if (app_->objectNames_.size() != 0) {
				app_->objectData_.at(selectedObjectName_)->SetScale(scale);
			}
		}
		if (ImGui::Button("Delete")) {
			cout << "Delete Model: " << selectedObjectName_ << endl;
			deletedModelName_ = selectedObjectName_;
		}
		else {
			isNeedRecreate_ = false;
			deletedModelName_ = "";
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
		if (ImGui::Combo("Select EnvMap", &selectedEnvMapIndex_, itemPtrs.data(), static_cast<int>(itemPtrs.size())))
		{
			if (beforeEnvMapIndex != selectedEnvMapIndex_) {
				isChangedEnvMap_ = true;
			}
			else {
				isChangedEnvMap_ = false;
			}
			if (app_->envMapNames_.size() != 0) {
				selectedEnvMapName_ = app_->envMapNames_[selectedEnvMapIndex_];
			}
			else {
				selectedEnvMapIndex_ = -1;
				selectedEnvMapName_ = "";
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();

	// Asset view 
	ImGuiWindowFlags assetWindowFlag = ImGuiWindowFlags_NoCollapse;
	if (catchedAssetDir_ & (GuiEdge::Left | GuiEdge::Down))
	{
		cout << "No Resizing File Panel" << endl;
		assetWindowFlag |= ImGuiWindowFlags_NoResize;
		cout << "File Panel No Resize." << endl;
	}
	else
	{
		assetWindowFlag &= ~ImGuiWindowFlags_NoResize;
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
	if (assetViewSize_.width != assetViewSize_.changedWidth || assetViewSize_.height != assetViewSize_.changedHeight) {
		//for (int testi = 0; testi < 20; testi++) {
			cout << "        Asset View Size: " << assetViewSize_.width << ", " << assetViewSize_.height << endl;
			cout << "Changed Asset View Size: " << assetViewSize_.changedWidth << ", " << assetViewSize_.changedHeight << endl;
		//}
	}
	ImGui::Columns(2, nullptr, false);
	if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("ModelChild", ImVec2(size.x / 2, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);

		isNeedRecreate_ = false;
		addedModelName_ = "";
		for (const auto& [modelName, modelData] : app_->models_) {
			ImGui::Text(modelName.c_str());
			ImGui::SameLine();
			ImVec2 childWindowSize = ImGui::GetWindowSize();
			ImGui::SetCursorPosX(childWindowSize.x - 50);
			//cout << "modelName: " << modelName << endl;
			if (ImGui::Button(("Add##" + modelName).c_str()))
			{
				cout << "Add Model: " << modelName << endl;
				addedModelName_ = modelName;
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

	isChangedSceneSize_ = false;
	if (sceneViewSize_.changedWidth != sceneViewSize_.width) {
		sceneViewSize_.width = sceneViewSize_.changedWidth;
		inspectorViewSize_.width = app_->windowWidth_ - sceneViewSize_.width;
		assetViewSize_.width = sceneViewSize_.width;
		isChangedSceneSize_ = true;
	}
	else if (sceneViewSize_.changedHeight != sceneViewSize_.height) {
		sceneViewSize_.height = sceneViewSize_.changedHeight;
		assetViewSize_.height = app_->windowHeight_ - sceneViewSize_.height;
		isChangedSceneSize_ = true;
	}
	else if (inspectorViewSize_.changedWidth != inspectorViewSize_.width) {
		inspectorViewSize_.width = inspectorViewSize_.changedWidth;
		sceneViewSize_.width = app_->windowWidth_ - inspectorViewSize_.width;
		assetViewSize_.width = sceneViewSize_.width;
		isChangedSceneSize_ = true;
	}
	else if (assetViewSize_.changedHeight != assetViewSize_.height) {
		assetViewSize_.height = assetViewSize_.changedHeight;
		sceneViewSize_.height = app_->windowHeight_ - assetViewSize_.height;
		sceneViewSize_.width = assetViewSize_.width;
		isChangedSceneSize_ = true;
	}
	else if (assetViewSize_.changedWidth != assetViewSize_.width) {
		assetViewSize_.width = assetViewSize_.changedWidth;
		sceneViewSize_.width = assetViewSize_.width;
		inspectorViewSize_.width = app_->windowWidth_ - sceneViewSize_.width;
		isChangedSceneSize_ = true;
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
		guiDescriptorSets_[i] = app_->device_.CreateDescriptorSet({
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

int GuiManager::GetSelectedObjectIndex()
{
	return selectedObjectIndex_;
}

std::string GuiManager::GetSelectedObjectName()
{
	return selectedObjectName_;
}

std::string GuiManager::GetSelectedEnvMapName()
{
	return selectedEnvMapName_;
}

bool GuiManager::IsChangedSceneSize()
{
	return isChangedSceneSize_;
}

bool GuiManager::IsNeedRecreate()
{
	return isNeedRecreate_;
}

bool GuiManager::IsNeedReloadModel()
{
	return isNeedReloadModel_;
}

bool GuiManager::IsNeedReloadEnvMap()
{
	return isNeedReloadEnvMap_;
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

std::string GuiManager::GetNewModelPath()
{
	return newModelPath_;
}

std::string GuiManager::GetNewEnvMapPath()
{
	return newEnvMapPath_;
}

void GuiManager::SetSelectedObjectIndex(int index)
{
	selectedObjectIndex_ = index;
}

void GuiManager::SetSelectedObjectName(std::string name)
{
	selectedObjectName_ = name;
}

void GuiManager::SetSelectedEnvMapName(std::string name)
{
	selectedEnvMapName_ = name;
}

void GuiManager::SetIsChangedEnvMap(bool isChanged)
{
	isChangedEnvMap_ = isChanged;
}