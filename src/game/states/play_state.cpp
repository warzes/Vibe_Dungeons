#include "stdafx.h"
#include "game/states/play_state.h"
#include "engine/delta_time.h"
#include "engine/input_manager.h"
#include "engine/window.h"
#include "engine/resource_manager.h"
#include "engine/renderer/shader_sources.h"
#include "core/logger.h"
#include "core/exception.h"
#include "core/collision.h"
#include "engine/renderer/renderer.h"

// ── Cube vertex data ──────────────────────────────────────────────────────
// Each face: 4 vertices in BL->BR->TR->TL order when viewed from outside.
// Indices: 0,1,2, 0,2,3 -> CCW for all faces.
static constexpr float CUBE_VERTICES[] = {
	// back face (z=-0.5)  -- looking from -z: x-right, y-up
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
	// front face (z=+0.5) -- looking from +z: x-flipped, y-up
	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
	// left face (x=-0.5)  -- looking from -x: z-flipped, y-up
	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
	// right face (x=+0.5) -- looking from +x: z-right, y-up
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
	// bottom face (y=-0.5) -- looking from -y: x-flipped, z-up
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
	// top face (y=+0.5)   -- looking from +y: x-right, z-up
	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
};

static constexpr uint32_t CUBE_INDICES[] = {
	0,2,1, 0,3,2,
	4,6,5, 4,7,6,
	8,10,9, 8,11,10,
	12,14,13, 12,15,14,
	16,18,17, 16,19,18,
	20,22,21, 20,23,22,
};

// ── Shader sources (shader_sources.h) ─────────────────────────────────────
// Instance model matrix at locations 3-6 (divisor=1) for instancing.

// ── Helpers ───────────────────────────────────────────────────────────────

static glm::mat4 makeModel(
	const glm::vec3& position,
	float angleDeg,
	const glm::vec3& axis
) noexcept
{
	glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
	return glm::rotate(m, glm::radians(angleDeg), axis);
}

// ── PlayState implementation ──────────────────────────────────────────────

PlayState::PlayState(
	GameStateMachine& machine,
	const Window& window,
	InputManager& input,
	ResourceManager& resources
) noexcept
	: m_machine(machine)
	, m_window(window)
	, m_input(input)
	, m_resources(resources)
{}

PlayState::~PlayState() noexcept = default;

