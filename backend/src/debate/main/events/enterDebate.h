#pragma once

#include <string>

/**
 * Event handler for entering a debate topic.
 * Updates the user's location in the database to the specified topic.
 * 
 * @param user The username of the user entering the debate
 * @param topic The debate topic ID to enter
 * @return true if successful, false otherwise
 */
bool enterDebate(const std::string& user, const std::string& topic);