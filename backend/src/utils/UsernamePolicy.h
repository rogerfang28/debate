#pragma once

#include <functional>
#include <string>

// What a NEW username may be, in one place: password registration and
// renaming both ask here, and Google sign-in builds its names to pass here.
//
//   3 to 24 characters; letters, digits, '_', '-', '.'; the first a letter or
//   a digit; unique ignoring case (the caller's database decides that).
//   Reserved: "visitor-..." (anonymous readers), "deleted-..." (closed
//   accounts), and anything with '@' (an email is private, and Google's).
//
// Only new names are gated. Names that already exist and break these rules
// -- voices such as "Fauci's attorney", old one-letter test accounts -- keep
// working: nothing here is applied to an existing account.
namespace username_policy {

constexpr size_t kMinLength = 3;
constexpr size_t kMaxLength = 24;

inline constexpr const char* kVisitorPrefix = "visitor-";
inline constexpr const char* kDeletedPrefix = "deleted-";

// Shown wherever a closed account's name would be.
inline constexpr const char* kDeletedDisplayName = "Deleted account";

// Why `name` cannot be a new username, as a sentence for the person typing
// it; "" when its shape is fine. Does not check uniqueness.
std::string ShapeError(const std::string& name);

// ShapeError, then uniqueness: `taken` says whether a name is in use by
// someone else (ignoring case). "" when the name may be used.
std::string Check(const std::string& name, const std::function<bool(const std::string&)>& taken);

// "deleted-<id>": the name a closed account is given. It can never be
// registered, so the account can never be signed into again.
std::string DeletedName(int userId);
bool IsDeletedName(const std::string& name);

// The username to show for `name`: "Deleted account" for a closed account,
// the name itself otherwise.
std::string DisplayName(const std::string& name);

// Every "deleted-<digits>" in `text` replaced by "Deleted account". The
// last step before anything leaves for a page or a model, so a closed
// account's internal name is never shown.
std::string ScrubDeletedNames(const std::string& text);

// A valid username made from an email's local part ("Bob.Smith+x@..." ->
// "Bob.Smith_x"): characters outside the rules dropped or replaced, cut to
// fit, padded when too short, moved off a reserved prefix. Not yet unique.
std::string BaseFromEmail(const std::string& email);

// `base` if it is free, else base2, base3, ... (cut so each stays within
// kMaxLength). `taken` as in Check.
std::string MakeUnique(const std::string& base, const std::function<bool(const std::string&)>& taken);

// ASCII lower case, for comparing names.
std::string Lower(const std::string& s);

}  // namespace username_policy
