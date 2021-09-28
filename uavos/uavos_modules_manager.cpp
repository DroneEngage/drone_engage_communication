#include <iostream>


#include <exception>
#include <typeinfo>
#include <stdexcept>


#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 


#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"


#include "../messages.hpp"
#include "../comm_server/andruav_unit.hpp"
#include "../udpCommunicator.hpp"
#include "../configFile.hpp"
#include "../comm_server/andruav_comm_server.hpp"
#include "../uavos/uavos_modules_manager.hpp"


uavos::CUavosModulesManager::~CUavosModulesManager()
{

}


/**
 * creates JSON message that identifies Module
**/
Json uavos::CUavosModulesManager::createJSONID (const bool& reSend)
{
    try
    {
    
        uavos::CConfigFile& cConfigFile = uavos::CConfigFile::getInstance();
        const Json& jsonConfig = cConfigFile.GetConfigJSON();
        Json jsonID;        
        
        jsonID[INTERMODULE_COMMAND_TYPE] =  CMD_TYPE_INTERMODULE;
        jsonID[ANDRUAV_PROTOCOL_MESSAGE_TYPE] =  TYPE_AndruavModule_ID;
        Json ms;
        
        ms["a"] = jsonConfig["module_id"];
        ms["b"] = jsonConfig["module_class"];
        ms["c"] = jsonConfig["module_messages"];
        ms["d"] = Json();
        ms["e"] = jsonConfig["module_key"]; 
        ms["f"] = 
        {
            {"sd", jsonConfig["partyID"]},
            {"gr", jsonConfig["groupID"]}
        };
        
        // this is NEW in communicator and could be ignored by current UAVOS modules.
        ms["z"] = reSend;

        jsonID[ANDRUAV_PROTOCOL_MESSAGE_CMD] = ms;
        
        return jsonID;

                /* code */
    }
    catch (...)
    {
        //https://stackoverflow.com/questions/315948/c-catching-all-exceptions/24142104
        std::exception_ptr p = std::current_exception();
        std::clog <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;
        
        return Json();
    }

}


bool uavos::CUavosModulesManager::updateUavosPermission (const Json& module_permissions)
{
    uavos::CAndruavUnitMe& andruav_unit_me = uavos::CAndruavUnitMe::getInstance();
    bool updated = false;
    const int&  len = module_permissions.size();
    for (int i=0; i<len; ++i)
    {
        const std::string& permission_item = module_permissions[i].get<std::string>();
        if (permission_item.compare("T") ==0)
        {
            uavos::ANDRUAV_UNIT_INFO& andruav_unit_info = andruav_unit_me.getUnitInfo();
            if (andruav_unit_info.permission[4]=='T') break;
            andruav_unit_info.permission[4] = 'T';
            andruav_unit_info.use_fcb = true;
            updated = true;
        }
        else if (permission_item.compare("R") ==0)
        {
            uavos::ANDRUAV_UNIT_INFO& andruav_unit_info = andruav_unit_me.getUnitInfo();
            if (andruav_unit_info.permission[6]=='R') break;
            andruav_unit_info.permission[6] = 'R';
            andruav_unit_info.use_fcb = true;
            updated = true;
        }
        else if (permission_item.compare("V") ==0)
        {
            uavos::ANDRUAV_UNIT_INFO& andruav_unit_info = andruav_unit_me.getUnitInfo();
            if (andruav_unit_info.permission[8]=='V') break;
            andruav_unit_info.permission[8] = 'V';
            updated = true;
        }
        else if (permission_item.compare("C") ==0)
        {
            uavos::ANDRUAV_UNIT_INFO& andruav_unit_info = andruav_unit_me.getUnitInfo();
            if (andruav_unit_info.permission[10]=='C') break;
            andruav_unit_info.permission[10] = 'C';
            updated = true;
        }
        
    }

    return updated;
}


