#include "UserNameResolver.h"

std::string UserNameResolver::ResolveUsername(int userId, VRUserDatabase& userDb) {
	if (userId <= 0) {
		return "Unknown user";
	}

	if (userId == 1) {
		return "Anti-Vax Influencer";
	}

	if (userId == 2) {
		return "Challenger";
	}

	std::string username = userDb.getUsername(userId);
	if (username.empty()) {
		return "User " + std::to_string(userId);
	}

	return username;
}