#include <fstream>
#include <iostream>
#include <sstream>

#include "DFSLConfig.h"

namespace DFSL {

void Config::resetDefault(){
    if (mIsDefault) {
        return;
    }
    mValue = mDefaultValue;
    mIsDefault = true;
}

bool Config::isDefault(){
    return mIsDefault;
}

ConfigType Config::getType(){
    return mType;
}

const std::string& Config::getDescription(){
    return mDescription;
}

void ConfigList::initAllConfigs(){
    // two ways to add a new config to the list:
    // newConfig<Type>("ConfigName", defaultValue,        "Description")
    // newConfig<Type>("ConfigName", defaultValue, Value, "Description")
    newConfig<int>   ("OutputLevel"             , ConfigType::INT   ,   2                , "0 : Errors\n1 : Warnings\n2 : Standard info\n3 : Verbose info\n4 : Debug");

    newConfig<double>("MaxCostCutoff"           , ConfigType::DOUBLE,   5000.0           , "max cost cutoff for dfs"             );

    newConfig<double>("OBAreaWeight"            , ConfigType::DOUBLE,   750.0            , ""                                    );
    newConfig<double>("OBUtilWeight"            , ConfigType::DOUBLE,   1000.0           , ""                                    );
    newConfig<double>("OBAspWeight"             , ConfigType::DOUBLE,   100.0            , ""                                    );
    newConfig<double>("OBUtilPosRein"           , ConfigType::DOUBLE,   -500.0           , ""                                    );
    newConfig<double>("BWUtilWeight"            , ConfigType::DOUBLE,   1500.0           , ""                                    );
    newConfig<double>("BWUtilPosRein"           , ConfigType::DOUBLE,   -500.0           , ""                                    );
    newConfig<double>("BWAspWeight"             , ConfigType::DOUBLE,   90.0             , ""                                    );
    newConfig<double>("BBAreaWeight"            , ConfigType::DOUBLE,   150.0            , ""                                    );
    newConfig<double>("BBFromUtilWeight"        , ConfigType::DOUBLE,   900.0            , ""                                    );
    newConfig<double>("BBFromUtilPosRein"       , ConfigType::DOUBLE,   -500.0           , ""                                    );
    newConfig<double>("BBToUtilWeight"          , ConfigType::DOUBLE,   1750.0           , ""                                    );
    newConfig<double>("BBToUtilPosRein"         , ConfigType::DOUBLE,   -100.0           , ""                                    );
    newConfig<double>("BBAspWeight"             , ConfigType::DOUBLE,   30.0             , ""                                    );
    newConfig<double>("BBFlatCost"              , ConfigType::DOUBLE,   150.0            , ""                                    );

    newConfig<double>("WWFlatCost"              , ConfigType::DOUBLE,   1000000.0        , ""                                    );
    
    newConfig<bool>  ("ExactAreaMigration"        , ConfigType::BOOL,     false             , "Controls if exact area migration"    ); // see note 1
    // unused
    newConfig<bool>  ("MigrationAreaLimit"        , ConfigType::BOOL,     false            , ""                                    );
    newConfig<double>("MaxMigrationAreaSingleIter", ConfigType::DOUBLE, 0.01             , ""                                    );
}


// config file syntax:
// \# this line is a comment
// [config 1] = [value]
// [config 2] =[value]
// [config 3]= [value]
// [config 4]=[value]
bool ConfigList::readConfigFile(std::string filename){
    std::ifstream fin(filename, std::ifstream::in);
    if (!fin.is_open()){
        std::cerr << "Input \"" << filename << "\" not opened " << std::endl;
        return false;
    }
    std::string line;
    // std::ws : modifier that discards leading whitespace from input stream
    while (std::getline(fin >> std::ws, line)){
        if (line[0] == '#'){
            continue;
        }
        size_t equalPosition = line.find_last_of('=');
        // find position of equal sign
        if (equalPosition == std::string::npos){
            return false;
        }
        std::istringstream configSS(line.substr(0, equalPosition));
        std::istringstream valueSS(line.substr(++equalPosition));
// HELP
// Is there a way to:
// 1. dynamically determine the underlying data type of the config
//      based on the config name during runtime (ie. when the config name is read)
// 2. dynamically create a variable of that data type and read the value from the config file
// 3. store that value inside the config 
// ????
// bandaid solution: enum class in config to store data type
        std::string configName;
        configSS >> configName;
        try {
            Config& config = getConfig(configName);

            switch (config.getType())
            {
            case ConfigType::BOOL: {
                bool var;
                valueSS >> var;
                config.setValue<bool>(var);
                break;
            }
            case ConfigType::INT: {
                int var;
                valueSS >> var;
                config.setValue<int>(var);
                break;
            }
            case ConfigType::UINT: {
                unsigned int var;
                valueSS >> var;
                config.setValue<unsigned int>(var);
                break;
            }
            case ConfigType::LONG64: {
                long long var;
                valueSS >> var;
                config.setValue<long long>(var);
                break;
            }
            case ConfigType::ULONG64: {
                unsigned long long var;
                valueSS >> var;
                config.setValue<unsigned long long>(var);
                break;
            }
            case ConfigType::DOUBLE: {
                double var;
                valueSS >> var;
                config.setValue<double>(var);
                break;
            }
            case ConfigType::FLOAT: {
                float var;
                valueSS >> var;
                config.setValue<float>(var);
                break;
            }
            case ConfigType::CHAR: {
                char var;
                valueSS >> var;
                config.setValue<char>(var);
                break;
            }
            case ConfigType::STRING: {
                std::string var;
                valueSS >> var;
                config.setValue<std::string>(var);
                break;
            }
            default:
                break;
            }
        }
        catch(std::out_of_range& e) {
            std::cerr << "ERROR parsing config file.\n";
            return false;
        }
    }
    return true;
}   

void ConfigList::resetAllDefault(){
    for (auto it = mConfigMap.begin(); it != mConfigMap.end(); ++it){
        it->second.resetDefault();
    }
}

Config& ConfigList::getConfig(std::string configName){
    try{
        return mConfigMap.at(configName);
    }
    catch(std::out_of_range& e){
        std::cerr << e.what() << std::endl;
        throw;
    }
}

}

/*
Note 1: 

*/