void uavos::CUavosModulesManager::cleanOrphanCameraEntries (const std::string& module_id, const uint64_t& time_now)
{
    auto camera_module = m_camera_list.find(module_id);
    if (camera_module == m_camera_list.end()) return ;

    std::map <std::string, std::unique_ptr<MODULE_CAMERA_ENTRY>> *camera_entry_list= camera_module->second.get();

    std::map <std::string, std::unique_ptr<MODULE_CAMERA_ENTRY>>::iterator camera_entry_itrator;
    std::vector <std::string> orphan_list;
    for (camera_entry_itrator = camera_entry_list->begin(); camera_entry_itrator != camera_entry_list->end(); camera_entry_itrator++)
    {   
            const MODULE_CAMERA_ENTRY * camera_entry= camera_entry_itrator->second.get();

            if (camera_entry->module_last_access_time < time_now)
            {
                // old and should be removed
                orphan_list.push_back (camera_entry->global_index);
            }
    }


    //TODO: NOT TESTED
    // remove orphans 
    for(auto const& value: orphan_list) {
        camera_entry_list->erase(value);
    }
    
}


void uavos::CUavosModulesManager::updateCameraList(const std::string& module_id, const Json& msg_cmd)
{

    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: updateCameraList " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    // Check if Module "Camera_ModuleName" is listed.
    auto camera_module = m_camera_list.find(module_id);
    if (camera_module == m_camera_list.end()) 
    {
        // Module Not found in camera list
        #ifdef DEBUG
            std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: updateCameraList // Module Not found in camera list" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

        std::map <std::string, std::unique_ptr<MODULE_CAMERA_ENTRY>> *pcamera_entries = new std::map <std::string, std::unique_ptr<MODULE_CAMERA_ENTRY>>();
        m_camera_list.insert(std::make_pair(module_id, std::unique_ptr<std::map <std::string, std::unique_ptr<MODULE_CAMERA_ENTRY>>>(pcamera_entries)));
    }


    // Retrieve list of camera entries of this module.
    camera_module = m_camera_list.find(module_id);

    std::map <std::string, std::unique_ptr<MODULE_CAMERA_ENTRY>> *camera_entry_list= camera_module->second.get();

    // List of camera devices in a camera module recieved by intermodule message.
    Json camera_array = msg_cmd["m"];

    // iterate over camera devices in recieved json message.
    const int messages_length = camera_array.size(); 
    const uint64_t now_time = get_time_usec();
    for (int i=0; i< messages_length; ++i)
    {
        
        Json jcamera_entry = camera_array[i];
        // camera device id
        const std::string& camera_entry_id = jcamera_entry["id"].get<std::string>();
        
        
        auto camera_entry_record = camera_entry_list->find(camera_entry_id);
        if (camera_entry_record == camera_entry_list->end()) 
        {
            // camera entry not listed in cameras list of submodule
            MODULE_CAMERA_ENTRY * camera_entry = new MODULE_CAMERA_ENTRY();
            camera_entry->module_id  = module_id;
            camera_entry->global_index = camera_entry_id;
            camera_entry->logical_name = jcamera_entry["ln"].get<std::string>();
            camera_entry->is_recording = jcamera_entry["r"].get<bool>();
            camera_entry->is_camera_avail = jcamera_entry["v"].get<bool>();
            camera_entry->is_camera_streaming = jcamera_entry["active"].get<int>();
            camera_entry->camera_type = jcamera_entry["p"].get<int>();
            
            camera_entry->module_last_access_time = now_time;
            camera_entry->updates = true;
            camera_entry_list->insert(std::make_pair(camera_entry_id, std::unique_ptr<MODULE_CAMERA_ENTRY> (camera_entry) ));
        
        }
        else
        {
            //camera listed
            
            MODULE_CAMERA_ENTRY * camera_entry = camera_entry_record->second.get();
            camera_entry->module_id  = module_id;
            camera_entry->global_index = camera_entry_id;
            camera_entry->logical_name = jcamera_entry["ln"].get<std::string>();
            camera_entry->is_recording = jcamera_entry["r"].get<bool>();
            camera_entry->is_camera_avail = jcamera_entry["v"].get<int>();
            camera_entry->is_camera_streaming = jcamera_entry["active"].get<int>();
            camera_entry->camera_type = jcamera_entry["p"].get<int>();
            
            camera_entry->module_last_access_time = now_time;
            camera_entry->updates = true;

        }
    }

    cleanOrphanCameraEntries(module_id, now_time);
}