void PlayState::OnEnter() noexcept
{
	// Reset all state from previous session
	m_input.ResetState();
	m_collidingCubes = 0;
	m_renderer = nullptr;
	m_debugSphere = Sphere{glm::vec3(0.0f), 2.5f};

	m_debugRenderer.Init();

	try
	{
		// Reset camera to initial position/rotation
		m_camera.SetPosition(glm::vec3(5.0f, 4.0f, 6.0f));
		m_camera.SetRotation(-90.0f, 0.0f);
		m_camera.UpdateViewMatrix();

		// Register input actions
		m_input.BindAction("MoveForward",  SDL_SCANCODE_W);
		m_input.BindAction("MoveBackward", SDL_SCANCODE_S);
		m_input.BindAction("MoveLeft",     SDL_SCANCODE_A);
		m_input.BindAction("MoveRight",    SDL_SCANCODE_D);
		m_input.BindAction("MoveUp",       SDL_SCANCODE_SPACE);
		m_input.BindAction("MoveDown",     SDL_SCANCODE_LSHIFT);

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		constexpr size_t FLOATS_PER_VERTEX = 8;
		const size_t vertexCount = sizeof(CUBE_VERTICES) / (FLOATS_PER_VERTEX * sizeof(float));

		for (size_t i = 0; i < vertexCount; ++i)
		{
			Vertex v{};
			v.position = glm::vec3(
				CUBE_VERTICES[i * FLOATS_PER_VERTEX + 0],
				CUBE_VERTICES[i * FLOATS_PER_VERTEX + 1],
				CUBE_VERTICES[i * FLOATS_PER_VERTEX + 2]
			);
			v.normal = glm::vec3(
				CUBE_VERTICES[i * FLOATS_PER_VERTEX + 3],
				CUBE_VERTICES[i * FLOATS_PER_VERTEX + 4],
				CUBE_VERTICES[i * FLOATS_PER_VERTEX + 5]
			);
			v.texCoord = glm::vec2(
				CUBE_VERTICES[i * FLOATS_PER_VERTEX + 6],
				CUBE_VERTICES[i * FLOATS_PER_VERTEX + 7]
			);
			vertices.push_back(v);
		}

		indices.assign(std::begin(CUBE_INDICES), std::end(CUBE_INDICES));

		m_shader = m_resources.GetOrCreateShader("cube", CUBE_VERT_SRC, CUBE_FRAG_SRC);
		assert(m_shader != nullptr && "Failed to create cube shader");
		m_cube = m_resources.CreateMesh("cube", vertices, indices);
		assert(m_cube != nullptr && "Failed to create cube mesh");

		// Multiple colored checkerboard materials
		auto* texWhite = m_resources.CreateCheckerboard("tex_white", 256, 8, 255, 255, 255, 255, 50, 50, 50, 255);
		auto* texRed   = m_resources.CreateCheckerboard("tex_red",   256, 8, 220, 60, 60, 255, 120, 20, 20, 255);
		auto* texBlue  = m_resources.CreateCheckerboard("tex_blue",  256, 8, 60, 120, 220, 255, 20, 40, 120, 255);
		auto* texGreen = m_resources.CreateCheckerboard("tex_green", 256, 8, 60, 200, 80, 255, 20, 100, 40, 255);

		assert(texWhite != nullptr && texRed != nullptr && texBlue != nullptr && texGreen != nullptr
			&& "Failed to create checkerboard textures");

		m_materials[0] = m_resources.GetOrCreateMaterial("mat_white", *m_shader, *texWhite);
		m_materials[1] = m_resources.GetOrCreateMaterial("mat_red",   *m_shader, *texRed);
		m_materials[2] = m_resources.GetOrCreateMaterial("mat_blue",  *m_shader, *texBlue);
		m_materials[3] = m_resources.GetOrCreateMaterial("mat_green", *m_shader, *texGreen);

		for (int32_t i = 0; i < NUM_MATERIALS; ++i)
		{
			assert(m_materials[i] != nullptr && "Failed to create material");
		}

		// Build cube grid with per-cube rotation data
		m_cubes.clear();

		constexpr int32_t GRID = 3;
		constexpr float SPACING = 2.5f;
		m_cubes.reserve(static_cast<size_t>((2 * GRID + 1) * (2 * GRID + 1)));

		for (int32_t x = -GRID; x <= GRID; ++x)
		{
			for (int32_t z = -GRID; z <= GRID; ++z)
			{
				const int32_t matIndex = ((x + GRID) + (z + GRID) * (2 * GRID + 1)) % NUM_MATERIALS;

				CubeData cube{};
				cube.transform.SetPosition(glm::vec3(
					static_cast<float>(x) * SPACING,
					0.0f,
					static_cast<float>(z) * SPACING
				));
				cube.rotationAxis = glm::normalize(glm::vec3(
					static_cast<float>(x) * 0.3f + 0.5f,
					1.0f,
					static_cast<float>(z) * 0.3f + 0.7f
				));
				cube.angle = static_cast<float>(x * 10 + z * 7);
				cube.speed = 15.0f + static_cast<float>(std::abs(x) * 3 + std::abs(z) * 5);
				cube.materialIndex = matIndex;
				m_cubes.push_back(cube);
			}
		}

		m_camera.SetPerspective(60.0f,
			static_cast<float>(m_window.Width()) / static_cast<float>(m_window.Height()),
			0.1f, 100.0f);

		m_initialized = true;
		Logger::Info("PlayState initialized successfully");
	}
	catch (const std::exception& e)
	{
		Logger::Error(std::string("PlayState init failed: ") + e.what());
		m_initialized = false;
	}
}

void PlayState::OnExit() noexcept
{
	m_input.ClearActions();
	m_input.SetMouseCaptured(false);
	m_resources.CleanupUnused();
}

void PlayState::OnPause() noexcept
{
	m_input.SetMouseCaptured(false);
}

void PlayState::OnResume() noexcept
{
	SDL_ShowCursor();
}

void PlayState::HandleEvent(const SDL_Event& event) noexcept
{
	PROFILE_FUNCTION();

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_TAB)
	{
		m_showDebug = !m_showDebug;
	}

	if (event.type == SDL_EVENT_WINDOW_RESIZED)
	{
		const int32_t w = event.window.data1;
		const int32_t h = event.window.data2;
		if (h > 0)
		{
			m_camera.SetAspectRatio(static_cast<float>(w) / static_cast<float>(h));
		}
	}
}

void PlayState::FixedUpdate(float fixedDt) noexcept
{
	PROFILE_FUNCTION();
	assert(m_initialized && "PlayState::FixedUpdate called before init");

	// Rotate all cubes (physics/animation step) and cache world matrices
	for (auto& cube : m_cubes)
	{
		cube.angle += cube.speed * fixedDt;
		cube.worldMatrix = makeModel(cube.transform.GetPosition(), cube.angle, cube.rotationAxis);
	}

	// Collision detection: sphere vs each cube AABB
	m_debugSphere.center = m_camera.Position();
	m_debugSphere.radius = 2.5f;

	m_collidingCubes = 0;
	const AABB cubeLocalAABB = m_cube->LocalAABB();
	for (const auto& cube : m_cubes)
	{
		const AABB worldAABB = cubeLocalAABB.Transform(cube.worldMatrix);
		if (Collision::SphereVsAABB(m_debugSphere, worldAABB))
		{
			++m_collidingCubes;
		}
	}
}

