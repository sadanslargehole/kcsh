#include <functional>
#include <string>
#include <variant>
#include <vector>

using Replacement = std::variant<std::string, std::function<std::string()>>;