Json uavos::CUavosModulesManager::getCameraList()
{
    Json camera_list = Json::array();

    MODULE_CAMERA_LIST::iterator camera_module;
    for (camera_module = m_camera_list.begin(); camera_module != m_camera_list.end(); camera_module++)
    {   
        std::map <std::string, std::unique_ptr<MODULE_CAMERA_ENTRY>> * camera_entry_list = camera_module->second.get();
        
        std::map <std::string, std::unique_ptr<MODULE_CAMERA_ENTRY>>::iterator camera_entry_itrator;

        for (camera_entry_itrator = camera_entry_list->begin(); camera_entry_itrator != camera_entry_list->end(); camera_entry_itrator++)
        {   
            const MODULE_CAMERA_ENTRY * camera_entry= camera_entry_itrator->second.get();
    
            
            
            Json json_camera_entry =
            {
                // check uavos_camera_plugin
                {"v", camera_entry->is_camera_avail},
                {"ln", camera_entry->logical_name},
                {"id", camera_entry->global_index},
                {"active", camera_entry->is_camera_streaming},
                {"r", camera_entry->is_recording},
                {"p", camera_entry->camera_type}

            };
            camera_list.push_back(json_camera_entry);
        }
    }

    return camera_list;

}


bool uavos::CUavosModulesManager::updateModuleSubscribedMessages (const std::string& module_id, const Json& message_array)
{
    bool new_module = false;

    const int messages_length = message_array.size(); 
    for (int i=0; i< messages_length; ++i)
    {
        /**
        * @brief 
        * select list of a given message id.
        * * &v should be by reference to avoid making fresh copy.
        */
        std::vector<std::string> &v = m_module_messages[message_array[i].get<int>()];
        if (std::find(v.begin(), v.end(), module_id) == v.end())
        {
            /**
            * @brief 
            * add module in the callback list.
            * when this message is received from andruav-server it should be 
            * forwarded to this list.
            */
            v.push_back(module_id);
            new_module = true;
        }
    }

    return new_module;
}


bool uavos::CUavosModulesManager::handleModuleRegistration (const Json& msg_cmd, const struct sockaddr_in* ssock)
{

    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: handleModuleRegistration " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    bool updated = false;

    const uint64_t &now = get_time_usec();
            
    // array of message IDs
    
    MODULE_ITEM_TYPE * module_item;
    const std::string& module_id = std::string(msg_cmd["a"].get<std::string>()); 
    /**
    * @brief insert module in @param m_modules_list
    * this is the main list of modules.
    */
    auto module_entry = m_modules_list.find(module_id);
    if (module_entry == m_modules_list.end()) 
    {
        // New Module not registered in m_modules_list

        module_item = new MODULE_ITEM_TYPE();
        module_item->module_key         = msg_cmd["e"].get<std::string>();
        module_item->module_id          = module_id;
        module_item->module_class       = msg_cmd["b"].get<std::string>(); // fcb, video, ...etc.
        module_item->modules_features   = msg_cmd["d"];
        
        struct sockaddr_in * module_address = new (struct sockaddr_in)();  
        memcpy(module_address, ssock, sizeof(struct sockaddr_in)); 
                
        module_item->m_module_address = std::unique_ptr<struct sockaddr_in>(module_address);
                
        m_modules_list.insert(std::make_pair(module_item->module_id, std::shared_ptr<MODULE_ITEM_TYPE>(module_item)));


    }
    else
    {
        module_item = module_entry->second.get();
                
        // Update Module Info
     
        if ((module_item->module_last_access_time!=0)
            && (now - module_item->module_last_access_time >= MODULE_TIME_OUT))
        {
            //TODO Event Module Restored
            module_item->is_dead = true;
        }
    }

    module_item->module_last_access_time = now;

            
    // insert message callback
    
    const Json& message_array = msg_cmd["c"]; 
    updated |= updateModuleSubscribedMessages(module_id, message_array);

    const std::string module_type = msg_cmd["b"].get<std::string>(); 
    if (module_type.find("camera")==0)
    {
        // update camera list
        updateCameraList(module_id, msg_cmd);
    }   

    updated |= updateUavosPermission(msg_cmd["d"]);

    // reply with identification if required by module
    if (validateField(msg_cmd, "z", Json::value_t::boolean))
    {
        if (msg_cmd["z"].get<bool>() == true)
        {
            const Json &msg = createJSONID(false);
            struct sockaddr_in module_address = *module_item->m_module_address.get();  
            //memcpy(module_address, ssock, sizeof(struct sockaddr_in)); 
                
            uavos::comm::CUDPCommunicator::getInstance().SendJMSG(msg.dump(), &module_address);
        }
    }

    return updated;
}


