#include "UserNameResolver.h"

std::string UserNameResolver::ResolveUsername(int userId, VRUserDatabase& userDb) {
	if (userId <= 0) {
		return "Unknown user";
	}

	std::string username = userDb.getUsername(userId);
	if (username.empty()) {
		return "User " + std::to_string(userId);
	}

	return username;
}