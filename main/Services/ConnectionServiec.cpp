#include "ConnectionService.hpp"

int ConnectionService::publicMethod() {
    return privateMethod();
}

int ConnectionService::privateMethod() {
    return testInteger * 2;
}
