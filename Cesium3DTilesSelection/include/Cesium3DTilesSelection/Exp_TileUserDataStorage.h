#pragma once

#include <entt/entity/registry.hpp>
#include <cstdint>

namespace Cesium3DTilesSelection {
class TileUserDataStorage {
public:
  using Handle = entt::entity;

  static constexpr Handle NullHandle = entt::null;

  Handle createHandle() { return _registry.create(); }

  void destroyHandle(Handle handle) { _registry.destroy(handle); }

  bool isValidHandle(Handle handle) const { return _registry.valid(handle); }

  template <class UserData, class... Args>
  UserData& createUserData(Handle handle, Args&&... args) {
    return _registry.emplace<UserData>(handle, std::forward<Args>(args)...);
  }

  template <class UserData> UserData& getUserData(Handle handle) {
    return _registry.get<UserData>(handle);
  }

  template <class UserData> const UserData& getUserData(Handle handle) const {
    return _registry.get<UserData>(handle);
  }

  template <class UserData> UserData* tryGetUserData(Handle handle) {
    return _registry.try_get<UserData>(handle);
  }

  template <class UserData>
  const UserData* tryGetUserData(Handle handle) const {
    return _registry.try_get<UserData>(handle);
  }

  template <class UserData> void deleteUserData(Handle handle) {
    _registry.remove<UserData>(handle);
  }

private:
  entt::registry _registry;
};
} // namespace Cesium3DTilesSelection
