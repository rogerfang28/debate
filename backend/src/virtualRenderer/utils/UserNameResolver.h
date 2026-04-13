#pragma once

#include <string>
#include "../../database/virtualrenderer/VRUserDatabase.h"

class UserNameResolver {
public:
	static std::string ResolveUsername(int userId, VRUserDatabase& userDb);
};