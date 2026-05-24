#include "FastTravelPoint.h"

using namespace RE;

FastTravelPoint::~FastTravelPoint()
{
    delete condition_;
    delete message_;
    delete mapMarkers_;
    delete children_;
}

const FastTravelPoint& FastTravelPoint::GetBase() const
{
    return base_ ? base_->GetBase() : *this;
}

void FastTravelPoint::AddChild(const FastTravelPoint& child)
{
}

bool FastTravelPoint::HasInherited(InheritanceFlags flag) const
{
    return (inherited_ & flag) == flag;
}

bool FastTravelPoint::GetRequiresVisibility() const
{
    return (base_ && (inherited_ & InheritanceFlags::RequireVisibility) != 0) ? base_->GetRequiresVisibility() : requireVisibility_;
}

void FastTravelPoint::SetRequiresVisibility(bool value)
{
    requireVisibility_ = value;
    inherited_ = (InheritanceFlags)(inherited_ | InheritanceFlags::RequireVisibility);
}

bool FastTravelPoint::GetRequiresDiscovery() const
{
    return (base_ && (inherited_ & InheritanceFlags::RequireDiscovery) != 0) ? base_->GetRequiresDiscovery() : requireDiscovery_;
}

void FastTravelPoint::SetRequiresDiscovery(bool value)
{
    requireDiscovery_ = value;
    inherited_ = (InheritanceFlags)(inherited_ | InheritanceFlags::RequireDiscovery);
}

const char* FastTravelPoint::GetMessagePrompt() const
{
    return (base_ && (inherited_ & InheritanceFlags::Message) != 0) ? base_->GetMessagePrompt() : message_->c_str();
}

void FastTravelPoint::SetMessagePrompt(const char* message)
{
    if (message && message[0] != '\0')
    {
        message_ = new std::string(message);
        inherited_ = (InheritanceFlags)(inherited_ | InheritanceFlags::Message);
    }
}

const std::vector<TESObjectREFR*>& FastTravelPoint::GetMapMarkers() const
{
    return (base_ && (inherited_ & InheritanceFlags::MapMarkers) != 0) ? base_->GetMapMarkers() : *mapMarkers_;
}

void FastTravelPoint::SetMapMarkers(std::vector<TESObjectREFR*>&& mapMarkers)
{
    if (!mapMarkers.empty())
    {
        mapMarkers_ = new std::vector<TESObjectREFR*>(std::move(mapMarkers));
        inherited_ = (InheritanceFlags)(inherited_ | InheritanceFlags::MapMarkers);
    }
}

bool FastTravelPoint::IsUnlocked() const
{
    if (!condition_)
        return true;

    // TODO: Evaluate condition.

    return false;
}

TESObjectREFR* FastTravelPoint::GetClosestMapMarker(TESObjectREFR* target) const
{
    if (!target)
        return nullptr;

    const NiPoint3* targetPos = target->GetLocationOnReference();
    if (!targetPos)
        return nullptr;

    TESObjectREFR* closest = nullptr;
    float closestDistance = std::numeric_limits<float>::max();

    for (auto mapMarker : GetMapMarkers())
    {
        if (mapMarker->IsDisabled())
            continue;

        ExtraMapMarker* extra = mapMarker->extra.GetExtraData<ExtraMapMarker>();
        if (extra)
        {
            if (GetRequiresDiscovery() && !(extra->CanTravelTo()))
                continue;

            if (GetRequiresVisibility() && !(extra->IsVisible()))
                continue;
        }


        const NiPoint3* mapMarkerPos = mapMarker->GetLocationOnReference();
        if (!mapMarkerPos)
            continue;

        float distance = targetPos->GetSquaredDistance(*mapMarkerPos);
        if (distance < closestDistance)
        {
            closest = mapMarker;
            closestDistance = distance;
        }
    }

    return closest;
}

size_t FastTravelPoint::GetEligableChildren(std::vector<const FastTravelPoint*>& childrenOut)
{
    if (!children_)
        return 0;

    size_t count = 0;
    for (auto& child : *children_)
    {
        if (child->IsUnlocked())
        {
            childrenOut.push_back(child.get());
            ++count;
        }
    }

    return count;
}
