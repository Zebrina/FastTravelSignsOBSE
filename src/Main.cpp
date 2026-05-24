#include "OBSE/OBSE.h"
#include "RE/Oblivion.h"
#include <REL//REL.h>

#include <BGSScriptExtenderPluginTools.h>
#include <windows_lean_and_mean.h>

#include "FastTravelPoint.h"
#include "Plugin.h"

#include <map>
#include <memory>s
#include <set>
#include <string>

#pragma comment(lib, "commonlibob64.lib")
#pragma comment(lib, "BGSScriptExtenderPluginTools.lib")

using namespace RE;

static PlayerCharacter* gPlayer = nullptr;;

static std::map<TESObjectACTI*, FastTravelPoint*> gFastTravelPoints{};

namespace hooked
{
    using activate_func_t = bool(*)(TESObjectACTI*, TESObjectREFR*, TESObjectREFR*, bool, TESBoundObject*, int32_t);
    activate_func_t Activate = nullptr;
}

using MessageCallback = void(*)();

using ShowMessageFunction = bool(*)(const char*, MessageCallback, int, const char*, ...);
static ShowMessageFunction ShowMessage = REL::Relocation<ShowMessageFunction>{ REL::ID(405031) }.get();

static int GetMessageResult()
{
    static REL::Relocation<decltype(GetMessageResult)> func{ REL::ID(405054) };
    return func();
}

static void FastTravelTo(TESObjectREFR* target)
{
    static REL::Relocation<void(*)()> ForceFadeToGame{ REL::ID(76656) };


    static REL::Relocation<void(*)(PlayerCharacter*, TESObjectREFR*)> func{ REL::ID(406798) };

    ForceFadeToGame();

    bool sameWorldSpace = (target->GetWorldSpace() == gPlayer->GetWorldSpace());

    func(gPlayer, target);

    if (sameWorldSpace)
        gPlayer->MoveToRefIfLoaded(target);
        

    /*
    ExtraPersistentCell* persistentCell = target->extra.GetExtraData<ExtraPersistentCell>();

    TESObjectCELL* targetCell = target->parentCell;
    TESObjectCELL* saveParentCell = target->GetSaveParentCell();
    const NiPoint3* location = target->GetLocationOnReference();
    const NiPoint3* rotation = target->GetStartingAngle();
    */

    //gPlayer->MoveToIfLoaded(target);
    //gPlayer->MoveToCell(*location, *rotation, target->parentCell, false, true);
}

static void CenterOnCell(PlayerCharacter*, TESObjectCELL*)
{
    static REL::Relocation<int> func{ REL::ID(408579) };
}

static bool ActivatorActivate(TESObjectACTI* activator, TESObjectREFR* targetRef, TESObjectREFR* activatorRef, bool idFlag, TESBoundObject* object, int32_t targetCount)
{
    bool success = hooked::Activate(activator, targetRef, activatorRef, idFlag, object, targetCount);
    
    if (success && activatorRef == gPlayer)
    {
        auto found = gFastTravelPoints.find(activator);
        if (found != gFastTravelPoints.end())
        {
            static FastTravelPoint* target;
            static TESObjectREFR* mapMarkerRef;

            target = found->second;
            mapMarkerRef = target->GetClosestMapMarker(activatorRef);

            if (mapMarkerRef)
            {
                auto callback = []()
                    {
                        int result = InterfaceManager::GetLastMessageButtonClicked();
                        if (result == 1)
                            FastTravelTo(mapMarkerRef);
                    };
                InterfaceManager::ShowMessageBox(found->second->GetMessagePrompt(), callback, "LOC_HC_MenuGamesettings_sNo", "LOC_HC_MenuGamesettings_sYes");
            }
        }
    }

    return success;
}

