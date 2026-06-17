#include "stdafx.h"
#include "game/states/settings_state.h"
#include "engine/delta_time.h"
#include "core/logger.h"

#include "core/json_alias.h"
#include "core/file_io.h"

// ── JSON serialization for PlayerConfig ───────────────────────────────────

static void to_json(json& j, const PlayerConfig& c)
{
	j = json{
		{"playerName",          c.playerName},
		{"mouseSensitivity",    c.mouseSensitivity},
		{"soundVolume",         c.soundVolume},
		{"resolutionWidth",     c.resolutionWidth},
		{"resolutionHeight",    c.resolutionHeight},
		{"renderHeight",        c.renderHeight},
		{"fullscreen",          c.fullscreen},
		{"gridMoveRepeatDelay", c.gridMoveRepeatDelay}
	};
}

static void from_json(const json& j, PlayerConfig& c)
{
	j.at("playerName").get_to(c.playerName);
	j.at("mouseSensitivity").get_to(c.mouseSensitivity);
	j.at("soundVolume").get_to(c.soundVolume);
	j.at("resolutionWidth").get_to(c.resolutionWidth);
	j.at("resolutionHeight").get_to(c.resolutionHeight);
	j.at("renderHeight").get_to(c.renderHeight);
	j.at("fullscreen").get_to(c.fullscreen);
	j.at("gridMoveRepeatDelay").get_to(c.gridMoveRepeatDelay);
}

	static std::string configFilePath()
	{
		return "player_config.json";
	}

	static std::optional<nlohmann::json> readJsonFile(const std::string& path) noexcept
	{
		std::string contents = FileReadString(path.c_str());
		if (contents.empty())
		{
			return std::nullopt;
		}
		return nlohmann::json::parse(contents, nullptr, false);
	}

	static bool writeJsonFile(const std::string& path, const nlohmann::json& j) noexcept
	{
		const std::string contents = j.dump(2);
		return FileWriteBytes(path.c_str(), contents.data(), contents.size());
	}

	float GetGridMoveRepeatDelayFromConfig() noexcept
	{
		auto maybeJson = readJsonFile(configFilePath());
		if (maybeJson.has_value())
		{
			auto& j = *maybeJson;
			if (j.contains("gridMoveRepeatDelay") && j["gridMoveRepeatDelay"].is_number_float())
			{
				float d = j["gridMoveRepeatDelay"].get<float>();
				return d;
			}
		}
		return 0.1f;
	}

	int32_t GetRenderHeightFromConfig() noexcept
	{
		auto maybeJson = readJsonFile(configFilePath());
		if (maybeJson.has_value())
		{
			auto& j = *maybeJson;
			if (j.contains("renderHeight") && j["renderHeight"].is_number_integer())
			{
				return j["renderHeight"].get<int32_t>();
			}
		}
		return 480;
	}

// ── SettingsState ─────────────────────────────────────────────────────────

SettingsState::SettingsState(GameStateMachine& machine) noexcept
	: m_machine(machine)
{}

void SettingsState::OnEnter() noexcept
{
	if (!loadConfig())
	{
		Logger::Warn("Settings state entered with default config (load failed)");
	}
	else
	{
		Logger::Info("Settings state entered, config loaded");
	}
}

void SettingsState::OnExit() noexcept
{
	if (!saveConfig())
	{
		Logger::Error("Settings state exited, config NOT saved");
	}
	else
	{
		Logger::Info("Settings state exited, config saved");
	}
}

void SettingsState::HandleEvent(const SDL_Event& /*event*/) noexcept
{}

void SettingsState::Update(const DeltaTime& /*dt*/) noexcept
{}

void SettingsState::Render() noexcept
{
	ImGui::Begin("Settings");

	ImGui::Text("Player Configuration (JSON serialized)");
	ImGui::Separator();

	char nameBuf[256] = {};
	const size_t nameLen = m_config.playerName.copy(nameBuf, sizeof(nameBuf) - 1);
	nameBuf[nameLen] = '\0';
	if (ImGui::InputText("Player Name", nameBuf, sizeof(nameBuf)))
	{
		m_config.playerName = nameBuf;
	}

	ImGui::SliderFloat("Mouse Sensitivity", &m_config.mouseSensitivity, 0.1f, 2.0f, "%.2f");
	ImGui::SliderFloat("Sound Volume", &m_config.soundVolume, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("Move Repeat Delay", &m_config.gridMoveRepeatDelay, 0.05f, 0.5f, "%.2f s");

	int resW = m_config.resolutionWidth;
	int resH = m_config.resolutionHeight;
	ImGui::InputInt("Width", &resW);
	ImGui::InputInt("Height", &resH);
	if (resW > 0 && resH > 0)
	{
		m_config.resolutionWidth = resW;
		m_config.resolutionHeight = resH;
	}

	int rh = m_config.renderHeight;
	ImGui::SliderInt("Render Height", &rh, 120, 1080, "%d px");
	if (rh >= 120 && rh <= 1080)
	{
		m_config.renderHeight = rh;
	}
	ImGui::TextDisabled("Retro FBO height. Width = height * aspect ratio.");

	ImGui::Checkbox("Fullscreen", &m_config.fullscreen);

	ImGui::Separator();

	if (ImGui::Button("Save Config"))
	{
		if (!saveConfig())
		{
			ImGui::OpenPopup("Save Error");
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Load Config"))
	{
		if (!loadConfig())
		{
			ImGui::OpenPopup("Load Error");
		}
	}

	if (ImGui::BeginPopupModal("Save Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Failed to save config. Check logs for details.");
		if (ImGui::Button("OK")) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Failed to load config. Check logs for details.");
		if (ImGui::Button("OK")) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

	ImGui::Separator();
	ImGui::Text("JSON Output:");

	json j = m_config;
	std::string pretty = j.dump(2);
	ImGui::TextWrapped("%s", pretty.c_str());

	if (ImGui::Button("Back"))
	{
		m_machine.PopState();
	}

	ImGui::End();
}

bool SettingsState::saveConfig() noexcept
{
	try
	{
		json j = m_config;
		const std::string path = configFilePath();
		if (!writeJsonFile(path, j))
		{
			throw std::runtime_error("Cannot open file for writing: " + path);
		}
		Logger::Info("Config saved to " + path);
		return true;
	}
	catch (const std::exception& e)
	{
		Logger::Error(std::string("Failed to save config: ") + e.what());
		return false;
	}
}

bool SettingsState::loadConfig() noexcept
{
	try
	{
		const std::string path = configFilePath();
		auto maybeJson = readJsonFile(path);
		if (!maybeJson.has_value())
		{
			Logger::Warn("Config file not found, using defaults: " + path);
			return false;
		}
		m_config = maybeJson->get<PlayerConfig>();
		Logger::Info("Config loaded from " + path);
		return true;
	}
	catch (const std::exception& e)
	{
		Logger::Warn(std::string("Failed to load config, using defaults: ") + e.what());
		return false;
	}
}
