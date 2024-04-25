#ifndef _DFSCONFIG_H_
#define _DFSCONFIG_H_

#include <any>
#include <map>
#include <string>
#include <iostream>

// data structure to facilitate easy access of legalization related configs
// core data structure: std::map<std::string, Config>
// two ways to get value of config:
// 1. myConfigList.getConfig("ConfigName").getValue<Type>()
// 2. myConfigList.getConfigValue<Type>("ConfigName")

namespace DFSL {

class Config;
class ConfigList;


enum class ConfigType : unsigned char { BOOL, INT, UINT, LONG64, ULONG64, DOUBLE, FLOAT, CHAR, STRING };

enum DFSL_PRINTLEVEL : int {
    DFSL_ERROR    = 0,
    DFSL_WARNING  = 1,
    DFSL_STANDARD = 2,
    DFSL_VERBOSE  = 3,
    DFSL_DEBUG    = 4
}; 

class Config {
private:
    std::any mValue;
    bool mIsDefault;
    std::any mDefaultValue;
    std::string mDescription;
    ConfigType mType;

public:
    // do not allow default constructor (empty parentheses constructor)
    Config() = delete;

    // config constructor 1 (value provided)
    template<typename T>
    Config(ConfigType valueType, T defaultValue, T value, std::string& description){
        mType = valueType;
        mIsDefault = defaultValue == value;
        mDescription = description;
        mDefaultValue = defaultValue;
        mValue = value;
    }    

    // config constructor 2 (use default value)
    template<typename T>
    Config(ConfigType valueType, T defaultValue, std::string& description){
        mType = valueType;
        mIsDefault = true;
        mDescription = description;
        mDefaultValue = defaultValue;
        mValue = defaultValue;
    }

    // gets CONSTANT reference to config's value
    template<typename T>
    const T& getValue(){
        try {
            return std::any_cast<T&>(mValue);
        }
        catch(std::bad_any_cast &e){
            std::cerr << e.what() << std::endl;
            return T();
        }
    }

    // gets CONSTANT reference to config's default value
    template<typename T>
    const T& getDefault(){
        try {
            return std::any_cast<T&>(mDefaultValue);
        }
        catch(std::bad_any_cast &e){
            std::cerr << e.what() << std::endl;
            return T();
        }
    }

    // sets config's value to newValue
    // also checks if newValue is equal to default value
    // also checks for type equivalence 
    template<typename T>
    void setValue(T newValue){
        try{
            if (std::any_cast<T>(mDefaultValue) == newValue){
                mIsDefault = true;
            }
            else {
                mIsDefault = false;
                mValue = newValue;
            }
        }
        catch(std::bad_any_cast &e){
            std::cerr << e.what() << std::endl;
        }
    }

    void resetDefault();
    bool isDefault();
    ConfigType getType();
    const std::string& getDescription();
};

class ConfigList{
private: 
    std::map<std::string, Config> mConfigMap;
public: 
    // gets value according to config name
    // it is up to the user to make sure the type is correct
    // incorrect configName throws std::out_of_range error
    template<typename T>
    const T getConfigValue(std::string configName){
        try {
            Config& test = mConfigMap.at(configName);
            return test.getValue<T>();
        }
        catch (std::out_of_range &e){
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

    // sets value according to config name
    // it is up to the user to make sure the type is correct
    // incorrect configName throws std::out_of_range error
    template<typename T>
    void setConfigValue(std::string configName, T newValue){
        try {
            Config& test = mConfigMap.at(configName);
            test.setValue<T>(newValue);
        }
        catch (std::out_of_range &e){
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

    // returns false if key (configName) already exists
    template<typename T>
    bool newConfig(const char* configName, ConfigType valueType, T defaultValue, T value, const char* description){
        std::string descString(description);
        auto [it, success] = mConfigMap.insert({std::string(configName), Config(valueType, defaultValue, value, descString)});
        return success;
    }

    // returns false if key (configName) already exists
    template<typename T>
    bool newConfig(const char* configName, ConfigType valueType, T defaultValue, const char* description){
        std::string descString(description);
        auto [it, success] = mConfigMap.insert({std::string(configName), Config(valueType, defaultValue, descString)});
        return success;
    }

    void initAllConfigs();
    bool readConfigFile(std::string filename);
    void resetAllDefault();
    Config& getConfig(std::string configName);
};

}

#endif