#ifndef WEBMOCK_CORE_HTTP_HPP
#define WEBMOCK_CORE_HTTP_HPP

#include <iostream>
#include <vector>
#include <map>
#include <tuple>
#include <cctype>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <webmock/util/ci_value_base.hpp>
#include <webmock/util/uri_parser.hpp>

namespace webmock { namespace core { namespace http {
    struct method: public util::ci_value_base<method> {
        using util::ci_value_base<method>::ci_value_base;
        using util::ci_value_base<method>::operator =;
        
        operator std::string() const {
            std::string result;
            for (auto && c: this->data) {
                result.push_back(std::toupper(c));
            }
            return result;
        }
    };
    
    struct header_name: public util::ci_value_base<header_name> {
        using util::ci_value_base<header_name>::ci_value_base;
        using util::ci_value_base<header_name>::operator =;
        
        operator std::string() const {
            std::vector<ci_string> list;
            boost::split(list, this->data, boost::is_any_of("-"));
            
            for (auto && str: list) {
                str[0] = std::toupper(str[0]);
                for (uint32_t i=1; i < str.size(); ++i) {
                    str[i] = std::tolower(str[i]);
                }
            }
            return boost::join(list, "-").c_str();
        }
    };
    using headers = std::multimap<header_name, std::string>;
    
    struct status: public util::ci_value_base<status> {
        using util::ci_value_base<status>::ci_value_base;
        using util::ci_value_base<status>::operator =;
        
        status() : ci_value_base("200") {}
        status(unsigned int v) : ci_value_base(boost::lexical_cast<std::string>(v)) {}
        
        operator unsigned int() const {
            return boost::lexical_cast<unsigned int>(this->data.c_str());
        }
    };
    
    class url:
        boost::equality_comparable<url, url>,
        boost::less_than_comparable<url, url>
    {
        struct ci_component: public util::ci_value_base<ci_component> {
            using util::ci_value_base<ci_component>::ci_value_base;
            using util::ci_value_base<ci_component>::operator =;
            
            operator std::string() const {
                std::string result;
                for (auto && c: this->data) {
                    result.push_back(std::tolower(c));
                }
                return result;
            }
        };
        
        using port_type = uint16_t;
        using path_type = std::vector<std::string>;
        using query_type = std::map<std::string, boost::optional<std::string>>;
        
        boost::optional<ci_component> scheme;
        boost::optional<std::string> userinfo;
        boost::optional<ci_component> host;
        boost::optional<port_type> port;
        path_type path;
        boost::optional<query_type> query;
        
    public:
        url() = default;
        url(char const * value) : url(std::string(value)) {}
        url(std::string const & value) {
            util::uri_parser result(value);
            
            if (!result.scheme.empty()) this->scheme = result.scheme;
            if (!result.userinfo.empty()) this->userinfo = result.userinfo;
            if (!result.host.empty()) this->host = result.host;
            if (!result.port.empty()) this->port = boost::lexical_cast<port_type>(result.port);
            if (!this->port) this->port = this->default_port();
            
            path_type path_components;
            boost::split(path_components, result.path, boost::is_any_of("/"));
            for (auto & component: path_components) {
                if (component.empty() || component == ".") {
                    continue;
                }
                else if (component == "..") {
                    if (!this->path.empty()) {
                       this->path.pop_back();
                    }
                }
                else {
                    this->path.push_back(component);
                }
            }
            
            if (!result.query.empty()) {
                this->query = query_type{};
                std::vector<std::string> params;
                boost::split(params, result.query, boost::is_any_of("&"));
                for (auto && param: params) {
                    std::vector<std::string> param_pair;
                    boost::split(param_pair, param, boost::is_any_of("="));
                    if (param_pair.size() == 1) (*this->query)[param_pair[0]] = boost::none;
                    else                        (*this->query)[param_pair[0]] = param_pair[1];
                }
            }
        }
        
        operator std::string() const {
            std::ostringstream oss;
            
            if (this->scheme) oss << *this->scheme << ":";
            if (this->host) {
                oss << "//";
                if (this->userinfo) oss << *this->userinfo << "@";
                oss << *this->host;
                if (this->port != this->default_port()) oss << ":" << *this->port;
            }
            oss << "/" << boost::join(this->path, "/");
            
            if (this->query) {
                std::vector<std::string> params;
                for (auto && param_pair: *this->query) {
                    if (param_pair.second) {
                        params.push_back(param_pair.first +"="+ *param_pair.second);
                    }
                    else {
                        params.push_back(param_pair.first);
                    }
                }
                oss << "?" << boost::join(params, "&");
            }
            
            return oss.str();
        }
        
        friend bool operator ==(url const & lop, url const & rop) {
            bool result =
                std::tie(lop.userinfo, lop.host, lop.path, lop.query) ==
                std::tie(rop.userinfo, rop.host, rop.path, rop.query);
            
            if (!lop.scheme || !rop.scheme) {
                if (!lop.port || !rop.port) return result;
                return result && (lop.port == rop.port);
            }
            return result && (
                std::tie(lop.scheme, lop.port) ==
                std::tie(rop.scheme, rop.port)
            );
        }
        
        friend bool operator <(url const & lop, url const & rop) {
            return static_cast<std::string>(lop) < static_cast<std::string>(rop);
        }
        
        friend std::ostream & operator <<(std::ostream & lop, url const & rop) {
            return lop << static_cast<std::string>(rop);
        }
        
    private:
        boost::optional<port_type> default_port() const {
            if (this->scheme == ci_component("http")) return 80;
            if (this->scheme == ci_component("https")) return 443;
            return boost::none;
        }
    };
}}}

#endif
