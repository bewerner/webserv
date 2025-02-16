#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <regex>
#include <set>


#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <stdio.h>
#include <unistd.h>

#include "Data.hpp"

void parser(std::vector<Server>& data, std::string confPath);
