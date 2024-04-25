#include "cSException.h"


CSException::CSException(std::string exception_msg)
    : m_Exception_MSG(exception_msg) {
}

const char* CSException::what(){
    std::unordered_map<std::string, const char*>::const_iterator lutit = CSEXCEPTION_LUT.find(m_Exception_MSG);
    if(lutit == CSEXCEPTION_LUT.end()){
        return CSEX_EMPTY;
    }else{
        m_Exception_MSG += " ";
        m_Exception_MSG.append(lutit->second);
        return m_Exception_MSG.c_str();
    }
}