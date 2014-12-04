#include <webmock/condition_list.hpp>

namespace webmock {
    void condition_list::add(condition_type condition) {
        this->conditions.push_back(condition);
    }
    
    bool condition_list::match(webmock::request const & request) const {
        for (auto && condition: this->conditions) {
            if (!condition(request)) return false;
        }
        return true;
    }
}
