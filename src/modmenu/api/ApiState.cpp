#include "modmenu/api/ApiInternal.hpp"

#include <unordered_map>

namespace nugget::modmenu::api {
namespace {

std::unordered_map<std::string, Registration> registrations;

}

Registration FindRegistration(const std::string& id) {
    const auto it = registrations.find(id);
    return it == registrations.end() ? Registration{} : it->second;
}

void StoreRegistration(const std::string& id, Registration registration) {
    registrations.insert_or_assign(id, std::move(registration));
}

void EraseRegistration(const std::string& id) { registrations.erase(id); }

}
