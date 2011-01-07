/*
 This file is part of MMOServer. For more information, visit http://swganh.com
 
 Copyright (c) 2006 - 2010 The SWG:ANH Team

 MMOServer is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 MMOServer is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with MMOServer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "LoginServer.h"

#include <iostream>
#include <fstream>

// Fix for issues with glog redefining this constant
#ifdef ERROR
#undef ERROR
#endif
#include <glog/logging.h>

#include <mysql_driver.h>

#include "anh/memory.h"
#include "anh/database/database_manager.h"
#include "anh/server_directory/datastore.h"
#include "anh/server_directory/server_directory.h"

#include "LoginManager.h"
#include "Common/BuildInfo.h"

#include "NetworkManager/NetworkManager.h"
#include "NetworkManager/Service.h"

#include "DatabaseManager/Database.h"
#include "DatabaseManager/DatabaseManager.h"

#include "NetworkManager/MessageFactory.h"
#include "Utils/utils.h"

#include <boost/thread/thread.hpp>
#include "Utils/clock.h"

using std::make_shared;
using std::shared_ptr;

//======================================================================================================================
LoginServer* gLoginServer = 0;


//======================================================================================================================
LoginServer::LoginServer(int argc, char* argv[]) 
    : BaseServer()
    , database_manager_(make_shared<anh::database::DatabaseManager>(sql::mysql::get_driver_instance()))
    , server_directory_(nullptr)
    , mNetworkManager(0)
{
    Anh_Utils::Clock::Init();
    LOG(WARNING) << "Login Server Startup";

    // Load Configuration Options
    std::list<std::string> config_files;
    config_files.push_back("config/general.cfg");
    config_files.push_back("config/loginserver.cfg");
    LoadOptions_(argc, argv, config_files);

    // register our available storage types with the database manager
    database_manager_->registerStorageType("global", 
        configuration_variables_map_["DBGlobalSchema"].as<std::string>(),
        configuration_variables_map_["DBServer"].as<std::string>(),
        configuration_variables_map_["DBUser"].as<std::string>(),
        configuration_variables_map_["DBPass"].as<std::string>());

    std::string cluster_name = configuration_variables_map_["cluster_name"].as<std::string>();

    std::cout << "Joining the [" << cluster_name << "] cluster\n";

    server_directory_ = make_shared<anh::server_directory::ServerDirectory>(
        make_shared<anh::server_directory::Datastore>(database_manager_->getConnection("global")),
        cluster_name);

    if(!server_directory_->registerProcess("login_service", "login", "v1.0.0", configuration_variables_map_["BindAddress"].as<std::string>(), 0, configuration_variables_map_["BindPort"].as<uint16_t>(), 0)) {
        throw std::exception("Unable to register login process");
    }

    // Initialize our modules.
    MessageFactory::getSingleton(configuration_variables_map_["GlobalMessageHeap"].as<uint32_t>());

    mNetworkManager = new NetworkManager( NetworkConfig(configuration_variables_map_["ReliablePacketSizeServerToServer"].as<uint16_t>(), 
        configuration_variables_map_["UnreliablePacketSizeServerToServer"].as<uint16_t>(), 
        configuration_variables_map_["ReliablePacketSizeServerToClient"].as<uint16_t>(), 
        configuration_variables_map_["UnreliablePacketSizeServerToClient"].as<uint16_t>(), 
        configuration_variables_map_["ServerPacketWindowSize"].as<uint32_t>(), 
        configuration_variables_map_["ClientPacketWindowSize"].as<uint32_t>(),
        configuration_variables_map_["UdpBufferSize"].as<uint32_t>()));

    LOG(WARNING) << "Config port set to " << configuration_variables_map_["BindPort"].as<uint16>();
    mService = mNetworkManager->GenerateService((char*)configuration_variables_map_["BindAddress"].as<std::string>().c_str(), configuration_variables_map_["BindPort"].as<uint16_t>(),configuration_variables_map_["ServiceMessageHeap"].as<uint32_t>()*1024,false);

    mDatabaseManager = new ::DatabaseManager(DatabaseConfig(configuration_variables_map_["DBMinThreads"].as<uint32_t>(), configuration_variables_map_["DBMaxThreads"].as<uint32_t>(), configuration_variables_map_["DBGlobalSchema"].as<std::string>(), configuration_variables_map_["DBGalaxySchema"].as<std::string>(), configuration_variables_map_["DBConfigSchema"].as<std::string>()));



    // Connect to our database and pass it off to our modules.
    mDatabase = mDatabaseManager->connect(DBTYPE_MYSQL,
                                          (char*)(configuration_variables_map_["DBServer"].as<std::string>()).c_str(),
                                          configuration_variables_map_["DBPort"].as<uint16_t>(),
                                          (char*)(configuration_variables_map_["DBUser"].as<std::string>()).c_str(),
                                          (char*)(configuration_variables_map_["DBPass"].as<std::string>()).c_str(),
                                          (char*)(configuration_variables_map_["DBName"].as<std::string>()).c_str());

    mDatabase->executeProcedureAsync(0, 0, "CALL %s.sp_ServerStatusUpdate('login', NULL, NULL, NULL);",mDatabase->galaxy()); // SQL - Update Server Start ID
    mDatabase->executeProcedureAsync(0, 0, "CALL %s.sp_ServerStatusUpdate('login', %u, NULL, NULL);",mDatabase->galaxy(), 1); // SQL - Update Server Status
        
    // In case of a crash, we need to cleanup the DB a little.
    mDatabase->destroyResult(mDatabase->executeSynchSql("UPDATE %s.account SET account_authenticated = 0 WHERE account_authenticated = 1;",mDatabase->galaxy()));
    
    //and session_key now as well
    mDatabase->destroyResult(mDatabase->executeSynchSql("UPDATE %s.account SET account_session_key = '';",mDatabase->galaxy()));
  
    // Instant the messageFactory. It will also run the Startup ().
    (void)MessageFactory::getSingleton();		// Use this a marker of where the factory is instanced.
    // The code itself here is not needed, since it will instance itself at first use.

    mLoginManager = new LoginManager(mDatabase);

    // Let our network Service know about our callbacks
    mService->AddNetworkCallback(mLoginManager);

    // We're done initializing.
    mDatabase->executeProcedureAsync(0, 0, "CALL %s.sp_ServerStatusUpdate('login', %u, '%s', %u);",mDatabase->galaxy(), 2, mService->getLocalAddress(), mService->getLocalPort()); // SQL - Update Server Details
    server_directory_->updateProcessStatus(server_directory_->process(), 0);

    LOG(WARNING) << "Login Server startup complete";
    //gLogger->printLogo();
    // std::string BuildString(GetBuildString());

    LOG(WARNING) <<  "Login Server - Build " << GetBuildString().c_str();
    LOG(WARNING) << "Welcome to your SWGANH Experience!";
}


//======================================================================================================================
LoginServer::~LoginServer(void)
{
    // server is shutting down so remove it from the cluster process list
    server_directory_->removeProcess(server_directory_->process());

    mDatabase->executeProcedureAsync(0, 0, "CALL %s.sp_ServerStatusUpdate('login', %u, NULL, NULL);",mDatabase->galaxy(), 2); // SQL - Update server status
    
    LOG(WARNING) << "LoginServer shutting down...";

    delete mLoginManager;

    mNetworkManager->DestroyService(mService);
    delete mNetworkManager;

    MessageFactory::getSingleton()->destroySingleton();	// Delete message factory and call shutdown();

    delete mDatabaseManager;

    LOG(WARNING) << "LoginServer Shutdown complete";
}

//======================================================================================================================
void LoginServer::Process(void)
{
    server_directory_->pulse();
    mNetworkManager->Process();
    mDatabaseManager->process();
    mLoginManager->Process();
    gMessageFactory->Process();
}


//======================================================================================================================
void handleExit(void)
{
    delete gLoginServer;
}


//======================================================================================================================
int main(int argc, char* argv[])
{
    // Initialize the google logging.
    google::InitGoogleLogging(argv[0]);

#ifndef _WIN32
    google::InstallFailureSignalHandler();
#endif

    FLAGS_log_dir = "./logs";
    FLAGS_stderrthreshold = 1;
    
    //set stdout buffers to 0 to force instant flush
    setvbuf( stdout, NULL, _IONBF, 0);

    bool exit = false;

    try {
        gLoginServer = new LoginServer(argc, argv);

        // Since startup completed successfully, now set the atexit().  Otherwise we try to gracefully shutdown a failed startup, which usually fails anyway.
        //atexit(handleExit);

        // Main loop
        while (!exit)
        {
            gLoginServer->Process();

            if(Anh_Utils::kbhit())
                if(std::cin.get() == 'q')
                    break;

            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        }

        // Shutdown things
        delete gLoginServer;
    } catch( std::exception& e ) {
        std::cout << e.what() << std::endl;
        std::cin.get();
        return 0;
    }

    return 0;
}