/**
 * @brief 
 * 
 * @details 
 * @param full_mesage 
 * @param full_mesage_length 
 * @param ssock sender module ip & port
 */
void uavos::CUavosModulesManager::parseIntermoduleBinaryMessage (Json& jsonMessage, const char * full_mesage, const int full_mesage_length, const struct sockaddr_in* ssock)
{
    // Intermodule Message
    const int mt = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
    
    uavos::andruav_servers::CAndruavCommServer& commServer = uavos::andruav_servers::CAndruavCommServer::getInstance();
    
    const Json ms = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    
    int binary_length = 0; 
    int binary_start_index = 0;
    for (int i=0; i<full_mesage_length; ++i)
    { 
        if (full_mesage[i]==0)
        {
            
            binary_start_index = i + 1;
            binary_length = full_mesage_length - binary_start_index;
            break;
        }

    }

    const char * binaryMessage = &(full_mesage[binary_start_index]);

    
    if (jsonMessage.contains(INTERMODULE_COMMAND_TYPE) && jsonMessage[INTERMODULE_COMMAND_TYPE].get<std::string>().find(CMD_COMM_INDIVIDUAL) != std::string::npos)
    {
        // messages to comm-server
        commServer.API_sendBinaryCMD(jsonMessage[ANDRUAV_PROTOCOL_TARGET_ID].get<std::string>(), mt, binaryMessage, binary_length);
    }
    else
    {
        commServer.API_sendBinaryCMD(std::string(), mt, binaryMessage, binary_length);
    }
            
}

/**
 * @details 
 * Handels all messages recieved form a module.
 * @link CMD_TYPE_INTERMODULE @endlink message types contain many commands. most important is @link TYPE_AndruavModule_ID @endlink
 * Other Andruav based commands can be sent using this type of messages if uavos module wants uavos communicator to process 
 * the message before forwarding it. Although it is not necessary to forward the message to Andruav-Server.
 * @param jsonMessage Json object message.
 * @param address sender module ip & port
 */
