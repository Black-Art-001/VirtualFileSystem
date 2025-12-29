#pragma once

#include <stdexcept>
#include <iostream> 

#define ERROR_HANDELING 

#ifdef ERROR_HANDELING
#define error(type , msg) throw type (msg) 
#elif defined(MASSAGE_HANDELING) 
#define error(type , msg) std::cerr << msg << std::endl  
#else 
#define error(type , msg) 
#endif 

#define VALIDATOR

#ifdef VALIDATOR
#define check_if(condition , type , msg) if (condition) error(type, msg); 
#else
check_if(condition, type, msg)
#endif