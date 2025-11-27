#pragma once

#include <string>

/**
 * Event handler for going home.
 * Updates the user's location in the database to "home".
 * 
 * @param user The username of the user going home
 * @return true if successful, false otherwise
 */
bool goHome(const std::string& user);