void uavos::CUavosModulesManager::parseIntermoduleMessage (Json& jsonMessage, const struct sockaddr_in* ssock)
{
    // Intermodule Message
    const int mt = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
    
    const Json ms = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    switch (mt)
    {
        case TYPE_AndruavModule_ID:
        {
            const bool updated = handleModuleRegistration (ms, ssock);
            
            if (updated == true)
            {
                uavos::andruav_servers::CAndruavCommServer& commServer = uavos::andruav_servers::CAndruavCommServer::getInstance();
                commServer.API_sendID(std::string());
            }
            
        }
        break;

        case TYPE_AndruavResala_ID:
        {
            uavos::CAndruavUnitMe& m_andruavMe = uavos::CAndruavUnitMe::getInstance();
            uavos::ANDRUAV_UNIT_INFO&  unit_info = m_andruavMe.getUnitInfo();
            
            unit_info.vehicle_type              = ms["VT"].get<int>();
            unit_info.flying_mode               = ms["FM"].get<int>();
            unit_info.gps_mode                  = ms["GM"].get<int>();
            unit_info.use_fcb                   = ms["FI"].get<bool>();
            unit_info.is_armed                  = ms["AR"].get<bool>();
            unit_info.is_flying                 = ms["FL"].get<bool>();
            unit_info.telemetry_protocol        = ms["TP"].get<int>();
            unit_info.flying_last_start_time    = ms["z"].get<long long>();
            unit_info.flying_total_duration     = ms["a"].get<long long>();
            unit_info.is_tracking_mode          = ms["b"].get<bool>();
            unit_info.manual_TX_blocked_mode    = ms["C"].get<int>();
            unit_info.is_gcs_blocked            = ms["B"].get<bool>();

            uavos::andruav_servers::CAndruavCommServer& commServer = uavos::andruav_servers::CAndruavCommServer::getInstance();
            commServer.API_sendID(std::string());
        }
        break;

        default:
        {
            uavos::andruav_servers::CAndruavCommServer& commServer = uavos::andruav_servers::CAndruavCommServer::getInstance();
            if (jsonMessage.contains(INTERMODULE_COMMAND_TYPE) && jsonMessage[INTERMODULE_COMMAND_TYPE].get<std::string>().find(CMD_COMM_INDIVIDUAL) != std::string::npos)
            {
                // messages to comm-server
                commServer.API_sendCMD(jsonMessage[ANDRUAV_PROTOCOL_TARGET_ID].get<std::string>(), mt, ms.dump());
            }
            else
            {
                commServer.API_sendCMD(std::string(), mt, ms.dump());
            }
            
        }
        break;
    }
}

/**
 * @brief Process messages comming from AndruavServer and forward it to subscribed modules.
 * 
 * @param sender_party_id 
 * @param command_type 
 * @param jsonMessage 
 */
void uavos::CUavosModulesManager::processIncommingServerMessage (const std::string& sender_party_id, const int& command_type, const Json& jsonMessage)
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: processIncommingServerMessage " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    std::vector<std::string> &v = m_module_messages[command_type];
    for(std::vector<std::string>::iterator it = v.begin(); it != v.end(); ++it) 
    {
        std::cout << *it << std::endl;
        auto uavos_module = m_modules_list.find(*it);
        if (uavos_module == m_modules_list.end()) 
        {
            // module not available
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Module " << *it  << " for message " << command_type << " is not available" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            
            continue;
        }
        else
        {
            
            MODULE_ITEM_TYPE * module_item = uavos_module->second.get();        
            forwardMessageToModule (jsonMessage, module_item);
        }
    }

    return ;
}


/**
 * @brief forward a message from Andruav Server to a module.
 * Normally this module is subscribed in this message id.
 * 
 * @param jsonMessage 
 * @param module_item 
 */
void uavos::CUavosModulesManager::forwardMessageToModule (const Json& jsonMessage, const MODULE_ITEM_TYPE * module_item)
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: forwardMessageToModule: " << jsonMessage.dump() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    
    //const Json &msg = createJSONID(false);
    struct sockaddr_in module_address = *module_item->m_module_address.get();  
                
    uavos::comm::CUDPCommunicator::getInstance().SendJMSG(jsonMessage.dump(), &module_address);

    return ;
}


bool uavos::CUavosModulesManager::HandleDeadModules ()
{
    static std::mutex g_i_mutex; 

    const std::lock_guard<std::mutex> lock(g_i_mutex);
    
    bool dead_found = false;

    const uint64_t &now = get_time_usec();
    
    MODULE_ITEM_LIST::iterator it;
    
    for (it = m_modules_list.begin(); it != m_modules_list.end(); it++)
    {
        MODULE_ITEM_TYPE * module_item = it->second.get();
        const uint64_t diff =  (now - module_item->module_last_access_time);

        if (diff > MODULE_TIME_OUT)
        {
            
            if (!module_item->is_dead)
            {
                //TODO Event Module Warning
                module_item->is_dead = true;
                dead_found = true;
            }
            
           
        }
        else
        {
            if (module_item->is_dead)
            {
                //TODO Event Module Restored
                module_item->is_dead = false;
            }
            
        }

    }
    
    return dead_found;
}