static void MessageListener(OBSE::MessagingInterface::Message* message)
{
    if (message->type != OBSE::MessagingInterface::kDataLoaded)
        return;

    gPlayer = PlayerCharacter::GetSingleton();

    REL::Relocation<uintptr_t> vtbl{ TESObjectACTI::VTABLE[0] };

    hooked::Activate = hooking::hook_virtual_function("Activator::Activate", vtbl.get(), ActivatorActivate, 0x33);

    std::map<std::string, FastTravelPoint*> map{};
    std::map<std::string, std::string> dependencies{};
    std::set<std::string> ignored{};

    plugin_configuration::for_each_configuration("OBSE\\Plugins\\FastTravelSigns", [&map, &dependencies, &ignored](plugin_configuration& config)
        {
            config.for_each_section([&map, &dependencies, &ignored](plugin_configuration::table_interface& data)
                {
                    const char* name = data.get_name();
                    if (ignored.count(name))
                    {
                        plugin_log::err("'{}' ignored."sv, name);
                        return plugin_configuration::for_each_continue;
                    }

                    FastTravelPoint* base = nullptr;

                    std::string templateName;
                    if (data.try_get("sInheritFrom", templateName))
                    {
                        auto circularDependency = dependencies.find(templateName);
                        if (circularDependency != dependencies.end())
                        {
                            plugin_log::err("Circular dependency detected '{}' <=> '{}'!"sv, name, templateName);
                            ignored.insert(name);
                            ignored.insert(templateName);
                            return plugin_configuration::for_each_continue;
                        }
                        else
                        {
                            dependencies.emplace(name, templateName);
                        }

                        auto mappedParent = map.find(templateName);
                        if (mappedParent == map.end())
                            return plugin_configuration::for_each_yield;

                        base = mappedParent->second;
                    }

                    std::unique_ptr<FastTravelPoint> fastTravelPoint{ new FastTravelPoint(base) };

                    TESObjectACTI* activator = data.get_form<TESObjectACTI>("kActivatorForm");
                    if (activator)
                    {
                        fastTravelPoint->SetActivator(activator);
                    }
                    else if (!base)
                    {
                        plugin_log::err("'{}' does not have a valid activator or a base from which to inherit from."sv, name);
                        return plugin_configuration::for_each_continue;
                    }

                    std::vector<TESObjectREFR*> mapMarkers = data.get_form_array<TESObjectREFR>("kMapMarkerReferences");

                    if (!mapMarkers.empty())
                    {
                        fastTravelPoint->SetMapMarkers(std::move(mapMarkers));
                    }
                    else if (!base)
                    {
                        plugin_log::err("'{}' does not have any valid map marker references or a base from which to inherit from."sv, name);
                        return plugin_configuration::for_each_continue;
                    }

                    std::string message = data.get("sMessage", "");
                    if (!message.empty())
                    {
                        fastTravelPoint->SetMessagePrompt(message.c_str());
                    }
                    else if (!base)
                    {
                        plugin_log::err("'{}' does not have a valid message or a base from which to inherit from."sv, name);
                        return plugin_configuration::for_each_continue;
                    }

                    std::string parentName = data.get("sParent", "");
                    if (!parentName.empty())
                    {
                        if (parentName == templateName)
                        {
                            plugin_log::err("'{}' cannot inherit from parent."sv, name);
                            return plugin_configuration::for_each_continue;
                        }

                        base->AddChild(fastTravelPoint.release());

                        return plugin_configuration::for_each_continue;
                    }

                    bool flag;

                    if (data.try_get("bRequireVisibility", flag))
                        fastTravelPoint->SetRequiresVisibility(flag);

                    if (data.try_get("bRequireDiscovery", flag))
                        fastTravelPoint->SetRequiresDiscovery(flag);

                    gFastTravelPoints.emplace(fastTravelPoint->GetActivator(), fastTravelPoint);
                    map[name] = fastTravelPoint.release();

                    return plugin_configuration::for_each_continue;
                });
        });

    //gFastTravelPoints.emplace(TESForm::LookupByID(0x065586), FastTravelPoint{ "Travel to Anvil?", (TESObjectREFR*)TESForm::LookupByID(0x020097) });
    //gFastTravelPoints.emplace(TESForm::LookupByID(0x065587), FastTravelPoint{ "Travel to Bravil?", (TESObjectREFR*)TESForm::LookupByID(0x020098) });
    //gFastTravelPoints.emplace(TESForm::LookupByID(0x065588), FastTravelPoint{ "Travel to Bruma?", (TESObjectREFR*)TESForm::LookupByID(0x020099) }); // 020099 = East, 18D897 = North Gate
    //gFastTravelPoints.emplace(TESForm::LookupByID(0x065589), FastTravelPoint{ "Travel to Cheydinhal?", (TESObjectREFR*)TESForm::LookupByID(0x0) }); // 02009A = West, 0FF19B = East Gate
    //gFastTravelPoints.emplace(TESForm::LookupByID(0x06557B), FastTravelPoint{ "Travel to Chorrol?", (TESObjectREFR*)TESForm::LookupByID(0x0) });
    // 0FF19C = South Gate
    // 06A80F = North Gate
    // 
    //gFastTravelPoints.emplace(TESForm::LookupByID(0x06558A), FastTravelPoint{ "Travel to The Imperial City?", (TESObjectREFR*)TESForm::LookupByID(0x0) });
    // 008B6D = Market District
    // 008B66 = Arboretum
    // 008B6E = Talos Plaza
    // 008B67 = Elven Gardens District
    // 008B6F = Temple District
    // 008B65 = Arena District

    //gFastTravelPoints.emplace(TESForm::LookupByID<TESObjectACTI>(0x06558B), FastTravelPoint{ "Travel to Kvatch?", TESForm::LookupByID<TESObjectREFR>(0x01E8A2) }); // KvatchMapMarker
    //gFastTravelPoints.emplace(TESForm::LookupByID(0x06558C), FastTravelPoint{ "Travel to Leyawiin?", (TESObjectREFR*)TESForm::LookupByID(0x02009B) }); // 02009B = North East Gate, 0E78DF = West Gate
    //gFastTravelPoints.emplace(TESForm::LookupByID(0x06558D), FastTravelPoint{ "Travel to Skingrad?", (TESObjectREFR*)TESForm::LookupByID(0x0) }); // 00145A = West Gate, 10D752 = East Gate
}

