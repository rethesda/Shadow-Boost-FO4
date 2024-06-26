
#include "shadow.h"

#include "imgui_menu.h"
#include "f4se_plugin.h"

namespace Shadow {

	void Boost::EnableShadow() noexcept
	{
		if (!Settings::Ini::GetSingleton().GetShadowEnabled() && instance) {
			*fDirShadowDistance = o_fDirShadowDistance;
		}
	}

	void Boost::EnableLod() noexcept
	{
		if (!Settings::Ini::GetSingleton().GetLodEnabled() && instance) {

			*fLODFadeOutMultObjects = o_fLODFadeOutMultObjects;
			*fLODFadeOutMultItems = o_fLODFadeOutMultItems;
			*fLODFadeOutMultActors = o_fLODFadeOutMultActors;
		}
	}

	void Boost::EnableBlock() noexcept
	{
		if (!Settings::Ini::GetSingleton().GetBlockEnabled() && instance) {

			*fBlockLevel2Distance = o_fBlockLevel2Distance;
			*fBlockLevel1Distance = o_fBlockLevel1Distance;
			*fBlockLevel0Distance = o_fBlockLevel0Distance;
		}
	}

	void Boost::EnableGrass() noexcept
	{
		if (!Settings::Ini::GetSingleton().GetGrassEnabled() && instance) {
			*fGrassStartFadeDistance = o_fGrassStartFadeDistance;
		}
	}

	void Boost::EnableGodRays() noexcept
	{
		if (!instance) {
			return;
		}

		auto& settings = Settings::Ini::GetSingleton();

		if (!settings.GetGodRaysEnabled()) {

			*GrQuality = o_GrQuality;
			*GrScale = o_GrScale;
			*GrCascade = o_GrCascade;
			*GrGrid = o_GrGrid;
		}
		else {

			*GrQuality = settings.GetGrQuality();
			*GrGrid = settings.GetGrGrid();
			*GrScale = settings.GetGrScale();
			*GrCascade = settings.GetGrCascade();
		}
	}

	void Boost::SetGodRays() noexcept
	{
		auto& settings = Settings::Ini::GetSingleton();

		if (!instance || !settings.GetGodRaysEnabled()) {
			return;
		}

		*GrQuality = settings.GetGrQuality();
		*GrGrid = settings.GetGrGrid();
		*GrScale = settings.GetGrScale();
		*GrCascade = settings.GetGrCascade();
	}

	[[nodiscard]] constexpr float GetTarget(float fps) noexcept { return Millisecond / fps; }

	void Boost::SetupFps() noexcept
	{
		auto& settings = Settings::Ini::GetSingleton();
		
		target = GetTarget(settings.GetFpsTarget());

		tolerance = settings.GetMsTolerance();

		oldTime = std::chrono::high_resolution_clock::now();
	}

	void Boost::ReloadStuff() noexcept
	{
		EnableShadow();
		EnableLod();
		EnableBlock();
		EnableGrass();
		EnableGodRays();
	}

	void Boost::GameLoaded() noexcept
	{
		if (isLoaded) {
			return;
		}

		SaveOriginalValues();
		SetGodRays();

		GetDispatcher<TESLoadGameEvent>(REL::ID(823570))->RegisterSink(&instance);
		GetDispatcher<TESCellAttachDetachEvent>(REL::ID(1029687))->RegisterSink(&instance);

		RE::CellAttachDetachEventSource::CellAttachDetachEventSourceSingleton::GetSingleton().source.RegisterSink(&instance);

		RE::UI::GetSingleton()->RegisterSink<RE::MenuOpenCloseEvent>(&instance);

		isLoaded = true;

		logger::info("Events Registered Successfully!"sv);

		Run();
	}

	void Boost::SaveOriginalValues() noexcept
	{
		if (!instance) {
			return;
		}

		o_fDirShadowDistance = *fDirShadowDistance;

		o_fLODFadeOutMultObjects = *fLODFadeOutMultObjects;
		o_fLODFadeOutMultItems = *fLODFadeOutMultItems;
		o_fLODFadeOutMultActors = *fLODFadeOutMultActors;

		o_fGrassStartFadeDistance = *fGrassStartFadeDistance;

		o_fBlockLevel2Distance = *fBlockLevel2Distance;
		o_fBlockLevel1Distance = *fBlockLevel1Distance;
		o_fBlockLevel0Distance = *fBlockLevel0Distance;

		o_GrQuality = *GrQuality;
		o_GrScale = *GrScale;
		o_GrCascade = *GrCascade;
		o_GrGrid = *GrGrid;
	}

	void Boost::operator()() noexcept
	{
		if (!run) {
			return;
		}

		auto& settings = Settings::Ini::GetSingleton();

		countFps++;

		if (countFps < settings.GetFpsDelay()) {
			return;
		}

		countFps = 0.0f;

		auto delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - oldTime);

		avg = static_cast<float>(delta.count()) / Millisecond / settings.GetFpsDelay();

		oldTime = std::chrono::high_resolution_clock::now();

		dyn = avg - target;

		if (dyn >= 0.0f && dyn <= tolerance) {
			dyn = 0.0f;
		}

