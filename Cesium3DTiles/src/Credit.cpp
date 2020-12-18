#include "Cesium3DTiles/Credit.h"
#include <map>
#include <string>
#include <utility>

namespace Cesium3DTiles {

    std::map<std::string, int> Credit::creditToId = {};
    int Credit::nextId = 0;

    Credit::Credit(const std::string& html, bool showOnScreen) :
        _showOnScreen(showOnScreen)
    {
        std::pair<std::map<std::string, int>::iterator, bool> insertionResult;
        insertionResult = creditToId.insert(std::pair<std::string, int>(html, nextId));
        
        if (insertionResult.second) {
            // this is a new credit
            _id = nextId++;
        } else {
            // this is an existing credit, so assign it the old id  
            _id = insertionResult.first->second;
        }

        _html = (char*)insertionResult.first->first.c_str(); 
    }

    Credit::~Credit() { 
    }
}