OBSE_PLUGIN_VERSION = []()
    {
        OBSE::PluginVersionData v{};

        v.PluginVersion(Plugin::VERSION);
        v.PluginName(Plugin::MODNAME);
        v.AuthorName(Plugin::AUTHOR);
        v.UsesAddressLibrary(true);
        v.HasNoStructUse(true);
        //v.UsesUpdatedStructs();
        //v.CompatibleVersions({ OBSE::Version{ 0, 0, 0, 1 } });

        return v;
    }();

OBSE_PLUGIN_LOAD(const OBSE::LoadInterface* obse)
{
    if (!plugin_log::initialize(Plugin::INTERNALNAME))
        return false;

    OBSE::InitInfo info{};
    info.log = false;
    OBSE::Init(obse, info);

    OBSE::MessagingInterface* messagingInterface = static_cast<OBSE::MessagingInterface*>(obse->QueryInterface(OBSE::LoadInterface::kMessaging));
    if (!messagingInterface)
    {
        plugin_log::err("Failed to query OBSE messaging interface."sv);
        return false;
    }

    if (!messagingInterface->RegisterListener(MessageListener))
    {
        plugin_log::err("Failed to register message listener."sv);
        return false;
    }

    //constexpr const std::string_view configSection{ "Settings"sv };

    //plugin_configuration config{ Plugin::CONFIGFILE };

    //gPercentRecharge = config.get(configSection, "bPercentRecharge"sv, false);
    //gRechargeValuePerDay = config.get(configSection, "fRechargeValuePerDay"sv, 3200.0f);
    //gRechargeInterval = config.get(configSection, "iRechargeInterval"sv, 1000);
    //gRechargeFollowerWeapons = config.get(configSection, "bRechargeFollowerWeapons", false);

    plugin_log::info("Successful plugin load! Have fun! :D"sv);

    return true;
}