		if (fDirShadowDistance.get() && 
			settings.GetShadowEnabled()) {
			*fDirShadowDistance = std::clamp(*fDirShadowDistance - dyn * settings.GetShadowFactor(),
				settings.GetMinDistance(), settings.GetMaxDistance());
		}

		float d = dyn * settings.GetLodFactor();

		if (settings.GetLodEnabled()) {

			if (fLODFadeOutMultObjects.get()) {
				*fLODFadeOutMultObjects = std::clamp(*fLODFadeOutMultObjects - d,
					settings.GetMinObjects(), settings.GetMaxObjects());
			}

			if (fLODFadeOutMultItems.get()) {
				*fLODFadeOutMultItems = std::clamp(*fLODFadeOutMultItems - d,
					settings.GetMinItems(), settings.GetMaxItems());
			}
		
			if (fLODFadeOutMultActors.get()) {
				*fLODFadeOutMultActors = std::clamp(*fLODFadeOutMultActors - d, 
					settings.GetMinActors(), settings.GetMaxActors());
			}
		}

		if (settings.GetGrassEnabled() && 
			fGrassStartFadeDistance.get()) {
			*fGrassStartFadeDistance = std::clamp(*fGrassStartFadeDistance - dyn * settings.GetGrassFactor(),
				settings.GetMinGrass(), settings.GetMaxGrass());
		}

		if (settings.GetBlockEnabled() &&
			fBlockLevel2Distance.get() && 
			fBlockLevel1Distance.get() && 
			fBlockLevel0Distance.get()) {

			if (*fDirShadowDistance <= settings.GetMinDistance() && dyn > 0.0) {
				indexBlockLevel = std::clamp(indexBlockLevel + 1, 0, Settings::MaxBlockLevels - 1);
			}

			if (*fDirShadowDistance >= settings.GetMaxDistance() && dyn <= 0.0) {
				indexBlockLevel = std::clamp(indexBlockLevel - 1, 0, Settings::MaxBlockLevels - 1);
			}

			auto& blockLevel = settings.GetBlockLevel(indexBlockLevel);

			*fBlockLevel2Distance = blockLevel.fDistBlockLevel2;
			*fBlockLevel1Distance = blockLevel.fDistBlockLevel1;
			*fBlockLevel0Distance = blockLevel.fDistBlockLevel0;
		}
	}

	Boost::operator bool() noexcept
	{
		return (fDirShadowDistance.get() &&
				fLODFadeOutMultObjects.get() &&
				fLODFadeOutMultItems.get() &&
				fLODFadeOutMultActors.get() &&
				fGrassStartFadeDistance.get() &&
				fBlockLevel2Distance.get() &&
				fBlockLevel1Distance.get() &&
				fBlockLevel0Distance.get() &&
				GrQuality.get() &&
				GrGrid.get() &&
				GrScale.get() &&
				GrCascade.get());
	}

	void Boost::SetPlayerLocation() noexcept
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player || !player->parentCell || !player->currentLocation) {
			return;
		}

		auto curCell = player->parentCell;
		auto curLocation = player->currentLocation;

		if (!curCell->cellState.all(RE::TESObjectCELL::CELL_STATE::kAttached)) {
			return;
		}

		imgui_menu::Menu::GetSingleton().SetLocation(curCell->formID, curCell->GetFullName(),
			curLocation->formID, curLocation->GetFullName(), curCell->IsInterior());
	}

	RE::BSEventNotifyControl Boost::ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source)
	{
		if (!a_source || !Settings::Ini::GetSingleton().IsPauseInMenu()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		Settings::Ini::GetSingleton().IsMenuOpen() ? Pause() : Run();

		//logger::info("{} {} {} ", a_event.menuName.c_str(), a_event.opening, cntMenusOpened);

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl Boost::ProcessEvent(const TESLoadGameEvent& a_event, RE::BSTEventSource<TESLoadGameEvent>* a_source)
	{
		if (!a_source) {
			return RE::BSEventNotifyControl::kContinue;
		}

		SetGodRays();

		f4se::Plugin::GetSingleton().AddTask(new TaskLocation());

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl Boost::ProcessEvent(const TESCellAttachDetachEvent& a_event, RE::BSTEventSource<TESCellAttachDetachEvent>* a_source)
	{
		if (!a_source || !a_event.refr || !a_event.refr->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		f4se::Plugin::GetSingleton().AddTask(new TaskLocation());

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl Boost::ProcessEvent(const RE::CellAttachDetachEvent& a_event, RE::BSTEventSource<RE::CellAttachDetachEvent>* a_source)
	{
		if (!a_source || !a_event.cell || !a_event.type.all(RE::CellAttachDetachEvent::EVENT_TYPE::kPostAttach)) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto player = RE::PlayerCharacter::GetSingleton();

		if (!player || !player->parentCell || player->parentCell != a_event.cell) {
			return RE::BSEventNotifyControl::kContinue;
		}

		f4se::Plugin::GetSingleton().AddTask(new TaskLocation());

		return RE::BSEventNotifyControl::kContinue;
	}

	void TaskLocation::Run()
	{
		Boost::GetSingleton().SetPlayerLocation();
	}

	Boost Boost::instance;
}
