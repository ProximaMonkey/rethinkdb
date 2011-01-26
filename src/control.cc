#include "control.hpp"
#include "logger.hpp"
#include "errors.hpp"
#include "concurrency/multi_wait.hpp"

control_map_t &get_control_map() {
    /* Getter function so that we can be sure that control_list is initialized before it is needed,
    as advised by the C++ FAQ. Otherwise, a control_t  might be initialized before the control list
    was initialized. */
    
    static control_map_t control_map;
    return control_map;
}

spinlock_t &get_control_lock() {
    /* To avoid static initialization fiasco */
    
    static spinlock_t lock;
    return lock;
}

std::string control_exec(std::string key) {
    std::string res;

    for (control_map_t::iterator it = get_control_map().find(key); it != get_control_map().end() && (*it).first == key; it++)
        res += (*it).second->call();

    return res;
}

std::string control_help() {
    std::string res, last_key, last_help;
    for (control_map_t::iterator it = get_control_map().begin(); it != get_control_map().end(); it++) {
        if (((*it).first != last_key || (*it).second->help != last_help) && (*it).second->help.length() > 0)
            res += ((*it).first + std::string(": ") + (*it).second->help + std::string("\r\n"));

        last_key = (*it).first;
        last_help = (*it).second->help;
    }
    return res;
}

control_t::control_t(std::string key, std::string help) 
    : key(key), help(help)
{ //TODO locks @jdoliner
    get_control_lock().lock();
    get_control_map().insert(std::pair<std::string, control_t*>(key, this));
    get_control_lock().unlock();
}

control_t::~control_t() {
    get_control_lock().lock();
    for (control_map_t::iterator it = get_control_map().find(key); it != get_control_map().end(); it++)
        if ((*it).second == this)
            get_control_map().erase(it);
    get_control_lock().unlock();
}

/* Example of how to add a control */
struct hi_t : public control_t
{
private:
    int counter;
public:
    hi_t(std::string key)
        : control_t(key, std::string("")), counter(0)
    {
        guarantee(key != ""); //this could potentiall cause some errors with control_help
    }
    std::string call() {
        counter++;
        if (counter < 3)
            return std::string("Salutations, user.\n");
        else if (counter < 4)
            return std::string("Say hi again, I dare you.\n");
        else
            return std::string("Base QPS decreased by 100,000.\n");
    }
};

hi_t hi(std::string("hi"));