void PlayState::Update(const DeltaTime& dt) noexcept
{
	PROFILE_FUNCTION();
	assert(m_initialized && "PlayState::Update called before init");

	// Camera movement via action mapping
	const float speed = 3.0f * dt.Seconds();
	if (m_input.IsActionDown("MoveForward"))  m_camera.MoveForward(speed);
	if (m_input.IsActionDown("MoveBackward")) m_camera.MoveForward(-speed);
	if (m_input.IsActionDown("MoveLeft"))     m_camera.MoveRight(-speed);
	if (m_input.IsActionDown("MoveRight"))    m_camera.MoveRight(speed);
	if (m_input.IsActionDown("MoveUp"))       m_camera.MoveUp(speed);
	if (m_input.IsActionDown("MoveDown"))     m_camera.MoveUp(-speed);

	if (m_input.IsMouseCaptured())
	{
		constexpr float mouseSensitivity = 0.15f;
		const float yawDelta = m_input.MouseDeltaX() * mouseSensitivity;
		const float pitchDelta = -m_input.MouseDeltaY() * mouseSensitivity;
		m_camera.Rotate(yawDelta, pitchDelta);
	}

	m_audio.Update();
}

void PlayState::RenderScene(Renderer& renderer) noexcept
{
	PROFILE_FUNCTION();
	if (!m_initialized)
	{
		return;
	}

	m_renderer = &renderer;

	renderer.BeginFrame(m_camera.ViewMatrix(), m_camera.ProjectionMatrix());

	for (const auto& cube : m_cubes)
	{
		assert(cube.materialIndex >= 0 && cube.materialIndex < NUM_MATERIALS
			&& "Cube material index out of range");
		renderer.Submit(*m_cube, *m_materials[cube.materialIndex], cube.worldMatrix);
	}

	m_debugRenderer.SetEnabled(m_showDebug);

	if (m_showDebug)
	{
		for (const auto& cube : m_cubes)
		{
			m_debugRenderer.DrawMeshWireframe(
				m_cube->GetVertices(),
				m_cube->GetIndices(),
				cube.worldMatrix,
				glm::vec3(0.3f, 0.6f, 1.0f)
			);
			m_debugRenderer.DrawAABB(m_cube->LocalAABB(), cube.worldMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
		}

		m_debugRenderer.DrawSphere(m_debugSphere, glm::vec3(1.0f, 0.0f, 0.0f));
	}

	renderer.EndFrame();
}

void PlayState::RenderOverlay(const glm::mat4& viewProj) noexcept
{
	PROFILE_FUNCTION();

	if (m_showDebug && m_initialized)
	{
		m_debugRenderer.SetViewProj(viewProj);
		m_debugRenderer.Flush(m_camera.ViewMatrix(), m_camera.ProjectionMatrix());
	}
}

void PlayState::Render() noexcept
{
	if (!m_initialized)
	{
		ImGui::Begin("Error");
		ImGui::Text("Failed to initialize PlayState. Check logs.");
		ImGui::End();
		return;
	}

	ImGui::Begin("Play State");
	ImGui::Text("Instanced cube grid (%zu cubes)", m_cubes.size());
	ImGui::Text("%d materials | Action mapping + instancing", NUM_MATERIALS);
	ImGui::Text("Collisions (sphere vs cube): %d/%zu", m_collidingCubes, m_cubes.size());
	ImGui::Separator();
	ImGui::Text("Controls: W/A/S/D/Space/Shift");
	ImGui::Text("Actions: MoveForward/Backward/Left/Right/Up/Down");
	ImGui::Text("Tab: Toggle debug visualization [%s]", m_showDebug ? "ON" : "OFF");

	if (ImGui::Button("Back to Menu"))
	{
		m_machine.ReplaceState("MainMenu");
	}
	ImGui::End();

	// Camera debug window
	ImGui::Begin("Camera Debug");
	const glm::vec3 pos = m_camera.Position();
	const glm::vec3 fwd = m_camera.Forward();
	ImGui::Text("Position:  %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
	ImGui::Text("Forward:   %.2f, %.2f, %.2f", fwd.x, fwd.y, fwd.z);
	ImGui::Separator();
	if (m_renderer != nullptr)
	{
		ImGui::Text("Meshes drawn: %d / %d", m_renderer->DrawnMeshes(), m_renderer->TotalMeshes());
	}
	else
	{
		ImGui::Text("Meshes drawn: N/A");
	}
	ImGui::End();
}
