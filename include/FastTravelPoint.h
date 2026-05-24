#pragma once

#include "RE/Oblivion.h"

#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

class FastTravelPoint
{
public:
    enum InheritanceFlags : std::uint8_t
    {
        RequireVisibility = 1 << 0,
        RequireDiscovery = 1 << 1,
        Message = 1 << 2,
        MapMarkers = 1 << 3,
    };

    class Error : public std::exception
    {
    public:
    };

    class ParseData
    {
    public:
        ParseData(const ParseData&) = delete;

        const FastTravelPoint* GetDependency(const std::string& name);

    private:
        std::map<std::string, FastTravelPoint*> parsed{};
        std::map<std::string, std::string> dependencies{};
        std::set<std::string> ignored{};
    };

    FastTravelPoint(RE::TESObjectACTI& activator) :
        base_{ nullptr },
        activator_{ &activator }
    { }
    FastTravelPoint(const FastTravelPoint& base) :
        base_{ &(base.GetBase()) },
        activator_{ nullptr }
    { }
    FastTravelPoint(const FastTravelPoint&) = default;
    FastTravelPoint(FastTravelPoint&&) = default;
    ~FastTravelPoint();

    const FastTravelPoint& GetBase() const;

    void AddChild(const FastTravelPoint& child);

    bool HasInherited(InheritanceFlags flag) const;
    
    bool GetRequiresVisibility() const;
    void SetRequiresVisibility(bool value);
    bool GetRequiresDiscovery() const;
    void SetRequiresDiscovery(bool value);
    const char* GetMessagePrompt() const;
    void SetMessagePrompt(const char* prompt);
    RE::TESObjectACTI* GetActivator() const { return activator_; }
    void SetActivator(RE::TESObjectACTI* activator) { activator_ = activator; }
    const std::vector<RE::TESObjectREFR*>& GetMapMarkers() const;
    void SetMapMarkers(std::vector<RE::TESObjectREFR*>&& mapMarkers);

    bool IsUnlocked() const;

    RE::TESObjectREFR* GetClosestMapMarker(RE::TESObjectREFR* target) const;

    bool HasChildren() const { return children_; }
    size_t GetEligableChildren(std::vector<const FastTravelPoint*>& childrenOut);

private:
    struct Condition
    {
        RE::TESQuest* quest{ nullptr };
    };

    const FastTravelPoint* base_;
    Condition* condition_{ nullptr };
    InheritanceFlags inherited_{ 0 };
    bool requireVisibility_{ false };
    bool requireDiscovery_{ false };
    std::string* message_{ nullptr };
    RE::TESObjectACTI* activator_;
    std::vector<RE::TESObjectREFR*>* mapMarkers_{ nullptr };
    std::vector<std::unique_ptr<const FastTravelPoint>>* children_{ nullptr };
};
