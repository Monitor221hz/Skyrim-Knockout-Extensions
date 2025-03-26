#include "log.h"
#include "hook.h"
#include "settings.h"
void OnDataLoaded()
{
   
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
        KnockoutExtensions::Settings::Load();
		break;
	case SKSE::MessagingInterface::kPostLoad:
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		break;
	case SKSE::MessagingInterface::kPostLoadGame:
		KnockoutExtensions::Settings::Load();
        break;
	case SKSE::MessagingInterface::kNewGame:
		KnockoutExtensions::Settings::Load();
		break;
	}
}
void InitializeSerialization()
{
	SKSE::log::trace("Initializing cosave serialization...");
	auto *serde = SKSE::GetSerializationInterface();
	serde->SetUniqueID(KnockoutExtensions::SerializeCode);
	serde->SetSaveCallback(KnockoutExtensions::KnockoutHandler::GameSaveCallback);
	serde->SetRevertCallback(KnockoutExtensions::KnockoutHandler::GameRevertCallback);
	serde->SetLoadCallback(KnockoutExtensions::KnockoutHandler::GameLoadCallback);
	SKSE::log::trace("Cosave serialization initialized.");
}


SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);
	SetupLog();


    auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		return false;
	}
	InitializeSerialization();
	KnockoutExtensions::Settings::Load();

	KnockoutExtensions::UnconsciousFuncHook::Install();
	KnockoutExtensions::BleedoutStateHook::Install();
	KnockoutExtensions::MainUpdateHook::Install();
	KnockoutExtensions::HitEventHook::Install();
	KnockoutExtensions::ConcussionStateHook::Install(); 

	KnockoutExtensions::PlayerActivateHook::Install(); 
	if (KnockoutExtensions::Settings::GetUnconsciousConsideredDead())
	{
		KnockoutExtensions::IsDeadHook::Install(); 
	}
	// KnockoutExtensions::RagdollStateHook::Install(); 
	
    return true;
}

